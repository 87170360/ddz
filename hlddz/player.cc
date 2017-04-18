#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "hlddz.h"
#include "log.h"
#include "proto.h"
#include "table.h"
#include "player.h"
#include "client.h"
#include "game.h"
#include "eventlog.h"  // cfc add 20140102

extern HLDDZ hlddz;
extern Log xt_log;

static const int DAY_SECOND = 86400;
static const int ALLOWANCE_NUM = 3;

Player::Player() :
_offline_timeout(1 * 1)
{
	_offline_timer.data = this;
	ev_timer_init(&_offline_timer, Player::offline_timeout, _offline_timeout, 0);
}

Player::~Player()
{
	ev_timer_stop(hlddz.loop, &_offline_timer);
	if (client)
		client->player = NULL;
}

void Player::set_client(Client *c)
{
	client = c;
	m_uid = c->uid;
	index = m_uid % hlddz.main_size;
	client->player = this;
}

int Player::init()
{
	// player information
	reset();
	int ret = hlddz.main_rc[index]->command("hgetall hu:%d", m_uid);
	if (ret < 0) {
		xt_log.error("player init error 1, because get player infomation error.uid:%d\n", m_uid);
		return -1;
	}
	
	if (hlddz.main_rc[index]->is_array_return_ok() < 0) {
		xt_log.error("player init error 2, because get player infomation error.uid:%d\n", m_uid);
		return -1;
	}
	
	m_skey = hlddz.main_rc[index]->get_value_as_string("skey");
	m_name = hlddz.main_rc[index]->get_value_as_string("name");
	m_avatar = hlddz.main_rc[index]->get_value_as_string("avatar");
	m_sex = hlddz.main_rc[index]->get_value_as_int("sex");
	m_money = hlddz.main_rc[index]->get_value_as_int("money");
	m_level = hlddz.main_rc[index]->get_value_as_int("level");
	m_allowance_num = hlddz.main_rc[index]->get_value_as_int("allowance_num");
	m_allowance_stamp = static_cast<time_t>(hlddz.main_rc[index]->get_value_as_int("allowance_stamp"));
    m_exp = hlddz.main_rc[index]->get_value_as_int("exp");

	if(m_uid<XT_ROBOT_UID_MAX)
	{
		set_money(rand() % 10000 + 50000);
	}
	else 
	{
		if (hlddz.conf["tables"]["max_money"].asInt() == 0) 
		{
			if (m_money < hlddz.conf["tables"]["min_money"].asInt()) 
			{
				xt_log.error("player init m_uid[%d] m_money is not fit.\n", m_uid);
				return -1;
			}
		} 
		else 
		{
			if (m_money < hlddz.conf["tables"]["min_money"].asInt() || m_money >= hlddz.conf["tables"]["max_money"].asInt()) 
			{
				xt_log.error("player init m_uid[%d] m_money is not fit.\n", m_uid);
				return -1;
			}
		}
	}

	m_tid = -1;
	idle_count= 0;

	return 0;
}

void Player::reset(void)
{
	stop_offline_timer();
}

int Player::update_info()
{
	int ret = hlddz.main_rc[index]->command("hgetall hu:%d", m_uid);
	if (ret < 0) {
		xt_log.error("update info error, because get player infomation error.\n");
		return -1;
	}

	if (hlddz.main_rc[index]->is_array_return_ok() < 0) {
		xt_log.error("update info error, because get player infomation error.\n");
		return -1;
	}

	m_money = hlddz.main_rc[index]->get_value_as_int("money");

	return 0;
}

int Player::set_money(int value)
{
	int ret=0;
	ret=hlddz.main_rc[index]->command("hset hu:%d money %d",m_uid,value);
	if(ret<0)
	{
		xt_log.error("set money error.\n");
		return -1;
	}

	m_money=value;
	return 0;
}

void Player::start_offline_timer()
{
	ev_timer_start(hlddz.loop, &_offline_timer);
}

void Player::stop_offline_timer()
{
	ev_timer_stop(hlddz.loop, &_offline_timer);
}

void Player::offline_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
	/* player logout
	 * remove from offline table */
	Player* self = (Player*)w->data;
	hlddz.game->del_player(self);
}
    
void Player::changeMoney(int value)
{
	if(value == 0) 
    {
		return;
	}

    int ret = hlddz.main_rc[index]->command("hincrby hu:%d money %d", m_uid, value);
	if (ret < 0) 
    {
        xt_log.error("%s:%d, changeMoney error. m_uid:%d, value:%d\n", __FILE__, __LINE__, m_uid, value); 
		return;
	}
	//xt_log.info("changeMoney m_uid[%d] value[%d] old[%d] new[%d].\n", m_uid, value, m_money, hlddz.main_rc[index]->reply->integer);
    //update the m_money
	m_money = hlddz.main_rc[index]->reply->integer; 
}
    
void Player::keepTotal(bool win)
{
    int ret = hlddz.main_rc[index]->command("hincrby hu:%d total %d", m_uid, 1);
	if (ret < 0) 
    {
        xt_log.error("%s:%d, keepTotal error1. m_uid:%d, value:%d\n", __FILE__, __LINE__, m_uid, 1); 
	}

    if(!win)
    {
        return;
    }

    ret = hlddz.main_rc[index]->command("hincrby hu:%d victory %d", m_uid, 1);
	if (ret < 0) 
    {
        xt_log.error("%s:%d, keepTotal error2. m_uid:%d, value:%d\n", __FILE__, __LINE__, m_uid, 1); 
	}
}
    
bool Player::allowance(int money)
{
    time_t curstamp = time(NULL);
    //不同一天
    if(m_allowance_stamp / DAY_SECOND != curstamp / DAY_SECOND)
    {
        //重置剩余次数         
        m_allowance_num = ALLOWANCE_NUM;
    }

    if(m_allowance_num > 0)
    {
        m_allowance_num--;
        m_allowance_stamp = curstamp;
        changeMoney(money);
        
        xt_log.debug("allowance, uid:%d, money:%d, m_allowance_num:%d\n", m_uid, money, m_allowance_num);
        if(hlddz.main_rc[index]->command("hset hu:%d allowance_num %d", m_uid, m_allowance_num) < 0)
        {
            xt_log.error("set m_allowance_num error.\n");
            return false;
        }

        if(hlddz.main_rc[index]->command("hset hu:%d allowance_stamp %d", m_uid, m_allowance_stamp) < 0)
        {
            xt_log.error("set m_allowance_stamp error.\n");
            return false;
        }
        return true;
    }
            
    return false;
}
    
void Player::addExp(int exp)
{
    if(exp <= 0)
    {
        return;
    }

    m_exp += exp;
    if(levelUp())
    {
        xt_log.debug("levelup, uid:%d, level:%d\n", m_uid, m_level);
        //保存level 
        if(hlddz.main_rc[index]->command("hset hu:%d level %d", m_uid, m_level) < 0)
        {
            xt_log.error("set level error.level:%d\n", m_level);
        }

        //升级奖励
        int money = upMoney();
        xt_log.debug("levelup add money uid:%d, level:%d, money:%d\n", m_uid, m_level, money);
        changeMoney(money);
    }

    //xt_log.debug("add exp, uid:%d, addexp:%d, exp:%d\n", m_uid, exp, m_exp);
    //保存exp
    if(hlddz.main_rc[index]->command("hset hu:%d exp %d", m_uid, m_exp) < 0)
    {
        xt_log.error("set exp error.exp:%d\n", m_exp);
    }
}
    
bool Player::levelUp(void)
{
    //计算下一等级要多少经验
    int nextLevel = m_level + 1;
    int nextExp = 0;
    if(nextLevel <= 4)
    {
        switch(nextLevel) 
        {
            case 1: nextExp = 10; break;
            case 2: nextExp = 30; break;
            case 3: nextExp = 60; break;
            case 4: nextExp = 120; break;
        }
    }
    else
    {
        nextExp = 200 * pow(1.1, nextLevel - 2); 
    }
    
    //个位数保持是0
    nextExp = (nextExp / 10) * 10;

    if(m_exp >= nextExp)
    {
        m_level++;
        return true;
    }

    return false;
}
    
int Player::upMoney(void)
{
    int money = 0;
    if(m_level <= 10)
    {
        switch(m_level) 
        {
            case 1: money = 200; break;
            case 2: money = 400; break;
            case 3: money = 600; break;
            case 4: money = 800; break;
            case 5: money = 1000; break;
            case 6: money = 1200; break;
            case 7: money = 1400; break;
            case 8: money = 1600; break;
            case 9: money = 2000; break;
            case 10: money = 2100; break;
        }
    }
    else
    {
        money = 2100;
        for(int i = 0; i < m_level - 10; ++i)
        {
            money = money * 1.07; 
        }
    }

    //最后两位保持0
    money = (money / 100) * 100;

    return money;
}
