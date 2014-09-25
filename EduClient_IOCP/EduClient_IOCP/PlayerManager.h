#pragma once

#include "FastSpinlock.h"
#include "XTL.h"

class Player;
typedef xvector<Player*>::type PlayerList;

class PlayerManager
{
public:
	PlayerManager();

	/// �÷��̾ ����ϰ� ID�� �߱�
	int RegisterPlayer( Player* player );

	/// ID�� �ش��ϴ� �÷��̾� ����
	void UnregisterPlayer(int playerId);

	int GetCurrentPlayers(PlayerList& outList);
	long GetUserIdCount(){
		return InterlockedIncrement( &mUserIdCount );
	}

	void DoPlayersOnTick( int startNum, int gap );

private:
	FastSpinlock mLock;
	int mCurrentIssueId;
	xmap<int, Player*>::type mPlayerMap;

	// for test ----
	PlayerList mPlayerList;
	volatile long mUserIdCount;
	// ---- for test
};

extern PlayerManager* GPlayerManager;
