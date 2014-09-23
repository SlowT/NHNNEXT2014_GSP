#pragma once

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

private:

	HANDLE	mCompletionPort;
	int		mIoThreadCount;

	SOCKET	mTmpSocket;
	SOCKADDR_IN mServerAddr;
};

extern __declspec(thread) int LIoThreadId;
extern IocpManager* GIocpManager;

BOOL DisconnectEx(SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved);

BOOL ConnectEx( SOCKET hSocket, const struct sockaddr *name, int namelen, PVOID lpSendBuffer, 
	DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped );
