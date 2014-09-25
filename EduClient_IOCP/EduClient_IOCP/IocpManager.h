#pragma once
#include "XTL.h"

class ClientSession;

struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedAcceptContext;

class IocpManager
{
public:
	IocpManager();
	~IocpManager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	void StartConnects(int num);


	HANDLE GetComletionPort()	{ return mCompletionPort; }
	int	GetIoThreadCount()		{ return mIoThreadCount;  }

	const SOCKADDR_IN* GetServerAddr() const { return &mServerAddr; }

	static char mAcceptBuf[64];
	static LPFN_DISCONNECTEX mFnDisconnectEx;
	static LPFN_CONNECTEX mFnConnectEx;

private:

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);
	void DoIocpJob();
	void DoSendJob();

private:

	HANDLE	mCompletionPort;
	int		mIoThreadCount;
	xvector<HANDLE>::type	mThreadHandleList;

	SOCKET	mTmpSocket;
	SOCKADDR_IN mServerAddr;
};

extern __declspec(thread) int LIoThreadId;
extern __declspec(thread) int64_t LTickCount;
extern IocpManager* GIocpManager;
extern long GSengJobThreadCount;

BOOL DisconnectEx(SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved);

BOOL ConnectEx( SOCKET hSocket, const struct sockaddr *name, int namelen, PVOID lpSendBuffer, 
	DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped );
