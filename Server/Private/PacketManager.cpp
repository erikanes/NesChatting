#include "PacketManager.h"
#include "UserManager.h"

#include <iostream>
#include <utility>
#include <cstring>

PacketManager::PacketManager()
{
}

PacketManager::~PacketManager()
{
	m_pUserManager.reset();
}

void PacketManager::Init(const UINT32 uiMaxClient, std::function<void(UINT32, UINT32, _char*)> sendPacketFunc)
{
	m_recvFunctionDictionary = std::unordered_map<_int, PROCESS_RECV_PACKET_FUNCTION>();

	m_recvFunctionDictionary[(_int)PACKET_ID::SYS_USER_CONNECT] = &PacketManager::_ProcessUserConnect;
	m_recvFunctionDictionary[(_int)PACKET_ID::SYS_USER_DISCONNECT] = &PacketManager::_ProcessUserDisconnect;
	
	m_recvFunctionDictionary[(_int)PACKET_ID::LOGIN_REQUEST] = &PacketManager::_ProcessLogin;

	m_sendPacketFunc = sendPacketFunc;

	_CreateComponent(uiMaxClient);
}

_bool PacketManager::Run()
{
	m_bIsRunProcessThread = true;
	m_processThread = std::thread([this]() { _ProcessPacket(); });

	return true;
}

void PacketManager::End()
{
	m_bIsRunProcessThread = false;

	if (m_processThread.joinable())
		m_processThread.join();
}

void PacketManager::ReceivePacketData(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData)
{
	auto pUser = m_pUserManager->GetUserByConnIndex(uiClientIndex);
	if (nullptr == pUser)
		return;

	pUser->SetPacketData(uiSize, pData);
	_EnqueuePacketData(uiClientIndex);
}

void PacketManager::PushSystemPacket(const PacketInfo& packet)
{
	std::lock_guard<std::mutex> guard(m_lock);
	m_systemPacketQueue.push_back(packet);
}

void PacketManager::_CreateComponent(const UINT32 uiMaxClient)
{
	m_pUserManager = std::make_unique<UserManager>();
	m_pUserManager->Init(uiMaxClient);
}

void PacketManager::_ClearConnectionInfo(const UINT32 uiClientIndex)
{
	auto pRequestUser = m_pUserManager->GetUserByConnIndex(uiClientIndex);

	if (nullptr == pRequestUser)
		return;
	else if (User::DOMAIN_STATE::NONE != pRequestUser->GetDomainState())
		m_pUserManager->DeleteUserInfo(pRequestUser);
}

void PacketManager::_EnqueuePacketData(const UINT32 uiClientIndex)
{
	std::lock_guard<std::mutex> guard(m_lock);
	m_inComingPacketUserIndex.push_back(uiClientIndex);
}

PacketInfo PacketManager::_DequePacketData()
{
	UINT32 uiUserIndex = 0;

	{
		std::lock_guard<std::mutex> guard(m_lock);

		if (m_inComingPacketUserIndex.empty())
			return PacketInfo();

		uiUserIndex = m_inComingPacketUserIndex.front();
		m_inComingPacketUserIndex.pop_front();
	}

	auto pUser = m_pUserManager->GetUserByConnIndex(uiUserIndex);
	auto packetData = pUser->GetPacket();
	packetData.uiClientIndex = uiUserIndex;

	return packetData;
}

PacketInfo PacketManager::_DequeSystemPacketData()
{
	std::lock_guard<std::mutex> guard(m_lock);
	if (m_systemPacketQueue.empty())
		return PacketInfo();

	auto packetData = m_systemPacketQueue.front();
	m_systemPacketQueue.pop_front();

	return packetData;
}

void PacketManager::_ProcessPacket()
{
	while (m_bIsRunProcessThread)
	{
		_bool bIsIdle = true;

		if (auto packetData = _DequePacketData(); packetData.uiPacketID > (UINT16)PACKET_ID::SYS_END)
		{
			bIsIdle = false;
			_ProcessRecvPacket(packetData.uiClientIndex, packetData.uiPacketID, packetData.uiDataSize, packetData.pDataPtr);
		}

		if (auto packetData = _DequeSystemPacketData(); 0 != packetData.uiPacketID)
		{
			bIsIdle = false;
			_ProcessRecvPacket(packetData.uiClientIndex, packetData.uiPacketID, packetData.uiDataSize, packetData.pDataPtr);
		}

		if (bIsIdle)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void PacketManager::_ProcessRecvPacket(const UINT32 uiClientIndex, const UINT16 uiPacketID, const UINT16 uiPacketSize, _char* pPacket)
{
	auto iter = m_recvFunctionDictionary.find(uiPacketID);

	if (iter != m_recvFunctionDictionary.end())
		(this->*(iter->second))(uiClientIndex, uiPacketSize, pPacket);
}

void PacketManager::_ProcessUserConnect(const UINT32 uiClientIndex, const UINT16 uiPacketSize, _char* pPacket)
{
	std::cout << "[ProcessUserConnect] Client index : " << uiClientIndex << '\n';
	
	auto pUser = m_pUserManager->GetUserByConnIndex(uiClientIndex);
	if (nullptr == pUser)
		return;

	pUser->Clear();
}

void PacketManager::_ProcessUserDisconnect(const UINT32 uiClientIndex, const UINT16 uiPacketSize, _char* pPacket)
{
	std::cout << "[ProcessUserDisconnect] Client index : " << uiClientIndex << '\n';
	_ClearConnectionInfo(uiClientIndex);
}

void PacketManager::_ProcessLogin(const UINT32 uiClientIndex, const UINT16 uiPacketSize, _char* pPacket)
{
	if (LOGIN_REQUEST_PACKET_SIZE != uiPacketSize)
		return;

	auto pLoginRequestPacket = reinterpret_cast<LOGIN_REQUEST_PACKET*>(pPacket);
	if (nullptr == pPacket)
		return;

	auto pUserID = pLoginRequestPacket->userID;

	std::cout << "requested user id = " << pUserID << '\n';

	LOGIN_RESPONSE_PACKET loginResponsePacket;
	loginResponsePacket.uiPacketID = (UINT16)PACKET_ID::LOGIN_RESPONSE;
	loginResponsePacket.uiPacketLength = sizeof(LOGIN_RESPONSE_PACKET);

	if (m_pUserManager->GetCurrentUserCount() >= m_pUserManager->GetMaxUserCount())
	{
		// 최대 접속자수 초과
		loginResponsePacket.result = (UINT16)ERROR_CODE::LOGIN_USER_USED_ALL_OBJ;
		_SendPacket(uiClientIndex, sizeof(LOGIN_RESPONSE_PACKET), (_char*)&loginResponsePacket);
		return;
	}

	// 접속중인 유저인지 확인
	if (-1 == m_pUserManager->FindUserIndexByID(pUserID))
	{
		// 신규 접속
		loginResponsePacket.result = (UINT16)ERROR_CODE::NONE;
		_SendPacket(uiClientIndex, sizeof(LOGIN_RESPONSE_PACKET), (_char*)&loginResponsePacket);
		return;
	}
	else
	{
		// 이미 접속
		loginResponsePacket.result = (UINT16)ERROR_CODE::LOGIN_USER_ALREADY;
		_SendPacket(uiClientIndex, sizeof(LOGIN_RESPONSE_PACKET), (_char*)&loginResponsePacket);
		return;
	}
	
}

void PacketManager::_SendPacket(const UINT uiClientIndex, const UINT16 uiPacketSize, _char* pPacket)
{
	m_sendPacketFunc(uiClientIndex, uiPacketSize, pPacket);
}