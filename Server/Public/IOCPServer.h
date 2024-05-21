#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock")
#pragma comment(lib, "Common")

#include "Common_Define.h"
#include "ClientInfo.h"
#include "Packet.h"

#include <thread>
#include <vector>

class IOCPServer
{
public:
	IOCPServer() = default;
	virtual ~IOCPServer();

public:
	_bool	InitSocket(const UINT32 uiMaxIOWorkerThreadCount); // ���� �ʱ�ȭ
	_bool	BindAndListen(_int iBindPort); // ������ �Լ�. ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� ������ ���

	_bool	SendMsg(const UINT32 uiSessionIndex, const UINT32 uiDataSize, _char* pData); //WSASend Overlapped I/O
	_bool	SendMsg(PacketData& packetData);

protected:
	_bool	StartServer(const UINT32 uiMaxClientCount); // ���� ��û�� �����ϰ� �޽����� �޾Ƽ� ó��
	void	DestroyThread(); // �����Ǿ��ִ� ������ �ı�

public:
	virtual _bool	Run(const UINT32 uiMaxClientCount);
	virtual void	End();

	// ��Ʈ��ũ �̺�Ʈ ó�� �Լ�
	virtual void	OnConnect(const UINT32 uiClientIndex) {}
	virtual void	OnClose(const UINT32 uiClientIndex) {}
	virtual void	OnReceive(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData) {}

private:
	void		_CreateClient(const UINT32 iMaxClientCount);
	_bool		_CreateWorkerThread(); // WatingThreadQueue���� ����� ������� ����
	_bool		_CreateAccepterThread(); // accept ��û�� ó���ϴ� ������ ����

	ClientInfo* _GetEmptyClientInfo(); // ������� �ʴ� Ŭ���̾�Ʈ ����ü�� ��ȯ
	ClientInfo* _GetClientInfo(const UINT32 uiSessionIndex);
	
	void		_WorkerThread(); // Overlapped I/O �۾� �ϷḦ �뺸�޾� �׿� �ش��ϴ� ó���� ����
	void		_AcceptThread(); // ������� ������ �޴� ������
	void		_CloseSocket(ClientInfo* pClientInfo, _bool bIsForce = false); // ���� ���� ����

private:
	UINT32						m_uiMaxIOWorkerThreadCount = 0;

	std::vector<ClientInfo*>	m_clientInfos;							// Ŭ���̾�Ʈ ���� ���� ����ü
	SOCKET						m_socketListen = INVALID_SOCKET;		// Ŭ���̾�Ʈ�� ������ �ޱ����� ����
	_int						m_iClientCount = 0;						// ���� �Ǿ��ִ� Ŭ���̾�Ʈ ��

	std::vector<std::thread>	m_IOWorkerThreads;						// IO Worker ������
	std::thread					m_acceptThread;							// Accept ������
	HANDLE						m_IOCPHandle = INVALID_HANDLE_VALUE;	// CP ��ü �ڵ�

	_bool						m_bIsWorkerRun		= false;				// �۾� ������ ���� �÷���
	_bool						m_bIsAccepterRun	= false;				// ���� ������ ���� �÷���
};