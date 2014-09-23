#include "stdafx.h"
#include "Player.h"
#include "PlayerManager.h"

PlayerManager* GPlayerManager = nullptr;

PlayerManager::PlayerManager() : mLock(), mCurrentIssueId(0), mUserIdCount(100)
{

}

int PlayerManager::RegisterPlayer( std::shared_ptr<Player> player )
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap[++mCurrentIssueId] = player;

	return mCurrentIssueId;
}

void PlayerManager::UnregisterPlayer(int playerId)
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap.erase(playerId);
}


int PlayerManager::GetCurrentPlayers(PlayerList& outList)
{
	//TODO: mLock�� read���� �����ؼ� outList�� ���� �÷��̾���� ������ ��� total���� ���� �÷��̾��� �� ���� ��ȯ.
	FastSpinlockGuard readLocked( mLock, false );

	for( const auto& player : mPlayerMap ){ ///# for (const auto& it : mPlayerMap) ������ �ϴ°� ����
		outList.push_back( player.second );
	}

	int total = outList.size();


	return total;
}
