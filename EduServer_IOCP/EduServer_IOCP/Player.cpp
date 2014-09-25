#include "stdafx.h"
#include "ClientSession.h"
#include "Player.h"
#include "PlayerDBContext.h"
#include "DBManager.h"
#include "PlayerManager.h"
#include "PacketHandler.h"
#include "Log.h"

Player::Player(ClientSession* session) : mSession(session), mSightRadius(100.f)
{
	PlayerReset();
}

Player::~Player()
{
}

void Player::PlayerReset()
{
	FastSpinlockGuard criticalSection(mPlayerLock);

	memset(mPlayerName, 0, sizeof(mPlayerName));
	memset(mComment, 0, sizeof(mComment));
	mPlayerId = -1;
	mIsValid = false;
	mPosX = mPosY = mPosZ = 0;
}

void Player::RequestLoad(int pid)
{
 	LoadPlayerDataContext* context = new LoadPlayerDataContext(mSession, pid);
 	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseLoad(int pid, float x, float y, float z, bool valid, wchar_t* name, wchar_t* comment)
{
	FastSpinlockGuard criticalSection(mPlayerLock);

	mPlayerId = pid;
	mPosX = x;
	mPosY = y;
	mPosZ = z;
	mIsValid = valid;

	wcscpy_s(mPlayerName, name);
	wcscpy_s(mComment, comment);

	GPlayerManager->RegisterPlayer( this );
	wprintf_s(L"PID[%d], X[%f] Y[%f] Z[%f] NAME[%s] COMMENT[%s]\n", mPlayerId, mPosX, mPosY, mPosZ, mPlayerName, mComment);

	ClientPacket::Position tPos;
	tPos.set_x( mPosX );
	tPos.set_y( mPosY );
	tPos.set_z( mPosZ );
	GPacketHandler->SendLoginResult( *mSession, mPlayerId, std::string(&mPlayerName[0], &mPlayerName[MAX_NAME_LEN-1]), tPos );
}

void Player::RequestUpdatePosition(float x, float y, float z)
{
	//todo: DB�� �÷��̾� ��ġ�� x,y,z�� ������Ʈ ��û�ϱ�
	UpdatePlayerPositionContext* context = new UpdatePlayerPositionContext( mSession, mPlayerId );
	context->mPosX = x;
	context->mPosY = y;
	context->mPosZ = z;

	GDatabaseManager->PostDatabsaseRequest( context );
}
void Player::RequestUpdateLogoutPosition()
{
	UpdatePlayerLogoutPositionContext* context = new UpdatePlayerLogoutPositionContext( mSession, mPlayerId );
	context->mPosX = mPosX;
	context->mPosY = mPosY;
	context->mPosZ = mPosZ;

	GDatabaseManager->PostDatabsaseRequest( context );
}

void Player::ResponseUpdatePosition(float x, float y, float z)
{
	{
		FastSpinlockGuard criticalSection( mPlayerLock );
		mPosX = x;
		mPosY = y;
		mPosZ = z;
	}
	if( !GPacketHandler->SendMoveResult( *mSession, mPlayerId, x, y, z ) )
		EVENT_LOG( "SendMoveResult fail", mPlayerId );
}

void Player::RequestUpdateComment(const wchar_t* comment)
{
	UpdatePlayerCommentContext* context = new UpdatePlayerCommentContext(mSession, mPlayerId);
	context->SetNewComment(comment);
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseUpdateComment(const wchar_t* comment)
{
	FastSpinlockGuard criticalSection(mPlayerLock);
	wcscpy_s(mComment, comment);
}

void Player::RequestUpdateValidation(bool isValid)
{
	UpdatePlayerValidContext* context = new UpdatePlayerValidContext(mSession, mPlayerId);
	context->mIsValid = isValid;
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseUpdateValidation(bool isValid)
{
	FastSpinlockGuard criticalSection(mPlayerLock);
	mIsValid = isValid;
}


void Player::TestCreatePlayerData(const wchar_t* newName)
{
	//todo: DB������Ǯ�� newName�� �ش��ϴ� �÷��̾� ���� �۾��� ������Ѻ���
	CreatePlayerDataContext* context = new CreatePlayerDataContext( mSession );
	context->SetNewName( newName );
	GDatabaseManager->PostDatabsaseRequest( context );
}

void Player::Chat( const std::string& playerMessage )
{
	PlayerList curPlayers;
	GPlayerManager->GetCurrentPlayers( curPlayers );

	float tDistSquare(0.f);
	for( auto& player : curPlayers ){
		tDistSquare = std::powf( mPosX - player->GetPosX(), 2 ) + std::powf( mPosY - player->GetPosY(), 2 ) + std::powf( mPosZ - player->GetPosZ(), 2 );
		if( tDistSquare < std::powf( player->GetSightRadius(), 2 ) )
			if( !GPacketHandler->SendChatResult( *player->mSession, GetNameByString(), playerMessage ) )
				EVENT_LOG( "SendChatResult fail", mPlayerId );
	}
}

std::string Player::GetNameByString()
{
	return std::string( &mPlayerName[0], &mPlayerName[MAX_NAME_LEN - 1] );
}

void Player::ViewSight()
{
	PlayerList inSightList;
	GPlayerManager->GetInSightPlayers( this, inSightList );

	if( !GPacketHandler->SendSightResult( *mSession, inSightList ) )
		EVENT_LOG( "SendSightResult fail", mPlayerId );
}

// void Player::TestDeletePlayerData(int playerId)
// {
// 	//todo: DB������Ǯ�� playerId�� �ش��ϴ� �÷��̾� ���� ���� �۾��� ������Ѻ���
// 	DeletePlayerDataContext* context = new DeletePlayerDataContext( mSession, playerId );
// 	GDatabaseManager->PostDatabsaseRequest( context );
// }
// 
