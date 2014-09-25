#include "stdafx.h"
#include "Exception.h"
#include "EduClient_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"
#include "PlayerManager.h"
#include "PacketHandler.h"

__declspec(thread) xdeque<ClientSession*>::type* LSendRequestSessionList = nullptr;

OverlappedIOContext::OverlappedIOContext(ClientSession* owner, IOType ioType) 
: mSessionObject(owner), mIoType(ioType)
{
	memset(&mOverlapped, 0, sizeof(OVERLAPPED));
	memset(&mWsaBuf, 0, sizeof(WSABUF));
	mSessionObject->AddRef();
}

ClientSession::ClientSession() : mSendBuffer( BUFSIZE ), mRecvBuffer( BUFSIZE ), mConnected( 0 ), mRefCount( 0 ), mPlayer( this ), mSendPendingCount( 0 )
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	mClientAddr.sin_family = AF_INET;
	mClientAddr.sin_addr.s_addr = INADDR_ANY;
	mClientAddr.sin_port = 0;
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}


void ClientSession::SessionReset()
{
	mConnected = 0;
	mRefCount = 0;

	memset( &mClientAddr, 0, sizeof( SOCKADDR_IN ) );

	mRecvBuffer.BufferReset();

	mSendBufferLock.EnterWriteLock();
	mSendBuffer.BufferReset();
	mSendBufferLock.LeaveWriteLock();

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof( LINGER ) ) )
	{
		printf_s( "[DEBUG] setsockopt linger option error: %d\n", GetLastError() );
	}
	closesocket( mSocket );

	mSocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );

	mPlayer.PlayerReset();
}

bool ClientSession::PostConnect()
{
	if( mConnected )
	{
		/// 이미 접속 된 상태인데 이쪽으로 들어오면 잘못된 것
		CRASH_ASSERT( false );
		return false;
	}

	if( SOCKET_ERROR == bind( mSocket, (SOCKADDR*)&mClientAddr, sizeof( mClientAddr ) ) )
	{
		printf_s( "ClientSession::PostConnect() bind error: %d\n", GetLastError() );
		return false;
	}

	HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), (ULONG_PTR)this, 0 );
	if( handle != GIocpManager->GetComletionPort() )
	{
		printf_s( "ClientSession::PostConnect() CreateIoCompletionPort error: %d\n", GetLastError() );
		return false;
	}


	OverlappedConnectContext* context = new OverlappedConnectContext( this );

	if( FALSE == ConnectEx( mSocket, (sockaddr*)GIocpManager->GetServerAddr(), sizeof( SOCKADDR_IN ), NULL, 0, NULL, (LPWSAOVERLAPPED)context ) )
	{
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( context );
			printf_s( "ClientSession::PostConnect Error : %d\n", GetLastError() );
		}
	}

	return true;
}

void ClientSession::ConnectCompletion()
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);
	
	if( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0 ) )
	{
		DWORD errCode = GetLastError();

		if( WSAENOTCONN == errCode )
			printf_s( "Connecting a server failed: maybe WSAENOTCONN??\n" );
		else
			printf_s( "SO_UPDATE_CONNECT_CONTEXT failed: %d\n", errCode );

		return;
	}

	int opt = 1;
	if( SOCKET_ERROR == setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof( int ) ) )
	{
		printf_s( "[DEBUG] TCP_NODELAY error: %d\n", GetLastError() );
		CRASH_ASSERT( false );
		return;
	}

	opt = 0;
	if( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof( int ) ) )
	{
		printf_s( "[DEBUG] SO_RCVBUF change error: %d\n", GetLastError() );
		CRASH_ASSERT( false );
		return;
	}


	if( 1 == InterlockedExchange( &mConnected, 1 ) )
	{
		CRASH_ASSERT( false );
	}

	if( false == PreRecv() )
	{
		printf_s( "[DEBUG] PreRecv for Server Connection error: %d\n", GetLastError() );
		InterlockedExchange( &mConnected, 0 );
		return;
	}

	printf_s( "[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa( GIocpManager->GetServerAddr()->sin_addr ), ntohs( GIocpManager->GetServerAddr()->sin_port ) );

	//for test
	int pid = GPlayerManager->GetUserIdCount();
	mPlayer.RequestLoad( pid );
}


void ClientSession::DisconnectRequest(DisconnectReason dr)
{
	/// 이미 끊겼거나 끊기는 중이거나
	if (0 == InterlockedExchange(&mConnected, 0))
		return ;
	
	OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

	if (FALSE == DisconnectEx(mSocket, (LPWSAOVERLAPPED)context, TF_REUSE_SOCKET, 0))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(context);
			printf_s("ClientSession::DisconnectRequest Error : %d\n", GetLastError());
		}
	}
}

void ClientSession::DisconnectCompletion(DisconnectReason dr)
{
	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	/// release refcount when added at issuing a session
	ReleaseRef();
}


bool ClientSession::PreRecv()
{
	if (!IsConnected())
		return false;

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = 0;
	recvContext->mWsaBuf.buf = nullptr;

	/// start async recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(recvContext);
			printf_s("ClientSession::PreRecv Error : %d\n", GetLastError());
			return false;
		}
	}

	return true;
}

bool ClientSession::PostRecv()
{
	if( !IsConnected() )
		return false;

	if( 0 == mRecvBuffer.GetFreeSpaceSize() )
		return false;

	OverlappedRecvContext* recvContext = new OverlappedRecvContext( this );

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = (ULONG)mRecvBuffer.GetFreeSpaceSize();
	recvContext->mWsaBuf.buf = mRecvBuffer.GetBuffer();


	/// start real recv
	if( SOCKET_ERROR == WSARecv( mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL ) )
	{
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( recvContext );
			printf_s( "Session::PostRecv Error : %d\n", GetLastError() );
			return false;
		}

	}

	return true;
}

void ClientSession::RecvCompletion(DWORD transferred)
{
	mRecvBuffer.Commit(transferred);

	GPacketHandler->RecvPacketProcess( *this, reinterpret_cast<const unsigned char*>(mRecvBuffer.GetBufferStart()), mRecvBuffer.GetContiguiousBytes() );

	mRecvBuffer.Remove( transferred );
}

bool ClientSession::PostSend( const char* data, size_t len )
{
	if( !IsConnected() )
		return false;

	FastSpinlockGuard criticalSection( mSendBufferLock );

	if( mSendBuffer.GetFreeSpaceSize() < len )
		return false;

	/// flush later...
	LSendRequestSessionList->push_back( this );

	char* destData = mSendBuffer.GetBuffer();

	memcpy( destData, data, len );

	mSendBuffer.Commit( len );

	return true;
}

bool ClientSession::FlushSend()
{
	if( !IsConnected() )
	{
		DisconnectRequest( DR_SENDFLUSH_ERROR );
		return true;
	}


	FastSpinlockGuard criticalSection( mSendBufferLock );

	/// 보낼 데이터가 없는 경우
	if( 0 == mSendBuffer.GetContiguiousBytes() )
	{
		/// 보낼 데이터도 없는 경우
		if( 0 == mSendPendingCount )
			return true;

		return false;
	}

	/// 이전의 send가 완료 안된 경우
	if( mSendPendingCount > 0 )
		return false;


	OverlappedSendContext* sendContext = new OverlappedSendContext( this );

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG)mSendBuffer.GetContiguiousBytes();
	sendContext->mWsaBuf.buf = mSendBuffer.GetBufferStart();

	/// start async send
	if( SOCKET_ERROR == WSASend( mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL ) )
	{
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( sendContext );
			printf_s( "Session::FlushSend Error : %d\n", GetLastError() );

			DisconnectRequest( DR_SENDFLUSH_ERROR );
			return true;
		}

	}

	mSendPendingCount++;

	return mSendPendingCount == 1;
}

void ClientSession::SendCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(mSendBufferLock);
	
	mSendBuffer.Remove(transferred);

	mSendPendingCount--;
}


void ClientSession::AddRef()
{
	CRASH_ASSERT(InterlockedIncrement(&mRefCount) > 0);
}

void ClientSession::ReleaseRef()
{
	long ret = InterlockedDecrement(&mRefCount);
	CRASH_ASSERT(ret >= 0);
	
	if (ret == 0)
	{
		GSessionManager->ReturnClientSession(this);
	}
}


void DeleteIoContext(OverlappedIOContext* context)
{
	if (nullptr == context)
		return;

	context->mSessionObject->ReleaseRef();

	/// ObjectPool's operate delete dispatch
	switch (context->mIoType)
	{
	case IO_SEND:
		delete static_cast<OverlappedSendContext*>(context);
		break;

	case IO_RECV_ZERO:
		delete static_cast<OverlappedPreRecvContext*>(context);
		break;

	case IO_RECV:
		delete static_cast<OverlappedRecvContext*>(context);
		break;

	case IO_DISCONNECT:
		delete static_cast<OverlappedDisconnectContext*>(context);
		break;

	case IO_ACCEPT:
		delete static_cast<OverlappedAcceptContext*>(context);
		break;

	case IO_CONNECT:
		delete static_cast<OverlappedConnectContext*>(context);
		break;

	default:
		CRASH_ASSERT(false);
	}

	
}

