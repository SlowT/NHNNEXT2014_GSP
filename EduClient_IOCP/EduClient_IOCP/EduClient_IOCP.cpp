// EduServer_IOCP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"
#include "EduClient_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"
#include "PlayerManager.h"


__declspec(thread) int LThreadType = -1;

int _tmain(int argc, _TCHAR* argv[])
{
	LThreadType = THREAD_MAIN;

	/// for dump on crash
	SetUnhandledExceptionFilter(ExceptionFilter);

	/// Global Managers
	GMemoryPool = new MemoryPool;
	GPlayerManager = new PlayerManager;
	GSessionManager = new SessionManager;
	GIocpManager = new IocpManager;

// 	USHORT port = DEFAULT_PORT;
// 	if( argc >= 3 ){
// 		port = _wtoi(argv[2]);
// 	}

	if (false == GIocpManager->Initialize())
		return -1;

	if (false == GIocpManager->StartIoThreads())
		return -1;

	printf_s("Start Clients\n");
	
	GIocpManager->StartConnects(MAX_CONNECTION); ///< block here...

	GIocpManager->Finalize();

	printf_s("End Clients\n");

	delete GIocpManager;
	delete GSessionManager;
	delete GPlayerManager;
	delete GMemoryPool;

	return 0;
}

