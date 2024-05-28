#include "ChatServer.h"
#include <iostream>
#include <string>

#include "Common_Debug.h"

const UINT16	SERVER_PORT = 11021;	// ���� ��Ʈ
const UINT16	MAX_CLIENT = 3;		// ���� ������ Ŭ���̾�Ʈ�� ��
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