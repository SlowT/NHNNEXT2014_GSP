#pragma once

#define SERVER_ADDR		"127.0.0.1"
#define SERVER_PORT		9001
#define MAX_CONNECTION	1
#define CONNECT_TIME	60000

enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER
};

extern __declspec(thread) int LThreadType;