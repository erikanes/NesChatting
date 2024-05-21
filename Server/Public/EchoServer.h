#pragma once

#include "IOCPServer.h"

#include <deque>

class EchoServer : public IOCPServer
{
public:
	EchoServer() = default;
	virtual ~EchoServer() = default;

public:
	virtual _bool	Run(const UINT32 uiMaxClient) override;
	virtual void	End() override;

	virtual void	OnConnect(const UINT32 uiClientIndex) override;
	virtual void	OnClose(const UINT32 uiClientIndex) override;
	virtual void	OnReceive(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData) override;

private:
	void _ProcessPacket();
	PacketData _DequePacketData();

private:
	_bool m_bIsProcessRun = false;
	std::thread m_processThread;

	std::mutex m_mutex;
	std::deque<PacketData> m_packetDataQueue;
};