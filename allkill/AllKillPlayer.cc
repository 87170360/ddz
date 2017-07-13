#include <assert.h>
#include "AllKillPlayer.h"
#include "AllKillMacros.h"
#include "AllKillGame.h"
#include "AllKillServer.h"
#include "redis_client.h"
#include "log.h"

extern Log xt_log;


#ifdef AK_DEBUG 
int AllKillPlayer::ms_objectNu=0;
#endif 



AllKillPlayer::AllKillPlayer()
{
	m_uid=-1;
	m_money=0;
	m_sex=-1;
	m_datarc=NULL;
	m_game=NULL;
	m_server=NULL;
	m_isUnRole=true;

	resetSeatBet();

#ifdef AK_DEBUG
	ms_objectNu++;
#endif 

}

AllKillPlayer::~AllKillPlayer()
{
#ifdef AK_DEBUG 
	ms_objectNu--;
#endif 

}


void AllKillPlayer::setData(int uid,AllKillServer* server,AllKillGame* game)
{
	m_uid=uid;
	m_server=server;
	m_game=game;
	m_datarc=m_server->getDataRedis(uid);


	int ret = m_datarc->command("hgetall hu:%d", uid);
	if (ret < 0) 
	{
		xt_log.error("update info error, because get player infomation error.\n");
		return ;
	}

	if (m_datarc->is_array_return_ok() < 0) 
	{
		xt_log.error("update info error, because get player infomation error.\n");
		return ;
	}

	m_avatar=m_datarc->get_value_as_string("avatar");
	m_name=m_datarc->get_value_as_string("name");
	m_money=m_datarc->get_value_as_int("money");
	m_sex=m_datarc->get_value_as_int("sex");



	resetSeatBet();
}


int AllKillPlayer::setTimeLock(int value)
{
	int now=time(NULL)+value;

	int ret=m_datarc->command("hset hu:%d mlock_time %d",m_uid,now);
	return  ret;
}



int AllKillPlayer::getMoney()
{
#ifdef AK_DEBUG 
	if(m_uid<VALID_CLIENT_UID_MIN)
	{
		return 20000000;
	}
	return m_money;

#else 
	return m_money;
#endif
	

}




int AllKillPlayer::updateMoney()
{

#ifdef AK_DEBUG 
	if(m_uid<VALID_CLIENT_UID_MIN)
	{
		return 20000000;
	}
#endif 

	int ret;
	ret=m_datarc->command("hget hu:%d money",m_uid);
	if(ret<0)
	{
		xt_log.error("update money error\n");
		return -1;
	}

	m_money=atoi(m_datarc->reply->str);
	//printf("player(%d),money is %d\n",m_uid,m_money);

	return m_money;
}




int AllKillPlayer::incMoney(int value)
{
#ifdef AK_DEBUG 
	if(m_uid<VALID_CLIENT_UID_MIN)
	{
		return 0;
	}
#endif 
	assert(value>=0);
	int ret;
	ret=m_datarc->command("hincrby hu:%d money %d",m_uid,value);
	if(ret<0)
	{
		xt_log.error("incr money error\n");
		return -1;
	}
	m_money=m_datarc->reply->integer;
	return 0;
}


int AllKillPlayer::decMoney(int value)
{

#ifdef AK_DEBUG 
	if(m_uid<VALID_CLIENT_UID_MIN)
	{
		return 0;
	}
#endif 

	assert(value>=0);
	int ret;
	ret=m_datarc->command("hincrby hu:%d money -%d",m_uid,value);

	if(ret<0)
	{
		xt_log.error("decr money error\n");
		return -1;
	}
	m_money=m_datarc->reply->integer;
	return 0;
}



void AllKillPlayer::resetSeatBet()
{
	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		m_seatBetNu[i]=0;
	}
	m_betMoneyResult=0;
}

void AllKillPlayer::addSeatBet(int seat_id,int value)
{
	m_seatBetNu[seat_id-AK_SEAT_ID_START]+=value;
}

int AllKillPlayer::getAllKillTotalBet(void)
{
    int total = 0;
	for(int i=0; i < AK_SEAT_ID_NU; i++)
	{
		total += m_seatBetNu[i];
	}
    return total;
}

int AllKillPlayer::getSeatBet(int seat_id)
{
	return m_seatBetNu[seat_id-AK_SEAT_ID_START];
}


void AllKillPlayer::addBetMoneyResult(int value)
{
	assert(value>=0);
	m_betMoneyResult+=value;
}

void AllKillPlayer::decBetMoneyResult(int value)
{
	assert(value>=0);
	m_betMoneyResult-=value;
}

void AllKillPlayer::setBetMoneyResult(int value)
{
	m_betMoneyResult=value;
}

int AllKillPlayer::getBetMoneyResult()
{
	return m_betMoneyResult;
}

bool AllKillPlayer::checkBanned(void)
{
	int ret = m_datarc->command("hget hu:%d banned", m_uid);
	if(ret < 0)
	{
		xt_log.error("get banned error\n");
		return false;
	}

	int banned = atoi(m_datarc->reply->str);
	return (banned != 0);
}




