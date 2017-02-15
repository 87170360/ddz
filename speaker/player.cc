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

#include "speaker.h"
#include "log.h"
#include "proto.h"
#include "client.h"
#include "server.h"
#include "player.h"

extern Speaker speaker;
extern Log xt_log;

Player::Player() :
		client(NULL),
		_offline_timeout(20)
{
	_offline_timer.data = this;
	ev_timer_init(&_offline_timer, Player::offline_timeout, _offline_timeout, 0);
}

Player::~Player()
{
	ev_timer_stop(speaker.loop, &_offline_timer);
	if (client) {
		client->player = NULL;
	}
}

int Player::init()
{
    ev_timer_stop(speaker.loop, &_offline_timer);

	int ret = speaker.main_rc[index]->command("hgetall hu:%d", uid);
	if (ret < 0)
	{
		xt_log.debug("player init error, because get player infomation error.\n");
		return -1;
	}

	if (speaker.main_rc[index]->is_array_return_ok() < 0)
	{
		xt_log.debug("player init error, because get player infomation error.\n");
		return -1;
	}

	skey = speaker.main_rc[index]->get_value_as_string("token");
	name = speaker.main_rc[index]->get_value_as_string("userName");
	sex = speaker.main_rc[index]->get_value_as_int("sex");
//	birthday = speaker.main_rc[index]->get_value_as_string("birthday");
//	zone = speaker.main_rc[index]->get_value_as_string("zone");
//	contact = speaker.main_rc[index]->get_value_as_string("contact");
//	ps = speaker.main_rc[index]->get_value_as_string("ps");
//	avatar = speaker.main_rc[index]->get_value_as_string("avatar");
	exp = speaker.main_rc[index]->get_value_as_int("exp");
	money = speaker.main_rc[index]->get_value_as_int("money");
	rmb = speaker.main_rc[index]->get_value_as_int("rmb");
	coin = speaker.main_rc[index]->get_value_as_int("coin");
	total_board = speaker.main_rc[index]->get_value_as_int("totalBoard");
	total_win = speaker.main_rc[index]->get_value_as_int("totalWin");
//	best_cards = speaker.main_rc[index]->get_value_as_string("best_cards");
//	pcount = speaker.main_rc[index]->get_value_as_int("playCount");
//	vtime = speaker.main_rc[index]->get_value_as_int("vtime");
//	vlevel = speaker.main_rc[index]->get_value_as_int("vlevel");

    return 0;
}

void Player::set_client(Client *c)
{
	client = c;
	uid = c->uid;
	index = uid % speaker.main_size;

	client->player = this;
}


void Player::start_offline_timer()
{
	client = NULL;
	ev_timer_start(speaker.loop, &_offline_timer);
	xt_log.debug("uid[%d] start_offline_timer\n", uid);
}

void Player::stop_offline_timer()
{
    ev_timer_stop(speaker.loop, &_offline_timer);
    xt_log.debug("uid[%d] stop_offline_timer\n", uid);
}

void Player::offline_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
	// delete player
}
