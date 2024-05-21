#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>

#include "Common_Typedef.h"

constexpr const UINT32 MAX_SOCK_RECVBUF = 256;	// ���� ������ ũ��
constexpr const UINT32 MAX_SOCK_SENDBUF = 4096; // ���� ������ ũ��
constexpr const UINT32 MAX_ACCEPTBUF = 64;
constexpr const UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;

enum class IOOPERATION
{
	ACCEPT,
	RECV,
	SEND
};

struct OverlappedEx // WSAOVERLAPPED ����ü�� Ȯ����� �ʿ��� ������ �� ����
{
	WSAOVERLAPPED	m_wsaOverlapped;		// Overlapped I/O ����ü
	WSABUF			m_wsaBuf;				// Overlapped I/O �۾� ����
	IOOPERATION		m_eOperation;			// �۾� ���� ���� (����, �۽�)
	UINT32			m_uiSessionIndex = 0;		
};