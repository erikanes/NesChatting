#pragma once

#include "IOCPServer.h"

class PacketManager;

class ChatServer : public IOCPServer
{
public:
	ChatServer();
	virtual ~ChatServer();

public:
	virtual void OnConnect(const UINT32 uiClientIndex) override;
	virtual void OnClose(const UINT32 uiClientIndex) override;
	virtual void OnReceive(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData) override;

	virtual _bool Run(const UINT32 uiMaxClientCount) override;
	virtual void End() override;

private:
	std::unique_ptr<PacketManager> m_pPacketManager;
};