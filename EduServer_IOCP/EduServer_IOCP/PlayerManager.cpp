#include "stdafx.h"
#include "Player.h"
#include "PlayerManager.h"

PlayerManager* GPlayerManager = nullptr;

PlayerManager::PlayerManager() : mLock(), mCurrentIssueId(0)
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
	//TODO: mLock�� read���� �����ؼ� outList�� ���� �÷��̾���� ������ ��� total���� ���� �÷��̾��� �� ���� ��ȯ.
	FastSpinlockGuard readLocked( mLock, false );

	for( const auto& player : mPlayerMap ){ ///# for (const auto& it : mPlayerMap) ������ �ϴ°� ����
		outList.push_back( player.second );
	}

	int total = outList.size();


	return total;
}

int PlayerManager::GetInSightPlayers( Player* viewer, PlayerList& outList )
{
	FastSpinlockGuard readLocked( mLock, false );

	float tDistSquare( 0.f );
	float vX = viewer->GetPosX(), vY = viewer->GetPosY(), vZ = viewer->GetPosZ();

	for( const auto& player : mPlayerMap ){
		tDistSquare = std::powf( (vX - player.second->GetPosX()), 2 ) + std::powf( (vY - player.second->GetPosY()), 2 ) + std::powf( (vZ - player.second->GetPosZ()), 2 );
		if( tDistSquare < std::powf( viewer->GetSightRadius(), 2 ) )
			outList.push_back( player.second );
	}

	int total = outList.size();

	return total;
}
