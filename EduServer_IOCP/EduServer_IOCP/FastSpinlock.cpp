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
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while( mLockFlag & LF_WRITE_MASK )
			YieldProcessor();

		if( (InterlockedAdd( &mLockFlag, LF_WRITE_FLAG ) & LF_WRITE_MASK) == LF_WRITE_FLAG )
		{
			/// �ٸ����� readlock Ǯ���ٶ����� ��ٸ���.
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
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while( mLockFlag & LF_WRITE_MASK )
			YieldProcessor();

		//TODO: Readlock ���� ���� (mLockFlag�� ��� ó���ϸ� �Ǵ���?)
		// if ( readlock�� ������ )
		//return;

		if( (InterlockedIncrement( &mLockFlag ) & LF_WRITE_MASK) == 0 )
			return;

		InterlockedDecrement( &mLockFlag );

		// else
		// mLockFlag ����
	}
}

void FastSpinlock::LeaveReadLock()
{
	InterlockedDecrement( &mLockFlag );
}