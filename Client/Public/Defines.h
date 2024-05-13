#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

constexpr UINT32 MAX_SOCKBUF = 256;		// 패킷의 크기
constexpr UINT32 MAX_WORKERTHREAD = 8;	// 스레드 풀에 넣을 스레드의 수

enum class IOOPERATION
{
	RECV,
	SEND
};

#include "../Public/Struct.h"