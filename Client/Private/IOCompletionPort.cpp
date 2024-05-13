#include "../Public/IOCompletionPort.h"
#include <iostream>

IOCompletionPort::~IOCompletionPort()
{
	WSACleanup(); // 윈속 사용 종료
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

	// 연결지향형 TCP, Overlapped I/O 생성
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
	stServerAddr.sin_port = htons(iBindPort); // 서버 포트 설정

	// 일반적인 서버는 어떤 주소에서 들어오는 접속이라도 받아들인다.
	// 만약 단일 아이피에서만 접속을 받고 싶으면 inet_addr 함수를 통해 넣어주면 된다.
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// 위에서 지정한 서버 주소 정보와 IOCP 소켓을 연결한다
	_int iRet = bind(m_socketListen, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != iRet)
	{
		std::cout << "[Error] Failed to bind() : " << WSAGetLastError() << '\n';
		return false;
	}

	// 접속 요청을 받아들이기 위해 IOCP 소켓을 등록하고 접속 대기 큐를 5개로 설정
	iRet = listen(m_socketListen, 5);
	if (0 != iRet)
	{
		std::cout << "[Error] Failed to listen() : " << WSAGetLastError() << '\n';
		return false;
	}

	std::cout << "서버 등록 성공." << '\n';

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

	std::cout << "서버 시작." << '\n';

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

	// WaitingThreadQueue에 대기 상태로 넣을 스레드의 개수
	// 권장 개수 : (프로세서 개수 * 2) + 1
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
		if (INVALID_SOCKET == client.m_socketClient) // 유효하지 않은 소켓 == 비어있는 소켓
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
		// 이 함수로 인해 스레드들은 WaitingThreadQueue에 대기 상태로 들어가게 된다.
		// 완료된 Overlapped I/O 작업이 발생하면 IOCP Queue에서 완료된 작업을 가져와서 후처리를 한다.
		// PostQueuedCompletionStatus() 함수에 의해 사용자 메시지가 도착하면 스레드를 종료한다.

		bSuccess = GetQueuedCompletionStatus(
			m_IOCPHandle,				// IOCP 핸들
			&dwIoSize,					// 실제로 전송된 바이트
			(PULONG_PTR)&pClientInfo,	// Completion Key
			&lpOverlapped,				// Overlapped I/O 객체
			INFINITE);					// 대기할 시간

		// 사용자 스레드 종료 메시지 처리
		if (true == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			m_bIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
			continue;

		// 클라이언트가 접속을 끊었을 때
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

			// 클라이언트에게 메시지를 에코한다
			_SendMsg(pClientInfo, szBuf, dwIoSize);
			_BindRecv(pClientInfo);
		}

		else if (IOOPERATION::SEND == eOperation)
		{
			auto& szBuf = pOverlappedEx->m_szBuf;

			std::cout << "[Send] bytes : " << dwIoSize << ", msg : " << szBuf << '\n';
		}

		else // 예외상황
			std::cout << "[Exception] socket(" << (_int)pClientInfo->m_socketClient << ")" << '\n';
	}
}

// 1. 소켓 생성
// 2. iocp 객체 생성
// 3. 큐 생성
// 4. 접속 대기 (accept)
// 5. iocp 객체와 소켓 연결

void IOCompletionPort::_AcceptThread()
{
	SOCKADDR_IN	stClientAddr;
	_int		iAddrLen = sizeof(SOCKADDR_IN);

	while (m_bIsAcceptRun)
	{
		// 접속을 받을 구조체의 인덱스를 얻어옴
		ClientInfo* pClientInfo = _GetEmptyClientInfo();

		if (nullptr == pClientInfo)
		{
			std::cout << "[Error] Client is full." << '\n';
			return;
		}

		// 클라이언트 접속 요청이 들어올 때까지 대기
		pClientInfo->m_socketClient = accept(m_socketListen, (SOCKADDR*)&stClientAddr, &iAddrLen);
		if (INVALID_SOCKET == pClientInfo->m_socketClient)
			continue;

		// IOCP 객체와 소켓을 연결시킨다
		_bool bRet = _BindIOCP(pClientInfo);
		if (false == bRet)
			return;

		// Recv overlapped I/O 작업을 요청해 놓음
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
	struct linger stLinger = { 0, 0 }; // SO_DONTLINGER로 설정

	// 강제종료 시 timeout을 0으로 설정하여 강제 종료 시킨다.
	// 주의 : 데이터 손실이 발생할 수 있음
	if (true == bIsForce)
		stLinger.l_onoff = 1;

	// 해당 소켓의 데이터 송수신을 모두 중단시킨다
	shutdown(pClientInfo->m_socketClient, SD_BOTH);

	// 소켓 옵션 설정
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (_char*)&stLinger, sizeof(stLinger));

	// 소켓 연결 종료
	closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;
}