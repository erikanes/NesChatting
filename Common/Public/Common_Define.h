#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>

#include "Common_Typedef.h"

constexpr const UINT32 MAX_SOCK_RECVBUF = 256;	// 소켓 버퍼의 크기
constexpr const UINT32 MAX_SOCK_SENDBUF = 4096; // 소켓 버퍼의 크기
constexpr const UINT32 MAX_ACCEPTBUF = 64;
constexpr const UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;

enum class IOOPERATION
{
	ACCEPT,
	RECV,
	SEND
};

struct OverlappedEx // WSAOVERLAPPED 구조체를 확장시켜 필요한 정보를 더 담음
{
	WSAOVERLAPPED	m_wsaOverlapped;		// Overlapped I/O 구조체
	WSABUF			m_wsaBuf;				// Overlapped I/O 작업 버퍼
	IOOPERATION		m_eOperation;			// 작업 동작 종류 (수신, 송신)
	UINT32			m_uiSessionIndex = 0;		
};