#ifndef _XT_REDIS_CLIENT_H_
#define _XT_REDIS_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <list>
#include <vector>
#include <queue>
#include <string>
#include "hiredis/hiredis.h"

///---¶ÁÐ´redisÊý¾Ý¿â
class XtRedisClient 
{
	public:
		XtRedisClient();
		XtRedisClient(const std::string& host,int port,int timeout,const std::string& passwd);
		virtual ~XtRedisClient();

	public:
		int connectRedis();
		int deinit();
		int init(const std::string& host,int port,int timeout,const std::string& passwd);



		int doCommand(const char *format, ...);
		int doSpecCommand(const char *format, ...);

		int isArrayReturnOk();
		const char* getValueAsString(const char* key);
		int getValueAsInt(const char *key);
		long long getValueAsInt64(const char *key);
		float getValueAsFloat(const char*key);

	public:
		redisReply* getReply();


	private:
		std::string m_host;
		int m_port;
		int m_timeout;
		std::string m_pass;


	private:
		redisReply*	m_reply;
		redisContext* m_context;
};

#endif /*_XT_REDIS_CLIENT_H_*/


