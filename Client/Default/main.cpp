#include <iostream>
#include "IOCompletionPort.h"

const UINT16	SERVER_PORT	= 11021;	// 서버 포트
const UINT16	MAX_CLIENT	= 100;		// 접속 가능한 클라이언트의 수

int main()
{
	IOCompletionPort iocp;

	iocp.InitSocket();
	iocp.BindAndListen(SERVER_PORT);
	iocp.StartServer(MAX_CLIENT);

	std::cout << "Wait for press any key..." << '\n';
	getchar();

	iocp.DestroyThread();

	return 0;
}