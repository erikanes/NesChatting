#include "EchoServer.h"
#include <iostream>
#include <string>

#include "Common_Debug.h"

const UINT16	SERVER_PORT = 11021;	// 서버 포트
const UINT16	MAX_CLIENT = 100;		// 접속 가능한 클라이언트의 수

int main()
{
	_CRTSETDBGFLAG_

	EchoServer server;

	if (false == server.InitSocket())
		return 0;

	if (false == server.BindAndListen(SERVER_PORT))
		return 0;

	if (false == server.StartServer(MAX_CLIENT))
		return 0;

	std::cout << "Press any key...";

	while (true)
	{
		std::string strInputCmd;
		std::getline(std::cin, strInputCmd);

		if (0 == strInputCmd.compare("quit"))
			break;
	}

	server.DestroyThread();

	return 0;

	//IOCPServer iocp;

	//iocp.InitSocket();
	//iocp.BindAndListen(SERVER_PORT);
	//iocp.StartServer(MAX_CLIENT);

	//std::cout << "Wait for press any key..." << '\n';
	//getchar();

	//iocp.DestroyThread();

	return 0;
}