
#include "XtRedisClient.h"
#include "XtLog.h"


XtRedisClient::XtRedisClient()
{
    m_context = NULL;
	m_reply = NULL;
	m_port=0;
	m_timeout=0;
	m_pass="";
	m_host="127.0.0.1";

}


XtRedisClient::XtRedisClient (const std::string& host,int port,int timeout,const std::string& passwd)
{
	init(host,port,timeout,passwd);
}




XtRedisClient::~XtRedisClient()
{
	deinit();
}




int XtRedisClient::connectRedis()
{
	struct timeval tv;
	tv.tv_sec = m_timeout / 1000;
	tv.tv_usec =m_timeout * 1000;

	if (m_context)
	{
		redisFree(m_context);
		m_context = NULL;		
	}

    m_context = redisConnectWithTimeout(m_host.c_str(), m_port, tv);

    if (m_context->err) 
	{
		XT_LOG_ERROR("redis[%s] host[%s] port[%d]\n", m_context->errstr, m_host.c_str(), m_port);
        return -1;
    }

	XT_LOG_DEBUG("redis host[%s] port[%d] is connected succ.\n", m_host.c_str(), m_port);
	
	int ret =  doSpecCommand("auth %s", m_pass.c_str());
	if (ret < 0) 
	{
		XT_LOG_ERROR("auth %s >> redis[%s] host[%s] port[%d]\n", m_pass.c_str(), m_context->errstr, m_host.c_str(), m_port);
		return -1;
	}

	return 0;
}



int XtRedisClient::init (const std::string& host,int port,int timeout,const std::string& passwd)
{
	m_host=host;
	m_port=port;
	m_timeout=timeout;
	m_pass=passwd;
	return 0;
}
///¶Ï¿ªredis
int XtRedisClient::deinit()
{
	if(m_reply)
	{
		freeReplyObject(m_reply);
		m_reply=NULL;
	}

	if(m_context)
	{
		redisFree(m_context);
		m_context = NULL;
	}
	return 0;
}

int XtRedisClient::doCommand(const char *format, ...)
{
	int i = 200000;
	if (m_reply)
	{
		freeReplyObject(m_reply);
		m_reply = NULL;
	}

	time_t begin = time(NULL);

	while (i--)
	{
		va_list ap;
		va_start(ap, format);
		m_reply = (redisReply*)redisvCommand(m_context, format, ap);
		va_end(ap);
		if (m_context->err)
		{
			XT_LOG_ERROR("redis[%s] ip[%s] port[%d], reconnecting...\n",
					m_context->errstr, m_host.c_str(), m_port);
			int ret = connectRedis();
			if (ret < 0)
			{
				return -1;
			}
			continue;
		}
		break;
	}


	time_t end = time(NULL);
	int ret = end - begin;
	if (ret >= 1)
	{
		XT_LOG_ERROR("slow redis: [%d]\n", ret);
	}

	return 0;
}

int XtRedisClient::doSpecCommand(const char *format, ...)
{
	if (m_reply)
	{
		freeReplyObject(m_reply);
		m_reply = NULL;
	}

	va_list ap;
	va_start(ap, format);
	m_reply = (redisReply*)redisvCommand(m_context, format, ap);
	va_end(ap);

	if (m_context->err)
	{
		return -1;
	}

	return 0;
}

int XtRedisClient::isArrayReturnOk()
{
	if (m_reply->type == 2)
	{
		if (m_reply->elements > 0)
		{
			return 0;
		}
		return -1;
	}

	return -1;
}

const char* XtRedisClient::getValueAsString(const char *key)
{
	size_t i = 0;
	while (i < (m_reply->elements))
	{
		if (!strcmp(key, m_reply->element[i]->str))
		{
			return m_reply->element[i + 1]->str;
		}
		i += 2;
	}

	XT_LOG_ERROR("can't find key[%s]\n", key);

	return "";
}

int XtRedisClient::getValueAsInt(const char *key)
{
	size_t i = 0;
	while (i < (m_reply->elements))
	{
		if (!strcmp(key, m_reply->element[i]->str))
		{
			return ::atoi(m_reply->element[i + 1]->str);
		}
		i += 2;
	}

	XT_LOG_ERROR("can't find key[%s]\n", key);
	return 0;
}

long long XtRedisClient::getValueAsInt64(const char *key)
{
	size_t i = 0;
	while (i < (m_reply->elements))
	{
		if (!strcmp(key, m_reply->element[i]->str))
		{
			return ::atoll(m_reply->element[i + 1]->str);
		}
		i += 2;
	}

	XT_LOG_ERROR("can't find key[%s]\n", key);
	return 0;
}


float XtRedisClient::getValueAsFloat(const char *key)
{
	size_t i = 0;
	while (i < (m_reply->elements))
	{
		if (!strcmp(key, m_reply->element[i]->str))
		{
			return ::atof(m_reply->element[i + 1]->str);
		}
		i += 2;
	}

	XT_LOG_ERROR("can't find key[%s]\n", key);
	return 0;
}

redisReply* XtRedisClient::getReply()
{
	return m_reply;
}






