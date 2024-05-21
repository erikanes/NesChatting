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

	// IOCP ��ü�� ������ ����
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
	m_uiSendPos = 0;
	m_bIsSending = false;
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

// 1���� �����忡���� ȣ���ؾ� ��
_bool ClientInfo::SendMsg(const UINT32 uiDataSize, _char* pMsg)
{
	// SendMsg�� ���� ���ۿ� �����͸� ������ �ִ� �ϸ� �Ѵ�

	std::lock_guard<std::mutex> guard(m_sendLock);

	// �о�� �ϴ� �������� ���̰� ���� ����� �ѱ�ٸ� ��ġ�� �ʱ�ȭ ��Ŵ
	if (MAX_SOCK_SENDBUF < m_uiSendPos + uiDataSize)
		m_uiSendPos = 0;

	auto pSendBuf = &m_sendBuf[m_uiSendPos];

	// ���۵� �޽��� ����
	CopyMemory(pSendBuf, pMsg, uiDataSize);
	m_uiSendPos += uiDataSize;

	return true;
}

_bool ClientInfo::SendIO()
{
	if (0 >= m_uiSendPos || m_bIsSending)
		return true;

	std::lock_guard<std::mutex> guard(m_sendLock);
	m_bIsSending = true;

	CopyMemory(m_sendingBuf, &m_sendBuf[0], m_uiSendPos);

	m_stSendOverlappedEx.m_wsaBuf.len = (ULONG)m_uiSendPos;
	m_stSendOverlappedEx.m_wsaBuf.buf = &m_sendingBuf[0];
	m_stSendOverlappedEx.m_eOperation = IOOPERATION::SEND;

	DWORD dwRecvNumBytes = 0;
	_int iRet = WSASend(
		m_socket,
		&(m_stSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)&m_stSendOverlappedEx,
		NULL);

	// SOCKET_ERROR�� client socket�� ������ ������ ó��
	if (SOCKET_ERROR == iRet && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cout << "[Error] Failed to WSASend() : " << WSAGetLastError() << '\n';
		return false;
	}

	return true;
}

void ClientInfo::SendCompleted(const UINT32 uiDataSize)
{
	m_bIsSending = false;
	std::cout << "[Send complete] bytes : " << uiDataSize << '\n';
}