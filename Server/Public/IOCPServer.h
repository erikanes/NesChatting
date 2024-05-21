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
	_bool	InitSocket(const UINT32 uiMaxIOWorkerThreadCount); // 소켓 초기화
	_bool	BindAndListen(_int iBindPort); // 서버용 함수. 소켓과 연결시키고 접속 요청을 받기 위해 소켓을 등록

	_bool	SendMsg(const UINT32 uiSessionIndex, const UINT32 uiDataSize, _char* pData); //WSASend Overlapped I/O
	_bool	SendMsg(PacketData& packetData);

protected:
	_bool	StartServer(const UINT32 uiMaxClientCount); // 접속 요청을 수락하고 메시지를 받아서 처리
	void	DestroyThread(); // 생성되어있는 스레드 파괴

public:
	virtual _bool	Run(const UINT32 uiMaxClientCount);
	virtual void	End();

	// 네트워크 이벤트 처리 함수
	virtual void	OnConnect(const UINT32 uiClientIndex) {}
	virtual void	OnClose(const UINT32 uiClientIndex) {}
	virtual void	OnReceive(const UINT32 uiClientIndex, const UINT32 uiSize, _char* pData) {}

private:
	void		_CreateClient(const UINT32 iMaxClientCount);
	_bool		_CreateWorkerThread(); // WatingThreadQueue에서 대기할 스레드들 생성
	_bool		_CreateAccepterThread(); // accept 요청을 처리하는 스레드 생성

	ClientInfo* _GetEmptyClientInfo(); // 사용하지 않는 클라이언트 구조체를 반환
	ClientInfo* _GetClientInfo(const UINT32 uiSessionIndex);
	
	void		_WorkerThread(); // Overlapped I/O 작업 완료를 통보받아 그에 해당하는 처리를 수행
	void		_AcceptThread(); // 사용자의 접속을 받는 스레드
	void		_CloseSocket(ClientInfo* pClientInfo, _bool bIsForce = false); // 소켓 연결 종료

private:
	UINT32						m_uiMaxIOWorkerThreadCount = 0;

	std::vector<ClientInfo*>	m_clientInfos;							// 클라이언트 정보 저장 구조체
	SOCKET						m_socketListen = INVALID_SOCKET;		// 클라이언트의 접속을 받기위한 소켓
	_int						m_iClientCount = 0;						// 접속 되어있는 클라이언트 수

	std::vector<std::thread>	m_IOWorkerThreads;						// IO Worker 스레드
	std::thread					m_acceptThread;							// Accept 스레드
	HANDLE						m_IOCPHandle = INVALID_HANDLE_VALUE;	// CP 객체 핸들

	_bool						m_bIsWorkerRun		= false;				// 작업 스레드 동작 플래그
	_bool						m_bIsAccepterRun	= false;				// 접속 스레드 동작 플래그
};