#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Common_Typedef.h"

struct RawPacketData
{
	void Set(RawPacketData& value)
	{
		Set(value.m_uiClientIndex, value.m_uiDataSize, value.m_pData);
	}

	void Set(const UINT32 uiClientIndex, const UINT32 uiDataSize, _char* pData)
	{
		m_uiClientIndex = uiClientIndex;
		m_uiDataSize = uiDataSize;

		m_pData = new _char[m_uiDataSize];
		CopyMemory(m_pData, pData, m_uiDataSize);
	}

	void Release()
	{
		delete m_pData;
	}

	UINT32 m_uiClientIndex = 0;
	UINT32 m_uiDataSize = 0;
	_char* m_pData = nullptr;
};

struct PacketInfo
{
	UINT32 uiClientIndex = 0;
	UINT16 uiPacketID = 0;
	UINT16 uiDataSize = 0;
	_char* pDataPtr = nullptr;
};

enum class PACKET_ID : UINT16
{
	// system
	SYS_USER_CONNECT = 11,
	SYS_USER_DISCONNECT = 12,
	SYS_END = 30,

	// db
	DB_END = 199,

	// client
	LOGIN_REQUEST = 201,
	LOGIN_RESPONSE = 202,

	ROOM_ENTER_REQUEST = 206,
	ROOM_ENTER_RESPONSE = 207,

	ROOM_LEAVE_REQUEST = 215,
	ROOM_LEAVE_RESPONSE = 216,

	ROOM_CHAT_REQUEST = 221,
	ROOM_CHAT_RESPONSE = 222,
	ROOM_CHAT_NOTIFY = 223
};

#pragma pack(push,1)
struct PACKET_HEADER
{
	UINT16	uiPacketLength;
	UINT16	uiPacketID;
	UINT8	uiType;
};

constexpr const UINT32 PACKET_HEADER_LENGTH = sizeof(PACKET_HEADER);

// request login
constexpr const _int MAX_USER_ID_LEN = 32;
constexpr const _int MAX_USER_PW_LEN = 32;

struct LOGIN_REQUEST_PACKET : public PACKET_HEADER
{
	_char userID[MAX_USER_ID_LEN + 1];
	_char userPW[MAX_USER_PW_LEN + 1];
};

constexpr const size_t LOGIN_REQUEST_PACKET_SIZE = sizeof(LOGIN_REQUEST_PACKET);

struct LOGIN_RESPONSE_PACKET : public PACKET_HEADER
{
	UINT16 result;
};

// request join the room
struct ROOM_ENTER_REQUEST_PACKET : public PACKET_HEADER
{
	INT32 roomNumber;
};

struct ROOM_ENTER_RESPONSE_PACKET : public PACKET_HEADER
{
	INT16 result;
};

// request leave the room
struct ROOM_LEAVE_REQUEST_PACKET : public PACKET_HEADER
{
};

struct ROOM_LEAVE_RESPONSE_PACKET : public PACKET_HEADER
{
	INT16 result;
};

// room chat
constexpr const _int MAX_CHAT_MSG_SIZE = 256;

struct ROOM_CHAT_REQUEST_PACKET : public PACKET_HEADER
{
	_char message[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};

struct ROOM_CHAT_RESPONSE_PACKET : public PACKET_HEADER
{
	INT16 result;
};

struct ROOM_CHAT_NOTIFY_PACKET : public PACKET_HEADER
{
	_char userID[MAX_USER_ID_LEN + 1] = { 0, };
	_char message[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};
#pragma pack(pop) // delete packing settings