#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

constexpr UINT32 MAX_SOCKBUF = 256;		// ��Ŷ�� ũ��
constexpr UINT32 MAX_WORKERTHREAD = 8;	// ������ Ǯ�� ���� �������� ��

enum class IOOPERATION
{
	RECV,
	SEND
};

#include "../Public/Struct.h"