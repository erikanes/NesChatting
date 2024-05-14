#include "IOCPServer.h"
#include <iostream>

IOCPServer::~IOCPServer()
{
	WSACleanup(); // 윈속 사용 종료
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

_bool IOCPServer::BindAndListen(_int iBindPort)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(iBindPort); // 서버 포트 설정

	// 일반적인 서버는 어떤 주소에서 들어오는 접속이라도 받아들인다.
	// 만약 단일 아이피에서만 접속을 받고 싶으면 inet_addr 함수를 통해 넣어주면 된다.
	// inet_addr : 문자열로 된 IP를 32비트 ulong으로 변환.
	// stServerAddr.sin_addr.s_addr = inet_addr("192.168.0.1"); => inet_addr은 이미 네트워크 바이트로 정렬된 값을 리턴하기 때문에 htonl을 사용하면 안됨!
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

	// WaitingThreadQueue에 대기 상태로 넣을 스레드의 개수
	// 권장 개수 : (프로세서 개수 * 2) + 1
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
		// 이 함수로 인해 스레드들은 WaitingThreadQueue에 대기 상태로 들어가게 된다.
		// 완료된 Overlapped I/O 작업이 발생하면 IOCP Queue에서 완료된 작업을 가져와서 후처리를 한다.
		// PostQueuedCompletionStatus() 함수에 의해 사용자 메시지가 도착하면 스레드를 종료한다.

		bSuccess = GetQueuedCompletionStatus(
			m_IOCPHandle,				// IOCP 핸들
			&dwIoSize,					// 실제로 전송된 바이트
			(PULONG_PTR)&pClientInfo,	// Completion Key
			&lpOverlapped,				// Overlapped I/O 객체
			INFINITE					// 대기할 시간
		);

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

		else // 예외상황
			std::cout << "[Exception] socket(" << pClientInfo->GetIndex() << ")" << '\n';
	}
}

// 1. 소켓 생성
// 2. iocp 객체 생성
// 3. 큐 생성
// 4. 접속 대기 (accept)
// 5. iocp 객체와 소켓 연결

void IOCPServer::_AcceptThread()
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