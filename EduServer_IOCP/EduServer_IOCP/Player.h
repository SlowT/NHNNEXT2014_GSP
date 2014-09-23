#pragma once
#include "ContentsConfig.h"
#include "FastSpinlock.h"

class ClientSession;

class Player
{
public:
	Player(ClientSession* session);
	~Player();

	bool IsLoaded() { return mPlayerId > 0; }
	
	void RequestLoad(int pid);
	void ResponseLoad(int pid, float x, float y, float z, bool valid, wchar_t* name, wchar_t* comment);

	void RequestUpdatePosition(float x, float y, float z);
	void RequestUpdateLogoutPosition();
	void ResponseUpdatePosition(float x, float y, float z);

	void RequestUpdateComment(const wchar_t* comment);
	void ResponseUpdateComment(const wchar_t* comment);

	void RequestUpdateValidation(bool isValid);
	void ResponseUpdateValidation(bool isValid);

	int GetPlayerId(){ return mPlayerId; }
	std::string GetNameByString();
	float GetPosX(){ return mPosX; }
	float GetPosY(){ return mPosY; }
	float GetPosZ(){ return mPosZ; }
	float GetSightRadius(){ return mSightRadius; }

	void Chat( const std::string& playerMessage );
	void ViewSight();

private:

	void PlayerReset();

public:
	void TestCreatePlayerData(const wchar_t* newName);
// 	void TestDeletePlayerData(int playerId);

private:

	FastSpinlock mPlayerLock;
	int		mPlayerId;
	float	mPosX;
	float	mPosY;
	float	mPosZ;
	bool	mIsValid;
	wchar_t	mPlayerName[MAX_NAME_LEN];
	wchar_t	mComment[MAX_COMMENT_LEN];

	float	mSightRadius;

	ClientSession* const mSession;
	friend class ClientSession;
};