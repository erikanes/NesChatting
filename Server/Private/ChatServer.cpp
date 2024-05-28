#include "ChatServer.h"
#include "PacketManager.h"
#include <iostream>

ChatServer::ChatServer()
{
}

ChatServer::~ChatServer()
{
}

void ChatServer::OnConnect(const UINT32 uiClientIndex)
{
	std::cout << "[OnConnect] Client : Index(" << uiClientIndex << ")" << '\n';

	PacketInfo packet{ uiClientIndex, (UINT16)PACKET_ID::SYS_USER_CONNECT, 0 };
	m_pPacketManager->PushSystemPacket(packet);
}

void ChatServer::OnClose(const UINT32 uiClientIndex)
{
	std::cout << "[OnClose] Client : Index(" << uiClientIndex << ")" << '\n';

	PacketInfo packet{ uiClientIndex, (UINT16)PACKET_ID::SYS_USER_DISCONNECT, 0 };
	m_pPacketManager->PushSystemPacket(packet);
}

void ChatServer::OnReceive(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData)
{
	std::cout << "[OnReceive] Client : Index(" << uiClientIndex << "), dataSize("
		<< uiSize << ")" << '\n';

	m_pPacketManager->ReceivePacketData(uiClientIndex, uiSize, pData);
}

_bool ChatServer::Run(const UINT32 uiMaxClientCount)
{
	auto sendPacketFunc = [&](UINT32 uiClientIndex, UINT16 uiPacketSize, _char* pSendPacket)
	{
		return SendMsg(uiClientIndex, uiPacketSize, pSendPacket);
	};

	m_pPacketManager = std::make_unique<PacketManager>();
	m_pPacketManager->Init(uiMaxClientCount, sendPacketFunc);
	m_pPacketManager->Run();

	return StartServer(uiMaxClientCount);
}

void ChatServer::End()
{
	m_pPacketManager->End();

	DestroyThread();
}
