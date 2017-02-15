#ifndef _REDIS_SUB_H_
#define _REDIS_SUB_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ev++.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libev.h>

//class Server;

class RedisSub {
public:
	std::string   str;
//	Server        *server;
	redisAsyncContext *ac;
	redisReply *reply;

public:
	RedisSub();
	virtual ~RedisSub();
	int init(std::string host, int port, std::string pass);
	int async_connect_redis();
	void deinit();

private:
	std::string _host;
	int _port;
	std::string _pass;

};


#endif    /*_REDIS_SUB_H_*/
