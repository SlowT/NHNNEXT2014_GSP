#pragma once
#include "XTL.h"

/// ���� �÷��̾��� ƽ�� �ൿ���� ���� Ÿ�̸�

class Player;

struct TimerJobElement
{
	TimerJobElement( Player* player, int64_t execTick )
		: mPlayer( player ), mExecutionTick( execTick )
	{}
	virtual ~TimerJobElement() {}

	Player*		mPlayer;
	int64_t		mExecutionTick;
};

struct TimerJobComparator
{
	bool operator()( const TimerJobElement& lhs, const TimerJobElement& rhs )
	{
		return lhs.mExecutionTick > rhs.mExecutionTick;
	}
};


typedef std::priority_queue<TimerJobElement, std::deque<TimerJobElement, STLAllocator<TimerJobElement> >, TimerJobComparator> TimerJobPriorityQueue;

class Timer
{
public:

	Timer();

	void PushTimerJob( Player* player, uint32_t after );

	void DoTimerJob();

private:

	TimerJobPriorityQueue	mTimerJobQueue;

};

extern __declspec(thread) Timer* LTimer;