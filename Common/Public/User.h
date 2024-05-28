#pragma once

#include <string>
#include "Packet.h"

class User
{
private:
	const UINT32 PACKET_DATA_BUFFER_SIZE = 8096;

public:
	enum class DOMAIN_STATE
	{
		NONE = 0,
		LOGIN = 1,
		ROOM = 2
	};

public:
	User() = default;
	~User();

	void Init(const UINT32 uiIndex);
	void Clear();

	void EnterRoom(INT32 iRoomIndex);

#pragma region Getter
	PacketInfo GetPacket();

	INT32 GetCurrentRoom() const { return m_iRoomIndex; }
	INT32 GetNetConnIndex() const { return m_iIndex; }
	const std::string& GetUserID() const { return m_strUserID; }
	DOMAIN_STATE GetDomainState() const { return m_eCurDomainState; }

#pragma endregion

#pragma region Setter
	_int SetLogin(_char* userID);
	void SetPacketData(const UINT32 uiDataSize, _char* pData);

	void SetDomainState(DOMAIN_STATE value) { m_eCurDomainState = value; }
#pragma endregion

private:
	INT32 m_iIndex = -1;
	INT32 m_iRoomIndex = -1;

	std::string m_strUserID;
	std::string m_strAutoToken;

	_bool m_bIsConfirm = false;

	DOMAIN_STATE m_eCurDomainState = DOMAIN_STATE::NONE;

	UINT32 m_uiPacketDataBufferWPos = 0;
	UINT32 m_uiPacketDataBufferRPos = 0;
	_char* m_packetDataBuffer = nullptr;

};