#include "stdafx.h"
#include "ClientSession.h"
#include "Player.h"
#include "PlayerManager.h"
#include "PacketHandler.h"
#include "Timer.h"

Player::Player(ClientSession* session) : mSession(session), mSightRadius(100.f)
{
	PlayerReset();

	chatMessages.push_back( std::string( "ㅎㅇ" ) );
	chatMessages.push_back( std::string( "누구세요?" ) );
	chatMessages.push_back( std::string( "신급 레이드 [DongChan] 가실분 모집중!" ) );
	chatMessages.push_back( std::string( "렉봐라. 발로 코딩했냐. 영자 나와라!" ) );
	chatMessages.push_back( std::string( "자네, 거기 비누 좀 주워주지 않겠나?" ) );
	chatMessages.push_back( std::string( "어, 사람 잘못 봤네요." ) );
}

Player::~Player()
{
}

void Player::PlayerReset()
{
	FastSpinlockGuard criticalSection(mPlayerLock);

	GPlayerManager->UnregisterPlayer( mPlayerId );

	memset(mPlayerName, 0, sizeof(mPlayerName));
	mPlayerId = -1;
	mPos.x = mPos.y = mPos.z = 0;
	mSightRadius = 100.f;
}

void Player::RequestLoad( int pid )
{
	GPacketHandler->SendLoginRequest( *mSession, pid );
}

void Player::RequestMoveTo( float x, float y, float z )
{
	GPacketHandler->SendMoveToRequest( *mSession, mPlayerId, x, y, z );
}

void Player::RequestChat( const std::string& playerMessage )
{
	GPacketHandler->SendChatRequest( *mSession, mPlayerId, playerMessage );
}

void Player::RequestSight( int pid )
{
	GPacketHandler->SendSightRequest( *mSession, mPlayerId, mSightRadius );
}

void Player::ResultLoad( int pid, const std::string& playerName, float x, float y, float z )
{
	FastSpinlockGuard criticalSection( mPlayerLock );

	mPlayerId = pid;
	strcpy_s( mPlayerName, playerName.c_str() );
	mPos.x = x;
	mPos.y = y;
	mPos.z = z;

	GPlayerManager->RegisterPlayer( this );

	printf_s( "[LOG] %s is loaded.\n", mPlayerName );

	LTimer->PushTimerJob( this, 1000 );
}

void Player::ResultMoveTo( int pid, float x, float y, float z )
{
	if( mPlayerId != pid )
		mSession->DisconnectRequest( DR_NONE );

	FastSpinlockGuard criticalSection( mPlayerLock );

	mPos.x = x;
	mPos.y = y;
	mPos.z = z;

	printf_s( "[LOG] %s is moved to (%f,%f,%f).\n", mPlayerName, mPos.x, mPos.y, mPos.z );
}

void Player::ResultChat( const std::string& playerName, const std::string& playerMessage )
{
	printf_s( "[%s -> %s]\t%s\n", playerName.c_str(), mPlayerName, playerMessage.c_str() );
}

void Player::OnTick()
{
	switch( std::rand() % 4 ){
	case 0:
		RequestMoveTo( mPos.x - PLAYER_MOVE_SPEED, mPos.y, mPos.z );
		break;
	case 1:
		RequestMoveTo( mPos.x + PLAYER_MOVE_SPEED, mPos.y, mPos.z );
		break;
	case 2:
		RequestMoveTo( mPos.x, mPos.y, mPos.z - PLAYER_MOVE_SPEED );
		break;
	case 3:
		RequestMoveTo( mPos.x, mPos.y, mPos.z + PLAYER_MOVE_SPEED );
		break;
	}

	if( std::rand() % 100 < PLAYER_CHAT_CHANCE)	{
		RequestSight( mPlayerId );
	}

	LTimer->PushTimerJob( this, 100 );
}

void Player::ResultSight()
{
	if( mInSightPlayers.size() > 0 ){
		int i = rand() % chatMessages.size();
		RequestChat( chatMessages[i] );
	}
}
