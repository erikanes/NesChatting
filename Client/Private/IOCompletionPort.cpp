#include "../Public/IOCompletionPort.h"
#include <iostream>

IOCompletionPort::~IOCompletionPort()
{
	WSACleanup(); // ���� ��� ����
}

_bool IOCompletionPort::InitSocket()
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

_bool IOCompletionPort::BindAndListen(_int iBindPort)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(iBindPort); // ���� ��Ʈ ����

	// �Ϲ����� ������ � �ּҿ��� ������ �����̶� �޾Ƶ��δ�.
	// ���� ���� �����ǿ����� ������ �ް� ������ inet_addr �Լ��� ���� �־��ָ� �ȴ�.
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

_bool IOCompletionPort::StartServer(const UINT32 iMaxClientCount)
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

	std::cout << "���� ����." << '\n';

	return true;
}

void IOCompletionPort::DestroyThread()
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

void IOCompletionPort::_CreateClient(const UINT32 iMaxClientCount)
{
	for (UINT32 i = 0; i < iMaxClientCount; ++i)
		m_clientInfos.emplace_back();
}

_bool IOCompletionPort::_CreateWorkerThread()
{
	_uint uiThreadId = 0;

	// WaitingThreadQueue�� ��� ���·� ���� �������� ����
	// ���� ���� : (���μ��� ���� * 2) + 1
	for (_int i = 0; i < MAX_WORKERTHREAD; ++i)
		m_IOWorkerThreads.emplace_back([this]() { _WorkerThread(); });

	std::cout << "Start worker threads..." << '\n';

	return true;
}

_bool IOCompletionPort::_CreateAccepterThread()
{
	m_acceptThread = std::thread([this]() { _AcceptThread(); });

	std::cout << "Start accepter thread..." << '\n';

	return true;
}

ClientInfo* IOCompletionPort::_GetEmptyClientInfo()
{
	for (auto& client : m_clientInfos)
	{
		if (INVALID_SOCKET == client.m_socketClient) // ��ȿ���� ���� ���� == ����ִ� ����
			return &client;
	}

	return nullptr;
}

_bool IOCompletionPort::_BindIOCP(ClientInfo* pClientInfo)
{
	auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient, m_IOCPHandle, (ULONG_PTR)(pClientInfo), 0);

	if (NULL == hIOCP || m_IOCPHandle != hIOCP)
	{
		std::cout << "[Error] Failed to CreateIoCompletionPort() : " << GetLastError() << '\n';
		return false;
	}

	return true;
}

_bool IOCompletionPort::_BindRecv(ClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stRecvOverlappedEx.m_szBuf;
	pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOPERATION::RECV;

	_int iRet = WSARecv(
		pClientInfo->m_socketClient,
		&(pClientInfo->m_stRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED)&(pClientInfo->m_stRecvOverlappedEx),
		NULL);

	if (SOCKET_ERROR == iRet && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		std::cout << "[Error] Failed to WSARecv() : " << WSAGetLastError() << '\n';
		return false;
	}

	return true;
}

_bool IOCompletionPort::_SendMsg(ClientInfo* pClientInfo, _char* pMsg, _int iLen)
{
	DWORD dwRecvNumBytes = 0;

	CopyMemory(pClientInfo->m_stSendOverlappedEx.m_szBuf, pMsg, iLen);

	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = iLen;
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stSendOverlappedEx.m_szBuf;
	pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOPERATION::SEND;

	_int iRet = WSASend(
		pClientInfo->m_socketClient,
		&(pClientInfo->m_stSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)&(pClientInfo->m_stSendOverlappedEx),
		NULL);

	if (SOCKET_ERROR == iRet && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		std::cout << "[Error] Failed to WSASend() : " << WSAGetLastError() << '\n';
		return false;
	}

	return false;
}

void IOCompletionPort::_WorkerThread()
{
	ClientInfo*		pClientInfo		= nullptr;
	_bool			bSuccess		= true;
	DWORD			dwIoSize		= 0;
	LPOVERLAPPED	lpOverlapped	= NULL;

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
			INFINITE);					// ����� �ð�

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
			std::cout << "Disconnect socket(" << (_int)pClientInfo->m_socketClient << ")" << '\n';
			_CloseSocket(pClientInfo);
			continue;
		}

		OverlappedEx* pOverlappedEx = (OverlappedEx*)lpOverlapped;
		const auto eOperation = pOverlappedEx->m_eOperation;

		if (IOOPERATION::RECV == eOperation)
		{
			auto& szBuf = pOverlappedEx->m_szBuf;

			szBuf[dwIoSize] = NULL;
			std::cout << "[Recv] bytes : " << dwIoSize << ", msg : " << szBuf << '\n';

			// Ŭ���̾�Ʈ���� �޽����� �����Ѵ�
			_SendMsg(pClientInfo, szBuf, dwIoSize);
			_BindRecv(pClientInfo);
		}

		else if (IOOPERATION::SEND == eOperation)
		{
			auto& szBuf = pOverlappedEx->m_szBuf;

			std::cout << "[Send] bytes : " << dwIoSize << ", msg : " << szBuf << '\n';
		}

		else // ���ܻ�Ȳ
			std::cout << "[Exception] socket(" << (_int)pClientInfo->m_socketClient << ")" << '\n';
	}
}

// 1. ���� ����
// 2. iocp ��ü ����
// 3. ť ����
// 4. ���� ��� (accept)
// 5. iocp ��ü�� ���� ����

void IOCompletionPort::_AcceptThread()
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
		pClientInfo->m_socketClient = accept(m_socketListen, (SOCKADDR*)&stClientAddr, &iAddrLen);
		if (INVALID_SOCKET == pClientInfo->m_socketClient)
			continue;

		// IOCP ��ü�� ������ �����Ų��
		_bool bRet = _BindIOCP(pClientInfo);
		if (false == bRet)
			return;

		// Recv overlapped I/O �۾��� ��û�� ����
		bRet = _BindRecv(pClientInfo);
		if (false == bRet)
			return;

		_char szClientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), szClientIP, 32 - 1);
		std::cout << "[Connect] IP : (" << szClientIP << ") socket(" << (_int)pClientInfo->m_socketClient << ")" << '\n';

		++m_iClientCount;
	}
}

void IOCompletionPort::_CloseSocket(ClientInfo* pClientInfo, _bool bIsForce)
{
	struct linger stLinger = { 0, 0 }; // SO_DONTLINGER�� ����

	// �������� �� timeout�� 0���� �����Ͽ� ���� ���� ��Ų��.
	// ���� : ������ �ս��� �߻��� �� ����
	if (true == bIsForce)
		stLinger.l_onoff = 1;

	// �ش� ������ ������ �ۼ����� ��� �ߴܽ�Ų��
	shutdown(pClientInfo->m_socketClient, SD_BOTH);

	// ���� �ɼ� ����
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (_char*)&stLinger, sizeof(stLinger));

	// ���� ���� ����
	closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;
}