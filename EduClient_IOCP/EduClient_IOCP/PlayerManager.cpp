#include "stdafx.h"
#include "Player.h"
#include "PlayerManager.h"
#include "EduClient_IOCP.h"

PlayerManager* GPlayerManager = nullptr;

PlayerManager::PlayerManager() : mLock(), mCurrentIssueId( 0 ), mUserIdCount( FIRST_PLAYER_ID-1 )
{

}

int PlayerManager::RegisterPlayer( Player* player )
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
	//TODO: mLock을 read모드로 접근해서 outList에 현재 플레이어들의 정보를 담고 total에는 현재 플레이어의 총 수를 반환.
	FastSpinlockGuard readLocked( mLock, false );

	for( const auto& player : mPlayerMap ){ ///# for (const auto& it : mPlayerMap) 참조로 하는게 좋다
		outList.push_back( player.second );
	}

	int total = outList.size();


	return total;
}
