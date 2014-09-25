#pragma once

#define SERVER_ADDR		"10.73.44.51"
#define SERVER_PORT		9001
#define MAX_CONNECTION	2000
#define FIRST_PLAYER_ID 100
#define CONNECT_TIME	60000
#define PLAYER_MOVE_SPEED 1.f
#define PLAYER_CHAT_CHANCE 5 //ÆÛ¼¾Æ®


enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER
};

extern __declspec(thread) int LThreadType;