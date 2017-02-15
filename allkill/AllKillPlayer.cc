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

	m_seatId = -1;   //玩家就坐的位置(-1表示没有位置)
	m_winCount = 0;

	resetSeatBet();

	last_speak_time = 0;
	mBanned = 0;         //初始未禁言
	mQuickSpeakTimes = 0;

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

///设置禁言标记
void AllKillPlayer::setBanned(int banned) //设置禁言
{
	//更新禁言标记
	mBanned = banned;

	// 更新到redis
	if (NULL == m_datarc)
	{
		m_datarc=m_server->getDataRedis(m_uid);
	}

	if (m_datarc != NULL)
	{
		int ret = m_datarc->command("hset hu:%d banned",m_uid, banned);
		if (ret < 0)
		{
			xt_log.error("Func[%s] set banned error  UID[%d]\n", __FUNCTION__, banned);
		}

		xt_log.debug("Func[%s]  UID[%d] banned[%d].\n",__FUNCTION__, m_uid, banned);
	}
	else
	{
		xt_log.error("Func[%s] m_datarc is null", __FUNCTION__);
	}

}

int AllKillPlayer::getBanned()
{
	//已经被禁言了，就不需要再查了
	if (mBanned > 0)
	{
		return mBanned;
	}

	if (NULL == m_datarc)
	{
		m_datarc=m_server->getDataRedis(m_uid);
	}

	if (m_datarc != NULL)
	{
		int ret = m_datarc->command("hget hu:%d banned",m_uid);
		if (ret < 0)
		{
			xt_log.error("update banned error\n");
		}

		//保存禁言标记
		mBanned = m_datarc->reply->integer;
		xt_log.debug("Func[%s] UID[%d] banned[%d].\n",__FUNCTION__, m_uid, mBanned);		
	}
	else
	{
		xt_log.error("Func[%s] m_datarc is null", __FUNCTION__);
	}

	return mBanned;
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
	m_name=m_datarc->get_value_as_string("nickName");
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
	for(int i=0;i < AK_SEAT_ID_NU; i++)
	{
		m_seatBetNu[i]=0;
		m_tmpSeatBetNu[i]=0;
	}
	m_betMoneyResult=0;
}

void AllKillPlayer::addSeatBet(int seat_id,int value)
{
	m_seatBetNu[seat_id - AK_SEAT_ID_START] += value;

	// 临时下注值
	m_tmpSeatBetNu[seat_id - AK_SEAT_ID_START] += value;
}

// 获取在坐位上的下注数
int AllKillPlayer::getSeatBet(int seat_id)
{
	return m_seatBetNu[seat_id - AK_SEAT_ID_START];
}

// 取完就清空
int AllKillPlayer::getTmpSeatBet(int seat_id)
{
	int tmpBetNu = m_tmpSeatBetNu[seat_id - AK_SEAT_ID_START];
	m_tmpSeatBetNu[seat_id - AK_SEAT_ID_START] = 0;
	return tmpBetNu;
}


//累加玩家赢的
void AllKillPlayer::addBetMoneyResult(int value)
{
	assert(value >= 0);
	m_betMoneyResult += value;
}

//累减玩家输的
void AllKillPlayer::decBetMoneyResult(int value)
{
	assert(value >= 0);
	m_betMoneyResult -= value;
}

void AllKillPlayer::setBetMoneyResult(int value)
{
	m_betMoneyResult=value;
}

int AllKillPlayer::getBetMoneyResult()
{
	return m_betMoneyResult;
}
