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
	uid = c->uid;
	index = uid % hlddz.main_size;
	client->player = this;
}

int Player::init()
{
	// player information
	reset();
	int ret = hlddz.main_rc[index]->command("hgetall hu:%d", uid);
	if (ret < 0) {
		xt_log.error("player init error 1, because get player infomation error.uid:%d\n", uid);
		return -1;
	}
	
	if (hlddz.main_rc[index]->is_array_return_ok() < 0) {
		xt_log.error("player init error 2, because get player infomation error.uid:%d\n", uid);
		return -1;
	}
	
	skey = hlddz.main_rc[index]->get_value_as_string("skey");
	name = hlddz.main_rc[index]->get_value_as_string("name");
	sex = hlddz.main_rc[index]->get_value_as_int("sex");
	avatar = hlddz.main_rc[index]->get_value_as_string("avatar");
	exp = hlddz.main_rc[index]->get_value_as_int("exp");
	rmb = hlddz.main_rc[index]->get_value_as_int("rmb");
	money = hlddz.main_rc[index]->get_value_as_int("money");
	coin = hlddz.main_rc[index]->get_value_as_int("coin");
	total_board = hlddz.main_rc[index]->get_value_as_int("total_board");
	total_win = hlddz.main_rc[index]->get_value_as_int("total_win");
	pcount = hlddz.main_rc[index]->get_value_as_int("play_count");
	vlevel = hlddz.main_rc[index]->get_value_as_int("vlevel");
	ps = hlddz.main_rc[index]->get_value_as_string("ps");

	if(uid<XT_ROBOT_UID_MAX)
	{
		set_money(rand()%10000+20000);
	}
	else 
	{
		if (hlddz.conf["tables"]["max_money"].asInt() == 0) 
		{
			if (money < hlddz.conf["tables"]["min_money"].asInt()) 
			{
				xt_log.error("player init uid[%d] money is not fit.\n", uid);
				return -1;
			}
		} 
		else 
		{
			if (money < hlddz.conf["tables"]["min_money"].asInt() || money >= hlddz.conf["tables"]["max_money"].asInt()) 
			{
				xt_log.error("player init uid[%d] money is not fit.\n", uid);
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
	int ret = hlddz.main_rc[index]->command("hgetall hu:%d", uid);
	if (ret < 0) {
		xt_log.error("update info error, because get player infomation error.\n");
		return -1;
	}

	if (hlddz.main_rc[index]->is_array_return_ok() < 0) {
		xt_log.error("update info error, because get player infomation error.\n");
		return -1;
	}

	exp = hlddz.main_rc[index]->get_value_as_int("exp");
	rmb = hlddz.main_rc[index]->get_value_as_int("rmb");
	money = hlddz.main_rc[index]->get_value_as_int("money");
	coin = hlddz.main_rc[index]->get_value_as_int("coin");
	total_board = hlddz.main_rc[index]->get_value_as_int("total_board");
	total_win = hlddz.main_rc[index]->get_value_as_int("total_win");

	return 0;
}

int Player::set_money(int value)
{
	int ret=0;
	ret=hlddz.main_rc[index]->command("hset hu:%d money %d",uid,value);
	if(ret<0)
	{
		xt_log.error("set money error.\n");
		return -1;
	}

	money=value;
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

    int ret = hlddz.main_rc[index]->command("hincrby hu:%d money %d", uid, value);
	if (ret < 0) 
    {
        xt_log.error("%s:%d, changeMoney error. uid:%d, value:%d\n", __FILE__, __LINE__, uid, value); 
		return;
	}
	//xt_log.info("changeMoney uid[%d] value[%d] old[%d] new[%d].\n", uid, value, money, hlddz.main_rc[index]->reply->integer);
    //update the money
	money = hlddz.main_rc[index]->reply->integer; 
}
