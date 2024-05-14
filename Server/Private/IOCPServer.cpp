#include "IOCPServer.h"
#include <iostream>

IOCPServer::~IOCPServer()
{
	WSACleanup(); // ���� ��� ����
}

_bool IOCPServer::InitSocket()
{
	WSADATA wsaData;

	_int iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != iRet)
	{
		std::cout << "[Error] Failed to WSAStartup() : " << WSAGetLastError() << '\n';
		return false;
	}

	// ���������� TCP, Overlapped I/O ����
	m_socketListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == m_socketListen)
	{
		std::cout << "[Error] Failed to WSASocket() : " << WSAGetLastError() << '\n';
		return false;
	}

	std::cout << "Socket initialization successful." << '\n';

	return true;
}

_bool IOCPServer::BindAndListen(_int iBindPort)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(iBindPort); // ���� ��Ʈ ����

	// �Ϲ����� ������ � �ּҿ��� ������ �����̶� �޾Ƶ��δ�.
	// ���� ���� �����ǿ����� ������ �ް� ������ inet_addr �Լ��� ���� �־��ָ� �ȴ�.
	// inet_addr : ���ڿ��� �� IP�� 32��Ʈ ulong���� ��ȯ.
	// stServerAddr.sin_addr.s_addr = inet_addr("192.168.0.1"); => inet_addr�� �̹� ��Ʈ��ũ ����Ʈ�� ���ĵ� ���� �����ϱ� ������ htonl�� ����ϸ� �ȵ�!
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);


	// ������ ������ ���� �ּ� ������ IOCP ������ �����Ѵ�
	_int iRet = bind(m_socketListen, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != iRet)
	{
		std::cout << "[Error] Failed to bind() : " << WSAGetLastError() << '\n';
		return false;
	}

	// ���� ��û�� �޾Ƶ��̱� ���� IOCP ������ ����ϰ� ���� ��� ť�� 5���� ����
	iRet = listen(m_socketListen, 5);
	if (0 != iRet)
	{
		std::cout << "[Error] Failed to listen() : " << WSAGetLastError() << '\n';
		return false;
	}

	std::cout << "���� ��� ����." << '\n';

	return true;
}

_bool IOCPServer::StartServer(const UINT32 iMaxClientCount)
{
	_CreateClient(iMaxClientCount);

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == m_IOCPHandle)
	{
		std::cout << "[Error] Failed to CreateIoCompletionPort() : " << WSAGetLastError() << '\n';
		return false;
	}

	_bool bRet = _CreateWorkerThread();
	if (false == bRet)
		return false;

	bRet = _CreateAccepterThread();
	if (false == bRet)
		return false;

	std::cout << "Server start" << '\n';

	return true;
}

_bool IOCPServer::Run(const UINT32 uiMaxClientCount)
{
	return StartServer(uiMaxClientCount);
}

void IOCPServer::End()
{
	DestroyThread();
}

_bool IOCPServer::SendMsg(const UINT32 uiSessionIndex, const UINT32 uiDataSize, _char* pData)
{
	auto pClient = _GetClientInfo(uiSessionIndex);
	if (nullptr == pClient)
		return false;

	return pClient->SendMsg(uiDataSize, pData);
}

_bool IOCPServer::SendMsg(PacketData& packetData)
{
	return SendMsg(packetData.m_uiSessionIndex, packetData.m_uiDataSize, packetData.m_pData);
}

void IOCPServer::DestroyThread()
{
	m_bIsWorkerRun = false;
	CloseHandle(m_IOCPHandle);

	for (auto& th : m_IOWorkerThreads)
		if (th.joinable()) th.join();

	m_bIsAcceptRun = false;
	closesocket(m_socketListen);

	if (m_acceptThread.joinable())
		m_acceptThread.join();
}

void IOCPServer::_CreateClient(const UINT32 iMaxClientCount)
{
	for (UINT32 i = 0; i < iMaxClientCount; ++i)
	{
		m_clientInfos.emplace_back();
		m_clientInfos[i].Init(i);
	}
}

_bool IOCPServer::_CreateWorkerThread()
{
	_uint uiThreadId = 0;

	// WaitingThreadQueue�� ��� ���·� ���� �������� ����
	// ���� ���� : (���μ��� ���� * 2) + 1
	for (_int i = 0; i < MAX_WORKERTHREAD; ++i)
		m_IOWorkerThreads.emplace_back([this]() { _WorkerThread(); });

	std::cout << "Start worker threads..." << '\n';

	return true;
}

_bool IOCPServer::_CreateAccepterThread()
{
	m_acceptThread = std::thread([this]() { _AcceptThread(); });

	std::cout << "Start accepter thread..." << '\n';

	return true;
}

ClientInfo* IOCPServer::_GetEmptyClientInfo()
{
	for (auto& client : m_clientInfos)
	{
		if (false == client.IsConnected())
			return &client;
	}

	return nullptr;
}

ClientInfo* IOCPServer::_GetClientInfo(const UINT32 uiSessionIndex)
{
	if (uiSessionIndex >= m_clientInfos.size())
		return nullptr;

	return &m_clientInfos[uiSessionIndex];
}

void IOCPServer::_WorkerThread()
{
	ClientInfo* pClientInfo = nullptr;
	_bool			bSuccess = true;
	DWORD			dwIoSize = 0;
	LPOVERLAPPED	lpOverlapped = NULL;

	while (m_bIsWorkerRun)
	{
		// �� �Լ��� ���� ��������� WaitingThreadQueue�� ��� ���·� ���� �ȴ�.
		// �Ϸ�� Overlapped I/O �۾��� �߻��ϸ� IOCP Queue���� �Ϸ�� �۾��� �����ͼ� ��ó���� �Ѵ�.
		// PostQueuedCompletionStatus() �Լ��� ���� ����� �޽����� �����ϸ� �����带 �����Ѵ�.

		bSuccess = GetQueuedCompletionStatus(
			m_IOCPHandle,				// IOCP �ڵ�
			&dwIoSize,					// ������ ���۵� ����Ʈ
			(PULONG_PTR)&pClientInfo,	// Completion Key
			&lpOverlapped,				// Overlapped I/O ��ü
			INFINITE					// ����� �ð�
		);

		// ����� ������ ���� �޽��� ó��
		if (true == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			m_bIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
			continue;

		// Ŭ���̾�Ʈ�� ������ ������ ��
		if (false == bSuccess || (0 == dwIoSize && true == bSuccess))
		{
			//std::cout << "Disconnect socket(" << (_int)pClientInfo->m_socketClient << ")" << '\n';
			_CloseSocket(pClientInfo);
			continue;
		}

		auto pOverlappedEx = (OverlappedEx*)lpOverlapped;
		const auto eOperation = pOverlappedEx->m_eOperation;

		if (IOOPERATION::RECV == eOperation)
		{
			auto iIndex = pClientInfo->GetIndex();
			auto szBuf = pClientInfo->GetRecvBuffer();

			OnReceive(iIndex, dwIoSize, szBuf);
			pClientInfo->BindRecv();
		}

		else if (IOOPERATION::SEND == eOperation)
		{
			delete[] pOverlappedEx->m_wsaBuf.buf;
			delete pOverlappedEx;

			pClientInfo->SendCompleted(dwIoSize);
		}

		else // ���ܻ�Ȳ
			std::cout << "[Exception] socket(" << pClientInfo->GetIndex() << ")" << '\n';
	}
}

// 1. ���� ����
// 2. iocp ��ü ����
// 3. ť ����
// 4. ���� ��� (accept)
// 5. iocp ��ü�� ���� ����

void IOCPServer::_AcceptThread()
{
	SOCKADDR_IN	stClientAddr;
	_int		iAddrLen = sizeof(SOCKADDR_IN);

	while (m_bIsAcceptRun)
	{
		// ������ ���� ����ü�� �ε����� ����
		ClientInfo* pClientInfo = _GetEmptyClientInfo();
		if (nullptr == pClientInfo)
		{
			std::cout << "[Error] Client is full." << '\n';
			return;
		}

		// Ŭ���̾�Ʈ ���� ��û�� ���� ������ ���
		auto newSocket = accept(m_socketListen, (SOCKADDR*)&stClientAddr, &iAddrLen);
		if (INVALID_SOCKET == newSocket)
			continue;

		if (false == pClientInfo->OnConnect(m_IOCPHandle, newSocket))
		{
			pClientInfo->Close(true);
			return;
		}

		OnConnect(pClientInfo->GetIndex());

		++m_iClientCount;
	}
}

void IOCPServer::_CloseSocket(ClientInfo* pClientInfo, _bool bIsForce)
{
	auto iClientIndex = pClientInfo->GetIndex();
	pClientInfo->Close(bIsForce);

	OnClose(iClientIndex);
}