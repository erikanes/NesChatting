#pragma once

struct OverlappedEx // WSAOVERLAPPED 구조체를 확장시켜 필요한 정보를 더 담음
{
	WSAOVERLAPPED	m_wsaOverlapped;		// Overlapped I/O 구조체
	SOCKET			m_socketClient;			// 클라이언트 소켓
	WSABUF			m_wsaBuf;				// Overlapped I/O 작업 버퍼
	IOOPERATION		m_eOperation;			// 작업 동작 종류 (수신, 송신)
};

struct ClientInfo
{
	ClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(OverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(OverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}

	SOCKET			m_socketClient;			// 클라이언트와 연결되는 소켓
	OverlappedEx	m_stRecvOverlappedEx;	// Recv Overlapped I/O 작업을 위한 구조체
	OverlappedEx	m_stSendOverlappedEx;	// Send Overlapped I/O 작업을 위한 구조체

	_char			m_recvBuf[MAX_SOCKBUF];	// 수신 데이터 버퍼
	_char			m_sendBuf[MAX_SOCKBUF]; // 송신 데이터 버퍼
};