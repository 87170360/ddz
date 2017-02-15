#include <assert.h>

#include "XtBeautyPlayer.h"
#include "XtLog.h"

#include "XtBeautyMacros.h"
#include "XtRedisClient.h"

#include "XtGameServer.h"


int XtBeautyPlayer::getUid()
{
	return m_uid;
}

void XtBeautyPlayer::setData(int uid,XtGameServer* server,XtBeautyGame* game)
{
	m_uid=uid;
	m_server=server;
	m_game=game;

	m_datarc=m_server->getDataRedis(uid);



	int ret = m_datarc->doCommand("hgetall hu:%d", uid);
	if (ret < 0) 
	{
		XT_LOG_ERROR("update info error, because get player infomation error.\n");
		return ;
	}

	if (m_datarc->isArrayReturnOk() < 0) 
	{
		XT_LOG_ERROR("update info error, because get player infomation error.\n");
		return ;
	}

	m_avatar=m_datarc->getValueAsString("avatar");
	m_name=m_datarc->getValueAsString("userName");
	m_money=m_datarc->getValueAsInt("money");
	m_sex=m_datarc->getValueAsInt("sex");

	resetSeatBet();

}


int XtBeautyPlayer::incMoney(int value)
{


#ifdef BG_DEBUG 
	if(m_uid<BG_VALID_CLIENT_UID_MIN)
	{
		return 0;
	}
#endif 
	assert(value>=0);
	int ret;
	ret=m_datarc->doCommand("hincrby hu:%d money %d",m_uid,value);
	if(ret<0)
	{
		XT_LOG_ERROR("incr money error\n");
		return -1;
	}
	m_money=m_datarc->getReply()->integer;
	return 0;

}

int XtBeautyPlayer::decMoney(int value)
{

#ifdef BG_DEBUG 
	if(m_uid<BG_VALID_CLIENT_UID_MIN)
	{
		return 0;
	}
#endif 

	assert(value>=0);
	int ret;
	ret=m_datarc->doCommand("hincrby hu:%d money -%d",m_uid,value);

	if(ret<0)
	{
		XT_LOG_ERROR("decr money error\n");
		return -1;
	}
	m_money=m_datarc->getReply()->integer;
	return 0;

}


int XtBeautyPlayer::updateMoney()
{

#ifdef BG_DEBUG 
	if(m_uid<BG_VALID_CLIENT_UID_MIN)
	{
		return 20000000;
	}
#endif 

	int ret;
	ret=m_datarc->doCommand("hget hu:%d money",m_uid);
	if(ret<0)
	{
		XT_LOG_ERROR("update money error\n");
		return -1;
	}

	m_money=atoi(m_datarc->getReply()->str);
	//printf("player(%d),money is %d\n",m_uid,m_money);

	return m_money;

}


int XtBeautyPlayer::getMoney()
{
#ifdef BG_DEBUG 
	if(m_uid<XT_VALID_CLIENT_UID_MIN)
	{
		return 20000000;
	}
	return m_money;

#else 
	return m_money;
#endif
	
}


int XtBeautyPlayer::getMoneyLockTime()
{
	int ret;
	ret=m_datarc->doCommand("hget hu:%d mlock_time",m_uid);
	if(ret<0)
	{
		XT_LOG_ERROR("get money lock time error");
		return -1;
	}

	if(m_datarc->getReply()->type==REDIS_REPLY_NIL)
	{
		return 0;
	}

	int mlock_time=atoi(m_datarc->getReply()->str);
	return mlock_time;
}




void XtBeautyPlayer::resetSeatBet()
{
	for(int i=0;i<BG_SEAT_ID_NU;i++)
	{
		m_seatBetNu[i]=0;
	}

	m_resultMoney=0;
}




void XtBeautyPlayer::addSeatBet(int seat_id,int  value)
{

	m_seatBetNu[seat_id]+=value;
}

int XtBeautyPlayer::getSeatBet(int seat_id)
{
	return m_seatBetNu[seat_id];
}







