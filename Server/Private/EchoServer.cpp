#include "EchoServer.h"
#include <iostream>

void EchoServer::OnConnect(const UINT32 uiClientIndex)
{
	std::cout << "[OnConnect] Index : " << uiClientIndex << '\n';
}

void EchoServer::OnClose(const UINT32 uiClientIndex)
{
	std::cout << "[OnClose] Index : " << uiClientIndex << '\n';
}

void EchoServer::OnReceive(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData)
{
	std::cout << "[OnReceive] Index : " << uiClientIndex << ", Data size : " << uiSize << '\n';
}