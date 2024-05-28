#include "EchoServer.h"
#include <iostream>

_bool EchoServer::Run(const UINT32 uiMaxClient)
{
	m_bIsProcessRun = true;
	m_processThread = std::thread([this]() { _ProcessPacket(); });

	return StartServer(uiMaxClient);
}

void EchoServer::End()
{
	m_bIsProcessRun = false;

	if (m_processThread.joinable())
		m_processThread.join();

	DestroyThread();
}

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
	RawPacketData packet;
	packet.Set(uiClientIndex, uiSize, pData);

	std::lock_guard<std::mutex> guard(m_mutex);
	m_packetDataQueue.push_back(packet);

	std::cout << "[OnReceive] Index : " << uiClientIndex << ", Data size : " << uiSize << '\n';
}

void EchoServer::_ProcessPacket()
{
	while (m_bIsProcessRun)
	{
		auto packetData = _DequePacketData();
		
		if (0 != packetData.m_uiDataSize)
			SendMsg(packetData);
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

RawPacketData EchoServer::_DequePacketData()
{
	std::lock_guard<std::mutex> guard(m_mutex);

	if (m_packetDataQueue.empty())
		return RawPacketData();

	RawPacketData packetData;
	packetData.Set(m_packetDataQueue.front());

	m_packetDataQueue.front().Release();
	m_packetDataQueue.pop_front();

	return packetData;
}