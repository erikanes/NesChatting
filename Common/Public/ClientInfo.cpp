#include "ClientInfo.h"
#include <iostream>

ClientInfo::ClientInfo()
{
	ZeroMemory(&m_stRecvOverlappedEx, sizeof(OverlappedEx));
	m_socket = INVALID_SOCKET;
}

void ClientInfo::Init(const UINT32 iIndex)
{
	m_iIndex = iIndex;
}

_bool ClientInfo::OnConnect(HANDLE iocpHandle, SOCKET sock)
{
	m_socket = sock;

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

	m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
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