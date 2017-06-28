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

Player::Player() : m_table_count(0), _offline_timeout(60 * 30) 
{
	_offline_timer.data = this;
	ev_timer_init(&_offline_timer, Player::offline_timeout, _offline_timeout, 0);
}

Player::~Player()
{
	ev_timer_stop(hlddz.loop, &_offline_timer);
	if (client)
		client->player = NULL;
    printf("%s", DESC_STATE[0]);
    printf("%s", DESC_OP[0]);
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
    m_exp = hlddz.main_rc[index]->get_value_as_int("exp");
    m_top_money = hlddz.main_rc[index]->get_value_as_int("top_money");
    m_top_count = hlddz.main_rc[index]->get_value_as_int("top_count");
    xt_log.debug("player init, uid:%d, money:%d, skey:%s, name:%s, avatar:%s\n", m_uid, m_money, m_skey.c_str(), m_name.c_str(), m_avatar.c_str());

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
	int ret = hlddz.main_rc[index]->command("hget hu:%d money", m_uid);
    long long value = 0;
	if (ret < 0 || hlddz.main_rc[index]->getSingleInt(value) == false) 
    {
		xt_log.error("update_info error.uid:%d\n", m_uid);
	}
    //xt_log.debug("money:%d\n", value);
    m_money = value;

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
    //xt_log.debug("%s:%d, start_offlie_timer. m_uid:%d\n", __FILE__, __LINE__, m_uid); 
	ev_timer_start(hlddz.loop, &_offline_timer);
}

void Player::stop_offline_timer()
{
    //xt_log.debug("%s:%d, stop_offlie_timer. m_uid:%d\n", __FILE__, __LINE__, m_uid); 
	ev_timer_stop(hlddz.loop, &_offline_timer);
}

void Player::offline_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
	/* player logout
	 * remove from offline table */
	Player* self = (Player*)w->data;
    xt_log.debug("%s:%d, offline_timeout. m_uid:%d\n", __FILE__, __LINE__, self->m_uid); 
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

void Player::updateTopMoney(int money)
{
	if(m_top_money >= money) 
    {
		return;
	}

    if(hlddz.main_rc[index]->command("hset hu:%d top_money %d", m_uid, m_top_money) < 0)
    {
        xt_log.error("set top_money error.exp:%d\n", m_top_money);
    }
    else
    {
        m_top_money = money;
    }
}

void Player::updateTopCount(int count)
{
	if(m_top_count >= count) 
    {
		return;
	}

    if(hlddz.main_rc[index]->command("hset hu:%d top_count %d", m_uid, m_top_count) < 0)
    {
        xt_log.error("set top_count error.exp:%d\n", m_top_count);
    }
    else
    {
        m_top_count = count;
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

int Player::coupon(int score) 
{
    if(score < 10000)
    {
        return 0;
    }

    //当日上限
    int limit = couponLimit();
    if(limit <= 0)
    {
        return 0;
    }

    //付费标识
    int paid_coupon = 0; 
    int normal_coupon = 0; 
	int paid = hlddz.main_rc[index]->get_value_as_int("paid");
    if(score >= 10000 && score < 30000)
    {
        normal_coupon = rand() % 2 + 2;
        paid_coupon = rand() % 4 + 3;
    }
    else if(score >= 30000 && score < 100000)
    {
        normal_coupon = rand() % 3 + 3;
        paid_coupon = rand() % 6 + 5;
    }
    else if(score >= 100000 && score < 200000)
    {
        normal_coupon = rand() % 4 + 4;
        paid_coupon = rand() % 9 + 7;
    }
    else if(score >= 200000)
    {
        normal_coupon = rand() % 6 + 7;
        paid_coupon = rand() % 9 + 12;
    }

    int result = (paid == 1) ? paid_coupon : normal_coupon;

    int ret = hlddz.main_rc[index]->command("hincrby hu:%d coupon %d", m_uid, result);
	if (ret < 0) 
    {
        xt_log.error("%s:%d, coupon error. m_uid:%d, value:%d\n", __FILE__, __LINE__, m_uid, result); 
		return 0;
	}
    //设置获得券时间戳
	hlddz.main_rc[index]->command("hset hu:%d coupon_stamp %d",m_uid, time(NULL));
    //累计今日获得券
    hlddz.main_rc[index]->command("hincrby hu:%d coupon_today %d", m_uid, result);
    //xt_log.debug("aaaaaaaaaaaaaa\n");
    return result;
}
    
int Player::couponLimit(void)
{
	int ret = hlddz.main_rc[index]->command("hgetall hu:%d", m_uid);
	if (ret < 0 || hlddz.main_rc[index]->is_array_return_ok() < 0) 
    {
		xt_log.error("couponLimit 1 error.uid:%d\n", m_uid);
		return -1;
	}

    //付费标识
	int paid = hlddz.main_rc[index]->get_value_as_int("paid");
    //获券时间戳
    int coupon_stamp = hlddz.main_rc[index]->get_value_as_int("coupon_stamp");
    //当天获券数量
    int coupon_today = hlddz.main_rc[index]->get_value_as_int("coupon_today");

    //当日在线时长 分钟
	ret = hlddz.main_rc[index]->command("hget heart:%d %s", m_uid, getTimeYY().c_str());
    long long onlineTimeToday = 0;
    if(ret < 0 || false == hlddz.main_rc[index]->getSingleInt(onlineTimeToday))
    {
		xt_log.error("couponLimit 2 error.uid:%d, time:%s\n", m_uid, getTimeYY().c_str());
    }

    //每日上限
    int limit = 50 + onlineTimeToday  / 10;
    if(paid == 1)
    {
        //当日累计充值 
	    ret = hlddz.main_rc[index]->command("hget payrecord:%d %s", m_uid, getTimeYY().c_str());
        long long paySumToday = 0;
        if(ret < 0 || false == hlddz.main_rc[index]->getSingleInt(paySumToday))
        {
            xt_log.error("couponLimit 3 error.uid:%d, time:%s\n", m_uid, getTimeYY().c_str());
        }
        limit += paySumToday * 100 * 0.25;
    }
    if(coupon_today != 0 && !isToday(coupon_stamp))
    {
        coupon_today = 0; 
    }

    return max(limit - coupon_today, 0);
}
    
bool Player::isToday(int stamp)
{
    //stamp
    long int nowtime = static_cast<long int>(time(NULL));
    int tmp1 = (nowtime + 28800) / 86400;
    int tmp2 = (stamp + 28800) / 86400;
    return tmp1 == tmp2;
}

string Player::getTimeYY(void)
{
    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);

    int day = aTime->tm_mday;
    int month = aTime->tm_mon + 1; // Month is 0 - 11, add 1 to get a jan-dec 1-12 concept
    int year = aTime->tm_year + 1900; // Year is # years since 1900
    char timebuff[16];
    sprintf(timebuff, "%.4d-%.2d-%.2d", year, month, day);
    //xt_log.debug("%s\n", timebuff);
    return string(timebuff);
}

