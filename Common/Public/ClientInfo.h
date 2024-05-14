#pragma once

#include "Common_Define.h"

class ClientInfo
{
public:
	ClientInfo();
	~ClientInfo() = default;

public:
	void Init(const UINT32 iIndex);
	UINT32 GetIndex() const { return m_iIndex; }
	_bool IsConnected() const { return INVALID_SOCKET != m_socket; }
	SOCKET GetSocket() const { return m_socket; }
	_char* GetRecvBuffer() { return m_recvBuf; }

	_bool OnConnect(HANDLE iocpHandle, SOCKET sock);
	void Close(_bool bIsForce = false);
	void Clear();
	_bool BindIOCP(HANDLE iocpHandle);
	_bool BindRecv();
	_bool SendMsg(const UINT32 uiDataSize, _char* pMsg);
	void SendCompleted(const UINT32 uiDataSize);

private:
	UINT32 m_iIndex = 0;
	SOCKET m_socket = INVALID_SOCKET;

	OverlappedEx m_stRecvOverlappedEx;
	_char m_recvBuf[MAX_SOCKBUF];
};