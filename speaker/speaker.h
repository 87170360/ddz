#ifndef _SPEAKER_H_
#define _SPEAKER_H_

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <json/json.h>
#include "redis_client.h"
#include "redis_sub.h"

class Server;

class Speaker {
public:
	std::string     conf_file;
	int             is_daemonize;
	Json::Value     conf;
	Server          *server;
	struct ev_loop  *loop;
	int             main_size;
	RedisClient     *main_rc[20];
	RedisClient     *eventlog_rc;

	int             sub_size;     // ¶©ÔÄ
	RedisSub        *sub_rc[10];

	int             pub_size;     // ·¢²¼
	RedisClient     *pub_rc[10];
};


#endif   /*_SPEAKER_H_*/
