#pragma once

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

	SOCKET			m_socketClient;			// Ŭ���̾�Ʈ�� ����Ǵ� ����
	OverlappedEx	m_stRecvOverlappedEx;	// Recv Overlapped I/O �۾��� ���� ����ü
	OverlappedEx	m_stSendOverlappedEx;	// Send Overlapped I/O �۾��� ���� ����ü

	_char			m_recvBuf[MAX_SOCKBUF];	// ���� ������ ����
	_char			m_sendBuf[MAX_SOCKBUF]; // �۽� ������ ����
};