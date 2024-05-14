#include "EchoServer.h"
#include <iostream>
#include <string>

#include "Common_Debug.h"

const UINT16	SERVER_PORT = 11021;	// ���� ��Ʈ
const UINT16	MAX_CLIENT = 100;		// ���� ������ Ŭ���̾�Ʈ�� ��

int main()
{
	_CRTSETDBGFLAG_

	EchoServer server;

	if (false == server.InitSocket())
		return 0;

	if (false == server.BindAndListen(SERVER_PORT))
		return 0;

	if (false == server.Run(MAX_CLIENT))
		return 0;

	std::cout << "Press any key...";

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