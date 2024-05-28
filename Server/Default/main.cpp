#include "ChatServer.h"
#include <iostream>
#include <string>

#include "Common_Debug.h"

const UINT16	SERVER_PORT = 11021;	// 서버 포트
const UINT16	MAX_CLIENT = 3;		// 접속 가능한 클라이언트의 수
const UINT32	MAX_IO_WORKER_THREAD = 8;

int main()
{
	_CRTSETDBGFLAG_

	ChatServer server;

	if (false == server.InitSocket(MAX_IO_WORKER_THREAD))
		return 0;

	if (false == server.BindAndListen(SERVER_PORT))
		return 0;

	if (false == server.Run(MAX_CLIENT))
		return 0;

	std::cout << "Press any key..." << '\n';

	while (true)
	{
		std::string strInputCmd;
		std::getline(std::cin, strInputCmd);

		if (0 == strInputCmd.compare("quit"))
			break;
	}

	server.End();

	return 0;
}