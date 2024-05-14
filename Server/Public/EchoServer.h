#pragma once

#include "IOCPServer.h"

class EchoServer : public IOCPServer
{
public:
	EchoServer() = default;
	virtual ~EchoServer() = default;

public:
	virtual void OnConnect(const UINT32 uiClientIndex) override;
	virtual void OnClose(const UINT32 uiClientIndex) override;
	virtual void OnReceive(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData) override;
};