#include "XtBeautyServer.h"
#include "XtBeautyGame.h"
#include "XtBeautyPlayer.h"
#include "protocol/XtMsgProtocol.h"
#include "XtGameClient.h"

#include "XtRedisClient.h"
#include "XtLog.h"

XtGameLogic* XtBeautyServer::onCreateGame()
{

	XtGameLogic* ret=new XtBeautyGame;
	return ret;
}


XtGameClient* XtBeautyServer::onCreateClient()
{
	XtGameClient* ret =new XtGameClient(m_evLoop,this);
	XtMsgProtocol* protocol=new XtMsgProtocol();

	protocol->setXorEnabled(true);
	char key[]={13,0};
	protocol->setXorKey(key);

	ret->setProtocol(protocol);

	return ret;
}





int XtBeautyServer::refreshPlayerNuToRedis()
{

	int player_nu=m_loginClient.size();

	int ret=m_cacheRc->doCommand("hset gameinfo %d %d",m_conf["game"]["port"].asInt(),player_nu);

	if(ret<0)
	{
		XT_LOG_INFO("hset gameinfo error\n");
	}

	return ret;
}




