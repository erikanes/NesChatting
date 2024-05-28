#include "User.h"

User::~User()
{
	if (nullptr != m_packetDataBuffer)
	{
		delete m_packetDataBuffer;
		m_packetDataBuffer = nullptr;
	}
}

void User::Init(const UINT32 uiIndex)
{
	m_iIndex = uiIndex;
	m_packetDataBuffer = new _char[PACKET_DATA_BUFFER_SIZE];
}

void User::Clear()
{
	m_iRoomIndex = -1;
	m_bIsConfirm = false;
	m_eCurDomainState = DOMAIN_STATE::NONE;

	m_uiPacketDataBufferRPos = 0;
	m_uiPacketDataBufferWPos = 0;

	m_strUserID.clear();
}

void User::EnterRoom(INT32 iRoomIndex)
{
	m_iRoomIndex = iRoomIndex;
	m_eCurDomainState = DOMAIN_STATE::ROOM;
}

PacketInfo User::GetPacket()
{
	constexpr int PACKET_SIZE_LENGTH = 2;
	constexpr int PACKET_TYPE_LENGTH = 2;
	_short iPacketSize = 0;
	const UINT32 iRemainByte = m_uiPacketDataBufferWPos - m_uiPacketDataBufferRPos;

	if (iRemainByte < PACKET_HEADER_LENGTH)
		return PacketInfo();

	auto pHeader = (PACKET_HEADER*)&m_packetDataBuffer[m_uiPacketDataBufferRPos];

	if (pHeader->uiPacketLength > iRemainByte)
		return PacketInfo();

	PacketInfo packetInfo;
	packetInfo.uiPacketID = pHeader->uiPacketID;
	packetInfo.uiDataSize = pHeader->uiPacketLength;
	packetInfo.pDataPtr = &m_packetDataBuffer[m_uiPacketDataBufferRPos];

	m_uiPacketDataBufferRPos += pHeader->uiPacketLength;

	return packetInfo;
}

_int User::SetLogin(_char* userID)
{
	m_eCurDomainState = DOMAIN_STATE::LOGIN;
	m_strUserID = userID;

	return 0;
}

void User::SetPacketData(const UINT32 uiDataSize, _char* pData)
{
	if ((m_uiPacketDataBufferWPos + uiDataSize) >= PACKET_DATA_BUFFER_SIZE)
	{
		auto iRemainDataSize = m_uiPacketDataBufferWPos - m_uiPacketDataBufferRPos;

		if (iRemainDataSize > 0)
		{
			CopyMemory(m_packetDataBuffer, &m_packetDataBuffer[m_uiPacketDataBufferRPos], iRemainDataSize);
			m_uiPacketDataBufferWPos = iRemainDataSize;
		}
		else
			m_uiPacketDataBufferWPos = 0;

		m_uiPacketDataBufferRPos = 0;
	}

	CopyMemory(&m_packetDataBuffer[m_uiPacketDataBufferWPos], pData, uiDataSize);
	m_uiPacketDataBufferWPos += uiDataSize;
}