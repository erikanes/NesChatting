#pragma once

#include "Packet.h"

#include <unordered_map>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>

class UserManager;
//class RoomManager;
//class RedisManager;

class PacketManager
{
public:
	PacketManager();
	~PacketManager();

	void Init(const UINT32 uiMaxClient, std::function<void(UINT32, UINT32, _char*)> sendPacketFunc);
	_bool Run();
	void End();

	void ReceivePacketData(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData);
	void PushSystemPacket(const PacketInfo& packet);

private:
	void _CreateComponent(const UINT32 uiMaxClient);
	void _ClearConnectionInfo(const UINT32 uiClientIndex);
	
	void _EnqueuePacketData(const UINT32 uiClientIndex);
	PacketInfo _DequePacketData();
	PacketInfo _DequeSystemPacketData();

	void _ProcessPacket();
	void _ProcessRecvPacket(const UINT32 uiClientIndex, const UINT16 uiPacketID, const UINT16 uiPacketSize, _char* pPacket);
	void _ProcessUserConnect(const UINT32 uiClientIndex, const UINT16 uiPacketSize, _char* pPacket);
	void _ProcessUserDisconnect(const UINT32 uiClientIndex, const UINT16 uiPacketSize, _char* pPacket);
	void _ProcessLogin(const UINT32 uiClientIndex, const UINT16 uiPacketSize, _char* pPacket);

	void _SendPacket(const UINT uiClientIndex, const UINT16 uiPacketSize, _char* pPacket);

private:
	std::function<void(UINT32, UINT32, _char*)> m_sendPacketFunc;

	typedef void(PacketManager::* PROCESS_RECV_PACKET_FUNCTION)(UINT32, UINT16, _char*);
	std::unordered_map<_int, PROCESS_RECV_PACKET_FUNCTION> m_recvFunctionDictionary;

	std::unique_ptr<UserManager> m_pUserManager;

	std::function<void(_int, _char*)> m_sendMQDataFunc;

	_bool m_bIsRunProcessThread = false;
	std::thread m_processThread;
	std::mutex m_lock;

	std::deque<UINT32> m_inComingPacketUserIndex;
	std::deque<PacketInfo> m_systemPacketQueue;
};