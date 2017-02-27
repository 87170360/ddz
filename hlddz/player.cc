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

extern DDZ ddz;
extern Log xt_log;

Player::Player() :
_offline_timeout(60 * 1)
{
	_offline_timer.data = this;
	ev_timer_init(&_offline_timer, Player::offline_timeout, _offline_timeout, 0);
}

Player::~Player()
{
	ev_timer_stop(ddz.loop, &_offline_timer);
	if (client)
		client->player = NULL;
}

void Player::set_client(Client *c)
{
	client = c;
	uid = c->uid;
	index = uid % ddz.main_size;
	// maybe init by init_table in game.cc
	vid = c->vid;
	zid = c->zid;
	client->player = this;
}

int Player::init()
{
	// todo get from redis db.
	// player information
	reset();
	int ret = ddz.main_rc[index]->command("hgetall hu:%d", uid);
	if (ret < 0) {
		xt_log.error("player init error 1, because get player infomation error.uid:%d\n", uid);
		return -1;
	}
	
	if (ddz.main_rc[index]->is_array_return_ok() < 0) {
		xt_log.error("player init error 2, because get player infomation error.uid:%d\n", uid);
		return -1;
	}
	
	skey = ddz.main_rc[index]->get_value_as_string("skey");
	name = ddz.main_rc[index]->get_value_as_string("name");
	sex = ddz.main_rc[index]->get_value_as_int("sex");
	avatar = ddz.main_rc[index]->get_value_as_string("avatar");
	exp = ddz.main_rc[index]->get_value_as_int("exp");
	rmb = ddz.main_rc[index]->get_value_as_int("rmb");
	money = ddz.main_rc[index]->get_value_as_int("money");
	coin = ddz.main_rc[index]->get_value_as_int("coin");
	total_board = ddz.main_rc[index]->get_value_as_int("total_board");
	total_win = ddz.main_rc[index]->get_value_as_int("total_win");
	pcount = ddz.main_rc[index]->get_value_as_int("play_count");
//	vtime = ddz.main_rc[index]->get_value_as_int("vtime");
	vlevel = ddz.main_rc[index]->get_value_as_int("vlevel");
	ps = ddz.main_rc[index]->get_value_as_string("ps");
	// cfc add 20140321
	forbidden_card = ddz.main_rc[index]->get_value_as_int("forbidden_card");
	change_card = ddz.main_rc[index]->get_value_as_int("change_card");
	double_card = ddz.main_rc[index]->get_value_as_int("double_card");

	if(uid<XT_ROBOT_UID_MAX)
	{
		set_money(rand()%10000+20000);
	}
	else 
	{
		if (ddz.conf["tables"]["max_money"].asInt() == 0) 
		{
			if (money < ddz.conf["tables"]["min_money"].asInt()) 
			{
				xt_log.error("player init uid[%d] money is not fit.\n", uid);
				return -1;
			}
		} 
		else 
		{
			if (money < ddz.conf["tables"]["min_money"].asInt() || money >= ddz.conf["tables"]["max_money"].asInt()) 
			{
				xt_log.error("player init uid[%d] money is not fit.\n", uid);
				return -1;
			}
		}
	}

	tid = -1;
	idle_count= 0;

	return 0;
}

void Player::reset()
{
	logout_type = 0;
	time_cnt = 0;
	stop_offline_timer();
}

int Player::update_info()
{
	int ret = ddz.main_rc[index]->command("hgetall hu:%d", uid);
	if (ret < 0) {
		xt_log.error("update info error, because get player infomation error.\n");
		return -1;
	}

	if (ddz.main_rc[index]->is_array_return_ok() < 0) {
		xt_log.error("update info error, because get player infomation error.\n");
		return -1;
	}

	exp = ddz.main_rc[index]->get_value_as_int("exp");
	rmb = ddz.main_rc[index]->get_value_as_int("rmb");
	money = ddz.main_rc[index]->get_value_as_int("money");
	coin = ddz.main_rc[index]->get_value_as_int("coin");
	total_board = ddz.main_rc[index]->get_value_as_int("total_board");
	total_win = ddz.main_rc[index]->get_value_as_int("total_win");

	// cfc add 20140321
	forbidden_card = ddz.main_rc[index]->get_value_as_int("forbidden_card");
	change_card = ddz.main_rc[index]->get_value_as_int("change_card");
	double_card = ddz.main_rc[index]->get_value_as_int("double_card");

	return 0;
}


int Player::set_money(int value)
{
	int ret=0;
	ret=ddz.main_rc[index]->command("hset hu:%d money %d",uid,value);
	if(ret<0)
	{
		xt_log.error("set money error.\n");
		return -1;
	}

	money=value;
	return 0;
}


int Player::incr_money(int type, int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	if (type == 0) {
		ret = ddz.main_rc[index]->command("hincrby hu:%d money %d", uid, value);
	} else {
		ret = ddz.main_rc[index]->command("hincrby hu:%d money -%d", uid, value);
	}
	if (ret < 0) {
		xt_log.error("incr money error.\n");
		return -1;
	}
	xt_log.info("incr money uid[%d] value[%d] old[%d] new[%d].\n", uid, value, money, ddz.main_rc[index]->reply->integer);
	money = ddz.main_rc[index]->reply->integer; //update the money.

	return 0;
}

int Player::incr_coin(int value)
{
	if (value == 0) {
		return 0;
	}
	int ret = ddz.main_rc[index]->command("hincrby hu:%d coin %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr coin error.\n");
		return -1;
	}
	xt_log.info("incr coin uid[%d] old[%d] new[%d].\n", uid, coin, ddz.main_rc[index]->reply->integer);
	coin = ddz.main_rc[index]->reply->integer; //update the coin.

	return 0;
}

int Player::incr_exp(int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	ret = ddz.main_rc[index]->command("hincrby hu:%d exp %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr exp error.\n");
		return -1;
	}
	xt_log.info("incr exp uid[%d] old[%d] new[%d].\n", uid, exp, ddz.main_rc[index]->reply->integer);
	exp = ddz.main_rc[index]->reply->integer; //update the exp.

	return 0;
}

int Player::incr_rmb(int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	ret = ddz.main_rc[index]->command("hincrby hu:%d rmb %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr rmb error.\n");
		return -1;
	}
	xt_log.info("incr rmb uid[%d] old[%d] new[%d].\n", uid, rmb, ddz.main_rc[index]->reply->integer);
	rmb = ddz.main_rc[index]->reply->integer; //update the rmb.

	return 0;
}

int Player::incr_total_board(int vid, int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	ret = ddz.main_rc[index]->command("hincrby hu:%d total_board %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr total board error.\n");
		return -1;
	}
	total_board = ddz.main_rc[index]->reply->integer; //update the total_board.

	return 0;
}

int Player::incr_total_win(int vid, int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	ret = ddz.main_rc[index]->command("hincrby hu:%d total_win %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr total win error.\n");
		return -1;
	}
	total_win = ddz.main_rc[index]->reply->integer; //update the total_win.

	return 0;
}

int Player::incr_pcount(int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	ret = ddz.main_rc[index]->command("hincrby hu:%d play_count %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr pcount error.\n");
		return -1;
	}
	pcount = ddz.main_rc[index]->reply->integer; //update the total_board.

	return 0;
}

int Player::incr_achievement_count(int my_vid, int type, int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	switch (my_vid) {
		case 1:
			ret = ddz.main_rc[index]->command("hincrby st:%d room1 %d", uid, value);
			break;
		case 2:
			ret = ddz.main_rc[index]->command("hincrby st:%d room2 %d", uid, value);
			break;
		case 3:
			ret = ddz.main_rc[index]->command("hincrby st:%d room3 %d", uid, value);
			break;
		case 4:
			ret = ddz.main_rc[index]->command("hincrby st:%d room4 %d", uid, value);
			break;
		case 5:
			ret = ddz.main_rc[index]->command("hincrby st:%d room5 %d", uid, value);
			break;
		case 6:
			if (type == 0)
				ret = ddz.main_rc[index]->command("hincrby st:%d playoffs_win %d", uid, value);
			ret = ddz.main_rc[index]->command("hincrby st:%d playoffs_count %d", uid, value);
			break;
		case 7:
			if (type == 0)
				ret = ddz.main_rc[index]->command("hincrby st:%d winner_win %d", uid, value);
			ret = ddz.main_rc[index]->command("hincrby st:%d winner_count %d", uid, value);
			break;
		case 8:
			ret = 0;
			break;
		case 901:
			if (type == 0)
				ret = ddz.main_rc[index]->command("hincrby st:%d daily_props_win %d", uid, value);
			ret = ddz.main_rc[index]->command("hincrby st:%d daily_props_play %d", uid, value);
			ret = ddz.main_rc[index]->command("hincrby st:%d room201 %d", uid, value);
			break;
		case 902:
			if (type == 0)
				ret = ddz.main_rc[index]->command("hincrby st:%d daily_props_win %d", uid, value);
			ret = ddz.main_rc[index]->command("hincrby st:%d daily_props_play %d", uid, value);
			ret = ddz.main_rc[index]->command("hincrby st:%d room202 %d", uid, value);
			break;
		case 903:
			if (type == 0)
				ret = ddz.main_rc[index]->command("hincrby st:%d daily_props_win %d", uid, value);
			ret = ddz.main_rc[index]->command("hincrby st:%d daily_props_play %d", uid, value);
			ret = ddz.main_rc[index]->command("hincrby st:%d room203 %d", uid, value);
			break;
		default:
			ret = -1;
			break;
	}
	if (type == 0)
		ret = ddz.main_rc[index]->command("hincrby st:%d daily_win_count %d", uid, value);
	ret = ddz.main_rc[index]->command("hincrby st:%d daily_play_count %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr achievement count error uid[%d] vid[%d] type[%d] value[%d].\n", uid, my_vid, type, value);
		return -1;
	}

	return 0;
}

int Player::incr_achievement_card(int type, int value)
{
	if (value == 0) {
		return 0;
	}
	int ret = 0;
	switch (type) {
		case 1:
			ret = ddz.main_rc[index]->command("hincrby st:%d game_s2 %d", uid, value);
			break;
		case 2:
			ret = ddz.main_rc[index]->command("hincrby st:%d game_s3 %d", uid, value);
			break;
		default:
			break;
	}
	if (ret < 0) {
		xt_log.error("incr achievement card error uid[%d] type[%d] value[%d].\n", uid, type, value);
		return -1;
	}
	return 0;
}

void Player::start_offline_timer()
{
	ev_timer_start(ddz.loop, &_offline_timer);
}

void Player::stop_offline_timer()
{
	ev_timer_stop(ddz.loop, &_offline_timer);
}

void Player::offline_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
	/* player logout
	 * remove from offline table */
	Player* self = (Player*)w->data;
	ddz.game->del_player(self);
}

int Player::eventlog_update(int type, int value)
{
	int ret;
	if (type == 0) {
		ret = commit_eventlog(uid, tid, value, money, 200, 2);
	} else if (type == 1) {
		ret = commit_eventlog(uid, tid, -value, money, 200, 2);
	} else if (type == 2) {
		ret = commit_eventlog(0, tid, value, -1, 200, 2);
	} else if (type == 3) {
		ret = commit_eventlog(uid, tid, value, coin, 203, 3);
	} else if (type == 4) {
		ret = commit_eventlog(uid, tid, value, rmb, 204, 1);
	} else if (type == 5) {
		ret = commit_eventlog(uid, tid, -value, money, 206, 2);
	}
	if (ret < 0) {
		xt_log.error("eventlog update error type[%d] value[%d].\n", type, value);
		return -1;
	}
	return 0;
}

int Player::interaction_emotion(int price)
{
	if (price == 0) {
		return 0;
	}

	int value;
	if (vlevel == 3) {
		value = price * (float)0.8;
	} else if (vlevel == 4) {
		value = price * (float)0.5;
	} else if (vlevel == 5) {
		value = price * (float)0.1;
	} else {
		value = price;
	}
	incr_money(1, value);
	eventlog_update(5, value);

	return 0;
}

int Player::incr_forbidden_card(int value)
{
	int ret;
	ret = ddz.main_rc[index]->command("hincrby hu:%d forbidden_card -%d", uid, value);
	if (ret < 0) {
		xt_log.error("incr forbidden_card error.\n");
		return -1;
	}
	xt_log.info("incr change_card uid[%d] old[%d] new[%d].\n", uid, forbidden_card, ddz.main_rc[index]->reply->integer);
	forbidden_card = ddz.main_rc[index]->reply->integer;

	commit_eventlog(uid,tid,-1,forbidden_card,200,4);

	return 0;
}

int Player::incr_change_card(int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	ret = ddz.main_rc[index]->command("hincrby hu:%d change_card -%d", uid, value);
	if (ret < 0) {
		xt_log.error("incr change_card error.\n");
		return -1;
	}
	xt_log.info("incr change_card uid[%d] old[%d] new[%d].\n", uid, change_card, ddz.main_rc[index]->reply->integer);
	change_card = ddz.main_rc[index]->reply->integer;
	commit_eventlog(uid,tid,-1,change_card,200,5);

	return 0;
}

int Player::incr_double_card(int value)
{
	int ret;
	ret = ddz.main_rc[index]->command("hincrby hu:%d double_card -%d", uid, value);
	if (ret < 0) {
		xt_log.error("incr double_card error.\n");
		return -1;
	}
	xt_log.info("incr change_card uid[%d] old[%d] new[%d].\n", uid, double_card, ddz.main_rc[index]->reply->integer);
	double_card = ddz.main_rc[index]->reply->integer;
	commit_eventlog(uid,tid,-1,double_card,200,6);
	return 0;
}
