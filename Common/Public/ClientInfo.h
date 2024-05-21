#pragma once

#include "Common_Define.h"
#include <mutex>
#include <queue>

class ClientInfo
{
public:
	ClientInfo();
	~ClientInfo() = default;

public:
	void Init(const UINT32 iIndex, HANDLE iocpHandle);
	UINT32 GetIndex() const { return m_iIndex; }
	_bool IsConnected() const { return m_bIsConnected; }
	SOCKET GetSocket() const { return m_socket; }
	UINT64 GetLatestClosedTimeSec() const { return m_uiLatestClosedTimeSec; }
	_char* GetRecvBuffer() { return m_recvBuf; }

	_bool OnConnect(HANDLE iocpHandle, SOCKET sock);
	void Close(_bool bIsForce = false);
	void Clear();
	_bool BindIOCP(HANDLE iocpHandle);
	_bool BindRecv();
	_bool SendMsg(const UINT32 uiDataSize, _char* pMsg);
	void SendCompleted(const UINT32 uiDataSize);

	_bool PostAccept(SOCKET listenSock, const UINT64 uiCurTimeSec);
	_bool AcceptCompletion();


private:
	_bool _SendIO();
	_bool _SetSocketOption();

private:
	INT32						m_iIndex = 0;
	SOCKET						m_socket = INVALID_SOCKET;
	HANDLE						m_iocpHandle = INVALID_HANDLE_VALUE;

	_bool						m_bIsConnected = false;
	UINT64						m_uiLatestClosedTimeSec = 0;

	OverlappedEx				m_stAcceptContext;
	_char						m_acceptBuf[MAX_ACCEPTBUF];

	OverlappedEx				m_stRecvOverlappedEx;
	_char						m_recvBuf[MAX_SOCK_RECVBUF];
	
	std::mutex					m_sendLock;
	std::queue<OverlappedEx*>	m_sendDataQueue;
};