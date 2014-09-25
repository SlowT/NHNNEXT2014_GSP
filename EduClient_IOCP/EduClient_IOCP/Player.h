#pragma once
#include "ContentsConfig.h"
#include "FastSpinlock.h"
#include "XTL.h"
#include <array>

struct Position
{
	float x;
	float y;
	float z;
};

struct InSightPlayer
{
	std::string name;
	Position pos;
};

typedef xvector<InSightPlayer>::type InSightPlayers;

class ClientSession;

class Player
{
public:
	Player(ClientSession* session);
	~Player();

	bool IsLoaded() { return mPlayerId > 0; }
	
	void RequestLoad(int pid);
	void RequestMoveTo( float x, float y, float z );
	void RequestChat( const std::string& playerMessage );
	void RequestSight( int pid );

	void ResultLoad( int pid, const std::string& playerName, float x, float y, float z );
	void ResultMoveTo( int pid, float x, float y, float z );
	void ResultChat( const std::string& playerName, const std::string& playerMessage );
	void ResultSight();

	void OnTick();

	int GetPlayerId(){ return mPlayerId; }

	InSightPlayers& GetInSightList() { return mInSightPlayers; }

	// for test
	std::vector<std::string> chatMessages;

private:

	void PlayerReset();

private:

	FastSpinlock mPlayerLock;
	int		mPlayerId;
	Position mPos;
	char	mPlayerName[MAX_NAME_LEN];
	float	mSightRadius;

	InSightPlayers mInSightPlayers;

	ClientSession* const mSession;
	friend class ClientSession;
};