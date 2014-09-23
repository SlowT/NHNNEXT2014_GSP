#include "stdafx.h"
#include "FastSpinlock.h"


FastSpinlock::FastSpinlock() : mLockFlag(0)
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterWriteLock()
{
	while( true )
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while( mLockFlag & LF_WRITE_MASK )
			YieldProcessor();

		if( (InterlockedAdd( &mLockFlag, LF_WRITE_FLAG ) & LF_WRITE_MASK) == LF_WRITE_FLAG )
		{
			/// 다른놈이 readlock 풀어줄때까지 기다린다.
			while( mLockFlag & LF_READ_MASK )
				YieldProcessor();

			return;
		}

		InterlockedAdd( &mLockFlag, -LF_WRITE_FLAG );
	}

}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd( &mLockFlag, -LF_WRITE_FLAG );
}

void FastSpinlock::EnterReadLock()
{
	while( true )
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while( mLockFlag & LF_WRITE_MASK )
			YieldProcessor();

		//TODO: Readlock 진입 구현 (mLockFlag를 어떻게 처리하면 되는지?)
		// if ( readlock을 얻으면 )
		//return;

		if( (InterlockedIncrement( &mLockFlag ) & LF_WRITE_MASK) == 0 )
			return;

		InterlockedDecrement( &mLockFlag );

		// else
		// mLockFlag 원복
	}
}

void FastSpinlock::LeaveReadLock()
{
	InterlockedDecrement( &mLockFlag );
}