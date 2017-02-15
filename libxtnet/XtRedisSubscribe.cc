#include "XtRedisSubscribe.h"
#include "XtLog.h"


static void XtRedisSubscribe_ConnectCallBack(const redisAsyncContext* c,int status)
{

	if (status != REDIS_OK) 
	{
		XT_LOG_ERROR("Error: %s\n", c->errstr);
		return;
	}
	XT_LOG_INFO("async redis Connected...\n");

}


static void XtRedisSubscribe_DisconnectCallBack(const redisAsyncContext* c,int status)
{
	if (status != REDIS_OK) 
	{
		XT_LOG_ERROR("Error: %s\n", c->errstr);
		return;
	}
	XT_LOG_INFO("async redis Disconnected...\n");
}





static void XtRedisSubscribe_subscribeCallBack(redisAsyncContext* c,void* r,void* priv)
{
	redisReply* reply=(redisReply*) r;
	if(reply==NULL)
	{
		return;
	}

	XtRedisSubscribe* rs=(XtRedisSubscribe*) priv;

	if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) 
	{
		if (strcmp( reply->element[0]->str, "message" ) == 0) 
		{
			std::string subdata = reply->element[2]->str;
			rs->onSubscribe(reply->element[1]->str, reply->element[2]->str);
		}
	}
}



XtRedisSubscribe::XtRedisSubscribe(struct ev_loop* loop)
{
	m_host="127.0.0.1";
	m_port=0;
	m_passwd="";

	m_asynContext=NULL;

	m_evLoop=loop;
}


XtRedisSubscribe::~XtRedisSubscribe()
{
	deinit();
}


int XtRedisSubscribe::init(const std::string& host,int port,const std::string& passwd)
{
	m_host=host;
	m_port=port;
	m_passwd=passwd;

	return 0;
}

int XtRedisSubscribe::deinit()
{
	if(m_asynContext)
	{
	
		redisAsyncDisconnect(m_asynContext);
		redisAsyncFree(m_asynContext);
		m_asynContext=NULL;
	}
	m_evLoop=NULL;

	return 0;
}



int XtRedisSubscribe::asynConnectRedis()
{
	if(m_asynContext)
	{
		redisAsyncFree(m_asynContext);
		m_asynContext=NULL;
	}

	m_asynContext=redisAsyncConnect(m_host.c_str(),m_port);

	if(m_asynContext->err)
	{
		XT_LOG_ERROR( "redis[%s] host[%s] port[%d]\n", m_asynContext->errstr, m_host.c_str(), m_port);
		return -1;
	}

	XT_LOG_DEBUG(" async redis host[%s] port[%d] is connected succ.\n", m_host.c_str(), m_port);


	redisLibevAttach(m_evLoop, m_asynContext);

	redisAsyncSetConnectCallback(m_asynContext, XtRedisSubscribe_ConnectCallBack);

	redisAsyncSetDisconnectCallback(m_asynContext, XtRedisSubscribe_DisconnectCallBack);

	redisAsyncCommand(m_asynContext, NULL, NULL, "auth %s", m_passwd.c_str());


	return 0;
}

///---¶©ÔÄÏûÏ¢
int XtRedisSubscribe::subscribe(const char* key)
{
	char buf[1024];
	sprintf(buf,"SUBSCRIBE %s",key);
	redisAsyncCommand(m_asynContext, XtRedisSubscribe_subscribeCallBack, this, buf);
	return 0;
}
///½â³ý¶©ÔÄ
int XtRedisSubscribe::unsubscribe(const char* key)
{
	char buf[1024];
	sprintf(buf,"UNSUBSCRIBE %s",key);
	redisAsyncCommand(m_asynContext, XtRedisSubscribe_subscribeCallBack, this, buf);

	return 0;
}


