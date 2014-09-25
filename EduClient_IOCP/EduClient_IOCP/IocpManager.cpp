#include "stdafx.h"
#include "Exception.h"
#include "IocpManager.h"
#include "EduClient_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "Timer.h"

#define GQCS_TIMEOUT	20

__declspec(thread) int LIoThreadId = 0;
__declspec(thread) int64_t LTickCount = 0;
IocpManager* GIocpManager = nullptr;
long GSendJobThreadCount = 0;

LPFN_DISCONNECTEX IocpManager::mFnDisconnectEx = nullptr;
LPFN_CONNECTEX IocpManager::mFnConnectEx = nullptr;

char IocpManager::mAcceptBuf[64] = { 0, };


BOOL DisconnectEx(SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved)
{
	return IocpManager::mFnDisconnectEx(hSocket, lpOverlapped, dwFlags, reserved);
}

BOOL ConnectEx( SOCKET hSocket, const struct sockaddr *name, int namelen, PVOID lpSendBuffer, 
	DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped ){
	return IocpManager::mFnConnectEx( hSocket, name, namelen, lpSendBuffer, dwSendDataLength, 
		lpdwBytesSent, lpOverlapped );
};

IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2)
{	
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	/// set num of I/O threads
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	mIoThreadCount = si.dwNumberOfProcessors;

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (mCompletionPort == NULL)
		return false;

	// Set Server Addr
	ZeroMemory(&mServerAddr, sizeof(mServerAddr));
	mServerAddr.sin_family = AF_INET;
	mServerAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	mServerAddr.sin_port = htons( SERVER_PORT );

	mTmpSocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
	if( mTmpSocket == INVALID_SOCKET )
		return false;

	GUID guidDisconnectEx = WSAID_DISCONNECTEX ;
	DWORD bytes = 0 ;
	if( SOCKET_ERROR == WSAIoctl( mTmpSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidDisconnectEx, sizeof(GUID), &mFnDisconnectEx, sizeof(LPFN_DISCONNECTEX), &bytes, NULL, NULL) )
		return false;

	GUID guidConnectEx = WSAID_CONNECTEX ;
	if( SOCKET_ERROR == WSAIoctl( mTmpSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidConnectEx, sizeof( GUID ), &mFnConnectEx, sizeof( LPFN_CONNECTEX ), &bytes, NULL, NULL ) )
		return false;
	
	/// make session pool
	GSessionManager->PrepareSessions();

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (LPVOID)(i+1), 0, (unsigned int*)&dwThreadId);
		if (hThread == NULL)
			return false;
		mThreadHandleList.push_back( hThread );
	}

	return true;
}


void IocpManager::StartConnects(int num)
{
	int64_t startTick = GetTickCount64();
	int64_t curTick = startTick;

	while (GSessionManager->ConnectSessions())
	{
		Sleep(100);

		curTick = GetTickCount64();
		if( curTick - startTick > CONNECT_TIME )
			return;
	}
}


void IocpManager::Finalize()
{
	CloseHandle(mCompletionPort);

	/// winsock finalizing
	WSACleanup();

}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;

	LIoThreadId = reinterpret_cast<int>(lpParam);
	LSendRequestSessionList = new xdeque<ClientSession*>::type();
	LTimer = new Timer;

	std::srand( GetTickCount() + LIoThreadId );

	while (true)
	{
		LTimer->DoTimerJob();

		GIocpManager->DoIocpJob();

		if( InterlockedIncrement( &GSendJobThreadCount ) < GIocpManager->mIoThreadCount )
			GIocpManager->DoSendJob();
		InterlockedDecrement( &GSendJobThreadCount );
	}

	delete LSendRequestSessionList;
	return 0;
}

void IocpManager::DoIocpJob()
{
	DWORD dwTransferred = 0;
	LPOVERLAPPED overlapped = nullptr;

	ULONG_PTR completionKey = 0;

	int ret = GetQueuedCompletionStatus( mCompletionPort, &dwTransferred, (PULONG_PTR)&completionKey, &overlapped, GQCS_TIMEOUT );

	OverlappedIOContext* context = reinterpret_cast<OverlappedIOContext*>(overlapped);

	ClientSession* remote = context ? context->mSessionObject : nullptr;

	if( ret == 0 || dwTransferred == 0 )
	{
		int gle = GetLastError();
		/// check time out first 
		if( context == nullptr && gle == WAIT_TIMEOUT )
			return;

		if( gle == ERROR_ABANDONED_WAIT_0 || gle == ERROR_INVALID_HANDLE ){ // 무언가의 이유로 IOCP가 종료. 보통 프로그램 종료중일듯.
			DWORD dwExitCode;
			GetExitCodeThread( mThreadHandleList[LIoThreadId-1], &dwExitCode );
			ExitThread( dwExitCode );
		}

		if( context->mIoType == IO_RECV || context->mIoType == IO_SEND )
		{
			CRASH_ASSERT( nullptr != remote );

			/// In most cases in here: ERROR_NETNAME_DELETED(64)

			remote->DisconnectRequest( DR_COMPLETION_ERROR );

			DeleteIoContext( context );

			return;
		}
	}

	CRASH_ASSERT( nullptr != remote );

	bool completionOk = false;
	switch( context->mIoType )
	{
	case IO_DISCONNECT:
		remote->DisconnectCompletion( static_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason );
		completionOk = true;
		break;

	case IO_CONNECT:
		remote->ConnectCompletion();
		completionOk = true;
		break;

	case IO_RECV_ZERO:
		completionOk = remote->PostRecv();
		break;

	case IO_SEND:
		remote->SendCompletion( dwTransferred );

		if( context->mWsaBuf.len != dwTransferred )
			printf_s( "Partial SendCompletion requested [%d], sent [%d]\n", context->mWsaBuf.len, dwTransferred );
		else
			completionOk = true;

		break;

	case IO_RECV:
		remote->RecvCompletion( dwTransferred );

		completionOk = remote->PreRecv();

		break;

	default:
		printf_s( "Unknown I/O Type: %d\n", context->mIoType );
		CRASH_ASSERT( false );
		break;
	}

	if( !completionOk )
	{
		/// connection closing
		remote->DisconnectRequest( DR_IO_REQUEST_ERROR );
	}

	DeleteIoContext( context );
}

void IocpManager::DoSendJob()
{
	
	while( !LSendRequestSessionList->empty() )
	{
		auto& session = LSendRequestSessionList->front();

		if( session->FlushSend() )
		{
			/// true 리턴 되면 빼버린다.
			LSendRequestSessionList->pop_front();
		}
	}
	
}
