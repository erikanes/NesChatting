#include "IOCPServer.h"
#include <iostream>

IOCPServer::~IOCPServer()
{
	for (auto& pClient : m_clientInfos)
		delete pClient;

	m_clientInfos.clear();
	m_clientInfos.shrink_to_fit();

	WSACleanup(); // ���� ��� ����
}

_bool IOCPServer::InitSocket(const UINT32 uiMaxIOWorkerThreadCount)
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

	m_uiMaxIOWorkerThreadCount = uiMaxIOWorkerThreadCount;

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

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_uiMaxIOWorkerThreadCount);
	if (nullptr == m_IOCPHandle)
	{
		std::cout << "[Error] Failed to CreateIoCompletionPort() : " << WSAGetLastError() << '\n';
		return false;
	}

	auto hIOCPHandle = CreateIoCompletionPort((HANDLE)m_socketListen, m_IOCPHandle, (UINT32)0, 0);
	if (nullptr == hIOCPHandle)
	{
		std::cout << "[Error] Failed to bind listen socket : " << WSAGetLastError() << '\n';
		return false;
	}

	std::cout << "Server registration successful." << '\n';

	return true;
}

_bool IOCPServer::StartServer(const UINT32 iMaxClientCount)
{
	_CreateClient(iMaxClientCount);

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

_bool IOCPServer::SendMsg(RawPacketData& packetData)
{
	return SendMsg(packetData.m_uiClientIndex, packetData.m_uiDataSize, packetData.m_pData);
}

void IOCPServer::DestroyThread()
{
	// ���� ������ �߿���
	// accept�� �񵿱�� �����ߴٸ� ���߿� �����ؾ� ��
	// �ڵ��� ���� ��ȯ�ؾ� ��۸��� �߻����� ����

	m_bIsWorkerRun = false;
	CloseHandle(m_IOCPHandle);

	for (auto& th : m_IOWorkerThreads)
		if (th.joinable()) th.join();

	m_bIsAccepterRun = false;
	closesocket(m_socketListen);

	if (m_acceptThread.joinable())
		m_acceptThread.join();
}

void IOCPServer::_CreateClient(const UINT32 iMaxClientCount)
{
	for (UINT32 i = 0; i < iMaxClientCount; ++i)
	{
		auto pClient = new ClientInfo();
		pClient->Init(i, m_IOCPHandle);

		m_clientInfos.push_back(pClient);
	}
}

_bool IOCPServer::_CreateWorkerThread()
{
	m_bIsWorkerRun = true;

	// WaitingThreadQueue�� ��� ���·� ���� �������� ����
	// ���� ���� : (���μ��� ���� * 2) + 1
	for (UINT32 i = 0; i < m_uiMaxIOWorkerThreadCount; ++i)
		m_IOWorkerThreads.emplace_back([this]() { _WorkerThread(); });

	std::cout << "Start worker threads..." << '\n';

	return true;
}

_bool IOCPServer::_CreateAccepterThread()
{
	m_bIsAccepterRun = true;
	m_acceptThread = std::thread([this]() { _AcceptThread(); });

	std::cout << "Start accepter thread..." << '\n';

	return true;
}

ClientInfo* IOCPServer::_GetEmptyClientInfo()
{
	for (auto& pClient : m_clientInfos)
	{
		if (false == pClient->IsConnected())
			return pClient;
	}

	return nullptr;
}

ClientInfo* IOCPServer::_GetClientInfo(const UINT32 uiSessionIndex)
{
	if (uiSessionIndex >= m_clientInfos.size())
		return nullptr;

	return m_clientInfos[uiSessionIndex];
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

		auto pOverlappedEx = (OverlappedEx*)lpOverlapped;

		// Ŭ���̾�Ʈ�� ������ ������ ��
		if (false == bSuccess || (0 == dwIoSize && IOOPERATION::ACCEPT != pOverlappedEx->m_eOperation))
		{
			_CloseSocket(pClientInfo);
			continue;
		}

		const auto eOperation = pOverlappedEx->m_eOperation;

		if (IOOPERATION::ACCEPT == eOperation)
		{
			pClientInfo = _GetClientInfo(pOverlappedEx->m_uiSessionIndex);
			if (nullptr == pClientInfo)
				continue;

			if (pClientInfo->AcceptCompletion())
			{
				++m_iClientCount;
				OnConnect(pClientInfo->GetIndex());
			}

			else
				_CloseSocket(pClientInfo, true);
		}

		else if (IOOPERATION::RECV == eOperation)
		{
			auto iIndex = pClientInfo->GetIndex();
			auto szBuf = pClientInfo->GetRecvBuffer();

			OnReceive(iIndex, dwIoSize, szBuf);
			pClientInfo->BindRecv();
		}

		else if (IOOPERATION::SEND == eOperation)
			pClientInfo->SendCompleted(dwIoSize); // ����� ���ڼ����̱� ������ ���ٸ� ó�� ���ϴ� ��

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
	while (m_bIsAccepterRun)
	{
		const auto iCurTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		for (auto pClient : m_clientInfos)
		{
			if (pClient->IsConnected()) // �̹� ����� ���¶�� �Ѿ��
				continue;

			const auto iLatestClosedTimeSec = pClient->GetLatestClosedTimeSec();

			if ((UINT64)iCurTimeSec < iLatestClosedTimeSec) // ���� ���� �ð��� ���� ������ ���� ���
				continue;

			auto iDiff = iCurTimeSec - iLatestClosedTimeSec;
			if (iDiff <= RE_USE_SESSION_WAIT_TIMESEC) // �翬�� ���ð�
				continue;

			pClient->PostAccept(m_socketListen, iCurTimeSec);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
}

void IOCPServer::_CloseSocket(ClientInfo* pClientInfo, _bool bIsForce)
{
	auto iClientIndex = pClientInfo->GetIndex();
	pClientInfo->Close(bIsForce);

	OnClose(iClientIndex);
}