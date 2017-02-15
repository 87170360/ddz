#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <list>
#include <queue>
#include <stdarg.h>
#include "redis_client.h"
#include "log.h"

extern Log xt_log;

RedisClient::RedisClient()
{
    context = NULL;
	reply = NULL;
}

RedisClient::~RedisClient()
{
}

int RedisClient::init(std::string host, int port, int timeout, std::string pass)
{
    _host = host;
    _port = port;
	_timeout = timeout;
	_pass = pass;
	
    return connect_redis();
}

int RedisClient::connect_redis()
{
	struct timeval tv;
	tv.tv_sec = _timeout / 1000;
	tv.tv_usec = _timeout * 1000;
	if (context)
	{
		redisFree(context);
		context = NULL;		
	}

    context = redisConnectWithTimeout(_host.c_str(), _port, tv);
    if (context->err) {
        xt_log.error("redis[%s] host[%s] port[%d]\n", context->errstr, _host.c_str(), _port);
        return -1;
    }
	xt_log.info("redis host[%s] port[%d] is connected succ.\n", _host.c_str(), _port);
	
	int ret =  command_spec("auth %s", _pass.c_str());
	if (ret < 0) {
		xt_log.error("auth %s >> redis[%s] host[%s] port[%d]\n", _pass.c_str(), context->errstr, _host.c_str(), _port);
	}
	return 0;
}

void RedisClient::deinit()
{
	redisFree(context);
	context = NULL;
}

int RedisClient::command(const char *format, ...)
{
	int i = 200000;
    if (reply)
    {
        freeReplyObject(reply);
		reply = NULL;
    }
	
	time_t begin = time(NULL);
	while (i--)
	{
		va_list ap;
		va_start(ap, format);
	    reply = (redisReply*)redisvCommand(context, format, ap);
	    va_end(ap);
	    if (context->err)
	    {
	        xt_log.error("redis[%s] ip[%s] port[%d], reconnecting...\n",
	            context->errstr, _host.c_str(), _port);
			int ret = connect_redis();
			if (ret < 0)
				return -1;
			continue;
	    }
		//xt_log.debug("reply type: %d\n", reply->type);
		break;
	}
	time_t end = time(NULL);
	int ret = end - begin;
	if (ret >= 1)
	{
		xt_log.error("slow redis: [%d]\n", ret);
	}

    return 0;
}

int RedisClient::command_spec(const char *format, ...)
{
    if (reply)
    {
        freeReplyObject(reply);
		reply = NULL;
    }
	
	va_list ap;
	va_start(ap, format);
    reply = (redisReply*)redisvCommand(context, format, ap);
    va_end(ap);
    if (context->err)
    {
		return -1;
	}
	
    return 0;
}

int RedisClient::is_array_return_ok()
{
	if (reply->type == 2)
	{
		if (reply->elements > 0)
		{
			return 0;
		}
		return -1;
	}
	
	return -1;
}

char* RedisClient::get_value_as_string(const char *key)
{
    size_t i = 0;
    while (i < (reply->elements))
    {
        if (!strcmp(key, reply->element[i]->str))
        {
            return reply->element[i + 1]->str;
        }
        i += 2;
    }

    xt_log.error("can't find key[%s]\n", key);
    return "";
}

int RedisClient::get_value_as_int(const char *key)
{
    size_t i = 0;
    while (i < (reply->elements))
    {
        if (!strcmp(key, reply->element[i]->str))
        {
            return ::atoi(reply->element[i + 1]->str);
        }
        i += 2;
    }

    xt_log.error("can't find key[%s]\n", key);
    return 0;
}

long long RedisClient::get_value_as_int64(const char *key)
{
    size_t i = 0;
    while (i < (reply->elements))
    {
        if (!strcmp(key, reply->element[i]->str))
        {
            return ::atoll(reply->element[i + 1]->str);
        }
        i += 2;
    }

    xt_log.error("can't find key[%s]\n", key);
    return 0;
}

float RedisClient::get_value_as_float(const char *key)
{
    size_t i = 0;
    while (i < (reply->elements))
    {
        if (!strcmp(key, reply->element[i]->str))
        {
            return ::atof(reply->element[i + 1]->str);
        }
        i += 2;
    }

    xt_log.error("can't find key[%s]\n", key);
    return 0;
}

