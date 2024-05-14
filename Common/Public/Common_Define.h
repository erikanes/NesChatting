#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "Common_Typedef.h"

constexpr UINT32 MAX_SOCKBUF = 256;		// ��Ŷ�� ũ��
constexpr UINT32 MAX_WORKERTHREAD = 8;	// ������ Ǯ�� ���� �������� ��

enum class IOOPERATION
{
	RECV,
	SEND
};

struct OverlappedEx // WSAOVERLAPPED ����ü�� Ȯ����� �ʿ��� ������ �� ����
{
	WSAOVERLAPPED	m_wsaOverlapped;		// Overlapped I/O ����ü
	SOCKET			m_socketClient;			// Ŭ���̾�Ʈ ����
	WSABUF			m_wsaBuf;				// Overlapped I/O �۾� ����
	IOOPERATION		m_eOperation;			// �۾� ���� ���� (����, �۽�)
};

struct ClientInfo
{
	ClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(OverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(OverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}

	INT32			m_iIndex = 0;
	SOCKET			m_socketClient;			// Ŭ���̾�Ʈ�� ����Ǵ� ����
	OverlappedEx	m_stRecvOverlappedEx;	// Recv Overlapped I/O �۾��� ���� ����ü
	OverlappedEx	m_stSendOverlappedEx;	// Send Overlapped I/O �۾��� ���� ����ü

	_char			m_recvBuf[MAX_SOCKBUF];	// ���� ������ ����
	_char			m_sendBuf[MAX_SOCKBUF]; // �۽� ������ ����
};