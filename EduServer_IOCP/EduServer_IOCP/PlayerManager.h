#pragma once

#include "FastSpinlock.h"
#include "XTL.h"

class Player;
typedef xvector<Player*>::type PlayerList;

class PlayerManager
{
public:
	PlayerManager();

	/// 플레이어를 등록하고 ID를 발급
	int RegisterPlayer( Player* player );

	/// ID에 해당하는 플레이어 제거
	void UnregisterPlayer(int playerId);

	int GetCurrentPlayers(PlayerList& outList);
	int GetInSightPlayers( Player* viewer, PlayerList& outList );
	
private:
	FastSpinlock mLock;
	int mCurrentIssueId;
	xmap<int, Player*>::type mPlayerMap;
};

extern PlayerManager* GPlayerManager;
