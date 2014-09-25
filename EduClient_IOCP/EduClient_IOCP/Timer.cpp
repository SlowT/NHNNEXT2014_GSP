#include "stdafx.h"
#include "IocpManager.h"
#include "Exception.h"
#include "EduClient_IOCP.h"
#include "Timer.h"
#include "Player.h"

__declspec(thread) Timer* LTimer = nullptr;

Timer::Timer()
{
	LTickCount = GetTickCount64();
}


void Timer::PushTimerJob( Player* player, uint32_t after )
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	int64_t dueTimeTick = after + LTickCount;
	mTimerJobQueue.push( TimerJobElement( player, dueTimeTick ) );
}


void Timer::DoTimerJob()
{
	/// thread tick update
	LTickCount = GetTickCount64();

	while (!mTimerJobQueue.empty())
	{
		const TimerJobElement timerJobElem = mTimerJobQueue.top();
		
		if( LTickCount < timerJobElem.mExecutionTick )
			break;

		mTimerJobQueue.pop();

		timerJobElem.mPlayer->OnTick();
	}
}

