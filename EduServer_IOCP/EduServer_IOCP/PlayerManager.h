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
	int GetInSightPlayers( Player* viewer, PlayerList& outList );
	
private:
	FastSpinlock mLock;
	int mCurrentIssueId;
	xmap<int, Player*>::type mPlayerMap;
};

extern PlayerManager* GPlayerManager;
