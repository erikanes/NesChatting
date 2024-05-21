#include "ClientInfo.h"
#include <iostream>

ClientInfo::ClientInfo()
{
	ZeroMemory(&m_stRecvOverlappedEx, sizeof(OverlappedEx));
	m_socket = INVALID_SOCKET;
}

void ClientInfo::Init(const UINT32 iIndex, HANDLE iocpHandle)
{
	m_iIndex = iIndex;
	m_iocpHandle = iocpHandle;
}

_bool ClientInfo::OnConnect(HANDLE iocpHandle, SOCKET sock)
{
	m_socket = sock;
	m_bIsConnected = true;

	Clear();

	// IOCP 객체와 소켓을 연결
	if (false == BindIOCP(iocpHandle))
		return false;

	return BindRecv();
}

void ClientInfo::Close(_bool bIsForce)
{
	struct linger stLinger = { 0, 0 };

	if (bIsForce)
		stLinger.l_onoff = 1;

	shutdown(m_socket, SD_BOTH);
	
	setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (_char*)&stLinger, sizeof(stLinger));
	
	m_bIsConnected = false;
	m_uiLatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
}

void ClientInfo::Clear()
{
}

_bool ClientInfo::BindIOCP(HANDLE iocpHandle)
{
	auto hIOCP = CreateIoCompletionPort(
		(HANDLE)GetSocket(),
		iocpHandle,
		(ULONG_PTR)this,
		0);

	if (INVALID_HANDLE_VALUE == hIOCP)
	{
		std::cout << "[Error] Failed to CreateIoCompletionPort() : " << GetLastError() << '\n';
		return false;
	}

	return true;
}

_bool ClientInfo::BindRecv()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCK_RECVBUF;
	m_stRecvOverlappedEx.m_wsaBuf.buf = m_recvBuf;
	m_stRecvOverlappedEx.m_eOperation = IOOPERATION::RECV;

	_int iRet = WSARecv(
		m_socket,
		&(m_stRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED)&m_stRecvOverlappedEx,
		NULL);

	if (SOCKET_ERROR == iRet && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		std::cout << "[Error] Failed to WSARecv() : " << WSAGetLastError() << '\n';
		return false;
	}

	return true;
}

_bool ClientInfo::SendMsg(const UINT32 uiDataSize, _char* pMsg)
{
	auto pSendOverlappedEx = new OverlappedEx();
	ZeroMemory(pSendOverlappedEx, sizeof(OverlappedEx));

	pSendOverlappedEx->m_wsaBuf.len = uiDataSize;
	pSendOverlappedEx->m_wsaBuf.buf = new _char[uiDataSize];
	CopyMemory(pSendOverlappedEx->m_wsaBuf.buf, pMsg, uiDataSize);
	pSendOverlappedEx->m_eOperation = IOOPERATION::SEND;

	// 큐에 접근할때는 반드시 락을 건다
	std::lock_guard<std::mutex> guard(m_sendLock);

	m_sendDataQueue.push(pSendOverlappedEx);

	if (1 == m_sendDataQueue.size())
		return _SendIO();

	return true;
}

void ClientInfo::SendCompleted(const UINT32 uiDataSize)
{
	std::lock_guard<std::mutex> guard(m_sendLock);

	auto pSendOverlappedEx = m_sendDataQueue.front();

	delete[] pSendOverlappedEx->m_wsaBuf.buf;
	delete pSendOverlappedEx;

	m_sendDataQueue.pop();

	if (false == m_sendDataQueue.empty())
		_SendIO();

	std::cout << "[Send complete] bytes : " << uiDataSize << '\n';
}

_bool ClientInfo::PostAccept(SOCKET listenSock, const UINT64 uiCurTimeSec)
{
	std::cout << "Post accept. Client index: " << GetIndex() << '\n';

	m_uiLatestClosedTimeSec = UINT32_MAX;
	m_socket = WSASocket(
		AF_INET,
		SOCK_STREAM,
		IPPROTO_IP,
		NULL,
		0,
		WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == m_socket)
	{
		std::cout << "Client socket WSASocket error: " << GetLastError() << '\n';
		return false;
	}

	ZeroMemory(&m_stAcceptContext, sizeof(OverlappedEx));

	DWORD bytes = 0;
	DWORD flags = 0;
	m_stAcceptContext.m_wsaBuf.len = 0;
	m_stAcceptContext.m_wsaBuf.buf = nullptr;
	m_stAcceptContext.m_eOperation = IOOPERATION::ACCEPT;
	m_stAcceptContext.m_uiSessionIndex = m_iIndex;

	auto bRet = AcceptEx(
		listenSock,
		m_socket,
		m_acceptBuf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&bytes,
		(LPWSAOVERLAPPED)&m_stAcceptContext);

	if (FALSE == bRet)
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			std::cout << "AcceptEx Error: " << GetLastError() << '\n';
			return false;
		}
	}

	return true;
}

_bool ClientInfo::AcceptCompletion()
{
	std::cout << "Accept Completion: Session index(" << m_iIndex << ")" << '\n';

	if (false == OnConnect(m_iocpHandle, m_socket))
		return false;

	SOCKADDR_IN stClientAddr;
	_int iAddrlen = sizeof(SOCKADDR_IN);
	_char szClientIP[32] = { 0, };
	inet_ntop(AF_INET, &(stClientAddr.sin_addr), szClientIP, 32 - 1);

	std::cout << "Client connection. IP(" << szClientIP << ")" << "(" << (_int)m_socket << ")" << '\n';

	return true;
}

_bool ClientInfo::_SendIO()
{
	auto pSendOverlappedEx = m_sendDataQueue.front();

	DWORD dwRecvNumBytes = 0;
	_int iRet = WSASend(
		m_socket,
		&(pSendOverlappedEx->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)pSendOverlappedEx,
		NULL);

	// SOCKET_ERROR를 client socket이 끊어진 것으로 처리
	if (SOCKET_ERROR == iRet && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cout << "[Error] Failed to WSASend() : " << WSAGetLastError() << '\n';
		return false;
	}

	return true;
}

_bool ClientInfo::_SetSocketOption()
{
	_int opt = 1;
	if (SOCKET_ERROR == setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(_int)))
	{
		std::cout << "[Debug] TCP_NODELAY error: " << GetLastError() << '\n';
		return false;
	}

	opt = 0;
	if (SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(_int)))
	{
		std::cout << "[Debug] SO_RECBUF change error: " << GetLastError() << '\n';
		return false;
	}

	return true;
}