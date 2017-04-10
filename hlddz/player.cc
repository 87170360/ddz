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
	m_allowance_stamp = static_cast<time_t>(hlddz.main_rc[index]->get_value_as_int("m_allowance_stamp"));

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
	logout_type = 0;
	time_cnt = 0;
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
    
void Player::allowance(void)
{
    time_t curstamp = time(NULL);
}
