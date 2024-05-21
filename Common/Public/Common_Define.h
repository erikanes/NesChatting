#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "Common_Typedef.h"

constexpr UINT32 MAX_SOCKBUF = 256;		// ���� ������ ũ��
constexpr UINT32 MAX_SOCK_SENDBUF = 4096; // ���� ������ ũ��
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