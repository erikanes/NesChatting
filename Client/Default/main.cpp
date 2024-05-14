#include <iostream>
#include "IOCompletionPort.h"

const UINT16	SERVER_PORT	= 11021;	// ���� ��Ʈ
const UINT16	MAX_CLIENT	= 100;		// ���� ������ Ŭ���̾�Ʈ�� ��

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