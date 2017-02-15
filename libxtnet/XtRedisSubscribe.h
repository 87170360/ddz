#ifndef _XT_REDIS_SUBSCRIBE_H_
#define _XT_REDIS_SUBSCRIBE_H_ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ev++.h>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libev.h>
///¶©ÔÄredisÏûÏ¢
class XtRedisSubscribe 
{
	public:
		XtRedisSubscribe(struct ev_loop* loop );
		virtual ~XtRedisSubscribe();

	public:
		int init(const std::string& host,int port,const std::string&  passwd);
		int deinit();

		int asynConnectRedis();

	public:
		int subscribe(const char* key);
		int unsubscribe(const char* key);

	public:
		virtual void onSubscribe(const char* key,const char* data);

	protected:
		std::string m_host;
		int m_port;
		std::string m_passwd;

		struct ev_loop* m_evLoop;
		redisAsyncContext* m_asynContext;
};




#endif 




