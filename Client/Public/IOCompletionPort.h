#pragma once
#pragma comment(lib, "ws2_32")

#include "../Public/typedef.h"
#include "../Public/Defines.h"

#include <thread>
#include <vector>

class IOCompletionPort
{
public:
	IOCompletionPort() = default;
	~IOCompletionPort();

public:
	_bool InitSocket(); // ���� �ʱ�ȭ
	_bool BindAndListen(_int iBindPort); // ������ �Լ�. ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� ������ ���
	_bool StartServer(const UINT32 iMaxClientCount); // ���� ��û�� �����ϰ� �޽����� �޾Ƽ� ó��

	void DestroyThread(); // �����Ǿ��ִ� ������ �ı�

private:
	void _CreateClient(const UINT32 iMaxClientCount);
	_bool _CreateWorkerThread(); // WatingThreadQueue���� ����� ������� ����
	_bool _CreateAccepterThread(); // accept ��û�� ó���ϴ� ������ ����

	ClientInfo* _GetEmptyClientInfo(); // ������� �ʴ� Ŭ���̾�Ʈ ����ü�� ��ȯ
	_bool _BindIOCP(ClientInfo* pClientInfo); // CP��ü�� ����, CompletionKey�� ����
	_bool _BindRecv(ClientInfo* pClientInfo); // WSARecv Overlapped I/O
	_bool _SendMsg(ClientInfo* pClientInfo, _char* pMsg, _int iLen); //WSASend Overlapped I/O
	void _WorkerThread(); // Overlapped I/O �۾� �ϷḦ �뺸�޾� �׿� �ش��ϴ� ó���� ����
	void _AcceptThread(); // ������� ������ �޴� ������
	void _CloseSocket(ClientInfo* pClientInfo, _bool bIsForce = false); // ���� ���� ����

private:
	std::vector<ClientInfo>		m_clientInfos;							// Ŭ���̾�Ʈ ���� ���� ����ü
	SOCKET						m_socketListen = INVALID_SOCKET;		// Ŭ���̾�Ʈ�� ������ �ޱ����� ����
	_int						m_iClientCount = 0;						// ���� �Ǿ��ִ� Ŭ���̾�Ʈ ��

	std::vector<std::thread>	m_IOWorkerThreads;						// IO Worker ������
	std::thread					m_acceptThread;							// Accept ������
	HANDLE						m_IOCPHandle = INVALID_HANDLE_VALUE;	// CP ��ü �ڵ�

	_bool						m_bIsWorkerRun = true;					// �۾� ������ ���� �÷���
	_bool						m_bIsAcceptRun = true;					// ���� ������ ���� �÷���
	_char						m_szSocketBuf[MAX_SOCKBUF] = {};		// ���� ����
};