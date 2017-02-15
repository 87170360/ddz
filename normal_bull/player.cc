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

#include "bull.h"
#include "log.h"
#include "proto.h"
#include "table.h"
#include "player.h"
#include "client.h"
#include "game.h"
#include "eventlog.h"  // cfc add 20140102

extern Bull gBull;
extern Log xt_log;

Player::Player() :
_offline_timeout(60 * 1)  ///离线时间60s，删除掉
{
	_offline_timer.data = this;
	ev_timer_init(&_offline_timer, Player::offline_timeout, _offline_timeout, 0);

	stand_up = 0;
}

Player::~Player()
{
	ev_timer_stop(gBull.loop, &_offline_timer);
	if (client)
		client->player = NULL;
}

void Player::set_client(Client *c)
{
	client = c;
	uid    = c->uid;
	index  = uid % gBull.main_size;     //由哪个redis库处理，减轻数据库高并发访问

	// maybe init by init_table in game.cc
	vid = c->vid;
	client->player = this;   ///Client也保存下player信息

	stand_up = 0;   //重置为0(离开就回来了)
	
}


///从redis库中读取玩家信息
int Player::init()
{
	reset();

	update_info();

	///如果是机器玩家，随机设置金币2万左右
	if(uid < XT_ROBOT_UID_MAX)   
	{
		set_money(rand()%10000+20000);
	}
	else 
	{
		if (gBull.conf["tables"]["max_money"].asInt() == 0)   ///无上限
		{
			if (money < gBull.conf["tables"]["min_money"].asInt()) 
			{	///玩家金币额度小于下限，报错退出
				xt_log.error("player init uid[%d] money is not fit.\n", uid);
				return -1;
			}
		} 
		else 
		{
			if (money < gBull.conf["tables"]["min_money"].asInt() || money >= gBull.conf["tables"]["max_money"].asInt()) 
			{
				xt_log.error("player init uid[%d] money is not fit.\n", uid);
				return -1;
			}
		}
	}

	///桌子id初始为-1;
	tid = -1;
	///没响应次数
	idle_count = 0;

	return 0;
}

///玩家信息复位
void Player::reset()
{
	logout_type = 0;
	stop_offline_timer();
}

 ///更新玩家信息
int Player::update_info()
{
	int ret = gBull.main_rc[index]->command("hgetall hu:%d", uid);
	if (ret < 0) {
		xt_log.error("update info error, because get player infomation error.\n");
		return -1;
	}

	if (gBull.main_rc[index]->is_array_return_ok() < 0) {
		xt_log.error("update info error, because get player infomation error.\n");
		return -1;
	}

	skey = gBull.main_rc[index]->get_value_as_string("token");
	name = gBull.main_rc[index]->get_value_as_string("nickName");
	sex = gBull.main_rc[index]->get_value_as_int("sex");
	avatar = gBull.main_rc[index]->get_value_as_string("avatar");
	exp = gBull.main_rc[index]->get_value_as_int("exp");
	rmb = gBull.main_rc[index]->get_value_as_int("rmb");
	money = gBull.main_rc[index]->get_value_as_int("money");
	coin = gBull.main_rc[index]->get_value_as_int("coin");
	total_board = gBull.main_rc[index]->get_value_as_int("totalBoard");
	total_win = gBull.main_rc[index]->get_value_as_int("totalWin");
	pcount = gBull.main_rc[index]->get_value_as_int("playCount");
	vlevel = gBull.main_rc[index]->get_value_as_int("vlevel");
	ps = gBull.main_rc[index]->get_value_as_string("ps");
	max_win_money     = gBull.main_rc[index]->get_value_as_int("maxWinMoney");
	best_board        = gBull.main_rc[index]->get_value_as_int("bestBoard");
	best_board_detail = gBull.main_rc[index]->get_value_as_string("bestBoardDetail");
	
	return 0;
}

///+++更新最大赢取
int Player::update_max_win_money(int value)
{
	if (max_win_money < value)
	{
		int ret=0;
		ret=gBull.main_rc[index]->command("hset hu:%d maxWinMoney %d",uid,value);
		if(ret<0)
		{
			xt_log.error("set maxWinMoney error.\n");
			return -1;
		}

		max_win_money=value;
	}
	return 0;
}


//+++更新最好牌
int Player::update_best_board(int card_type, std::string card_detail)
{
	//更新历史最大牌型
	if (card_type > best_board)
	{
		int ret=0;
		//牌型
		ret=gBull.main_rc[index]->command("hset hu:%d bestBoard %d",uid, card_type);
		if(ret<0)
		{
			xt_log.error("set bestBoard error.\n");
			return -1;
		}
		//更新最好牌型
		best_board = card_type;

		//详情
		ret=gBull.main_rc[index]->command("hset hu:%d bestBoardDetail %s",uid, card_detail.c_str());
		if(ret<0)
		{
			xt_log.error("set bestBoardDetail error.\n");
			return -1;
		}

		//更新详情
		best_board_detail = card_detail;

        xt_log.info("Func[%s] After set best_board[%d], best_board_detail[%s]\n", 
			__FUNCTION__,best_board, best_board_detail.c_str());

	}
	return 0;
}




///设置金币数
int Player::set_money(int value)
{
	int ret=0;
	ret=gBull.main_rc[index]->command("hset hu:%d money %d",uid,value);
	if(ret<0)
	{
		xt_log.error("set money error.\n");
		return -1;
	}

	money=value;
	return 0;
}

/*type 0:加钱
       1:减钱
*/
int Player::incr_money(int type, int value)
{
	if (value == 0) {
		return 0;
	}
	int ret;
	if (type == 0) {
		ret = gBull.main_rc[index]->command("hincrby hu:%d money %d", uid, value);
	} else {
		ret = gBull.main_rc[index]->command("hincrby hu:%d money -%d", uid, value);     ///---扣钱
	}
	if (ret < 0) {
		xt_log.error("incr money error.\n");
		return -1;
	}

	xt_log.info("Func[%s] incr money uid[%d] value[%d] old[%d] new[%d].\n", 
		__FUNCTION__, uid, value, money, gBull.main_rc[index]->reply->integer);

	money = gBull.main_rc[index]->reply->integer; //update the money.

	return 0;
}

int Player::incr_coin(int value)
{
	if (value == 0) {
		return 0;
	}
	int ret = gBull.main_rc[index]->command("hincrby hu:%d coin %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr coin error.\n");
		return -1;
	}
	xt_log.info("incr coin uid[%d] old[%d] new[%d].\n", uid, coin, gBull.main_rc[index]->reply->integer);
	coin = gBull.main_rc[index]->reply->integer; //update the coin.

	return 0;
}

///增加经验值
int Player::incr_exp(int value)
{
	if (value == 0) 
	{
		return 0;
	}

	int ret;
	ret = gBull.main_rc[index]->command("hincrby hu:%d exp %d", uid, value);
	if (ret < 0) 
	{
		xt_log.error("incr exp error.\n");
		return -1;
	}
	xt_log.info("incr exp uid[%d] old[%d] new[%d].\n", 
		uid, exp, gBull.main_rc[index]->reply->integer);

	exp = gBull.main_rc[index]->reply->integer; //update the exp.

	return 0;
}

int Player::incr_rmb(int value)
{
	if (value == 0) 
	{
		return 0;
	}

	int ret;
	ret = gBull.main_rc[index]->command("hincrby hu:%d rmb %d", uid, value);
	if (ret < 0) 
	{
		xt_log.error("incr rmb error.\n");
		return -1;
	}
	xt_log.info("incr rmb uid[%d] old[%d] new[%d].\n", 
		uid, rmb, gBull.main_rc[index]->reply->integer);

	rmb = gBull.main_rc[index]->reply->integer; //update the rmb.

	return 0;
}

///累计总的对局数
int Player::incr_total_board(int vid, int value)
{
	if (value == 0) 
	{
		return 0;
	}
	int ret;
	ret = gBull.main_rc[index]->command("hincrby hu:%d total_board %d", uid, value);   ///累加总局数
	if (ret < 0) 
	{
		xt_log.error("incr total board error.\n");
		return -1;
	}
	total_board = gBull.main_rc[index]->reply->integer; //update the total_board.

	return 0;
}

///累计总的赢场数
int Player::incr_total_win(int vid, int value)
{
	if (value == 0) 
	{
		return 0;
	}
	int ret;
	ret = gBull.main_rc[index]->command("hincrby hu:%d total_win %d", uid, value);

	if (ret < 0) 
	{
		xt_log.error("incr total win error.\n");
		return -1;
	}
	total_win = gBull.main_rc[index]->reply->integer; //update the total_win.

	return 0;
}

///累计对局数 play count
int Player::incr_pcount(int value)
{
	if (value == 0) 
	{
		return 0;
	}

	int ret;
	ret = gBull.main_rc[index]->command("hincrby hu:%d play_count %d", uid, value);

	if (ret < 0) 
	{
		xt_log.error("incr pcount error.\n");
		return -1;
	}
	pcount = gBull.main_rc[index]->reply->integer; //update the total_board.

	return 0;
}

int Player::incr_achievement_count(int my_vid, int type, int value)
{
	if (value == 0) 
	{
		return 0;
	}

	int ret;
	switch (my_vid) 
	{
		case 1:
			ret = gBull.main_rc[index]->command("hincrby st:%d room1 %d", uid, value);
			break;
		case 2:
			ret = gBull.main_rc[index]->command("hincrby st:%d room2 %d", uid, value);
			break;
		case 3:
			ret = gBull.main_rc[index]->command("hincrby st:%d room3 %d", uid, value);
			break;
		case 4:
			ret = gBull.main_rc[index]->command("hincrby st:%d room4 %d", uid, value);
			break;
		case 5:
			ret = gBull.main_rc[index]->command("hincrby st:%d room5 %d", uid, value);
			break;
		case 6:
			if (type == 0)
				ret = gBull.main_rc[index]->command("hincrby st:%d playoffs_win %d", uid, value);
			ret = gBull.main_rc[index]->command("hincrby st:%d playoffs_count %d", uid, value);
			break;
		case 7:
			if (type == 0)
				ret = gBull.main_rc[index]->command("hincrby st:%d winner_win %d", uid, value);
			ret = gBull.main_rc[index]->command("hincrby st:%d winner_count %d", uid, value);
			break;
		case 8:
			ret = 0;
			break;
		case 901:
			if (type == 0)
				ret = gBull.main_rc[index]->command("hincrby st:%d daily_props_win %d", uid, value);
			ret = gBull.main_rc[index]->command("hincrby st:%d daily_props_play %d", uid, value);
			ret = gBull.main_rc[index]->command("hincrby st:%d room201 %d", uid, value);
			break;
		case 902:
			if (type == 0)
				ret = gBull.main_rc[index]->command("hincrby st:%d daily_props_win %d", uid, value);
			ret = gBull.main_rc[index]->command("hincrby st:%d daily_props_play %d", uid, value);
			ret = gBull.main_rc[index]->command("hincrby st:%d room202 %d", uid, value);
			break;
		case 903:
			if (type == 0)
				ret = gBull.main_rc[index]->command("hincrby st:%d daily_props_win %d", uid, value);
			ret = gBull.main_rc[index]->command("hincrby st:%d daily_props_play %d", uid, value);
			ret = gBull.main_rc[index]->command("hincrby st:%d room203 %d", uid, value);
			break;
		default:
			ret = -1;
			break;
	}

	if (type == 0)
	{
		ret = gBull.main_rc[index]->command("hincrby st:%d daily_win_count %d", uid, value);
	}

	ret = gBull.main_rc[index]->command("hincrby st:%d daily_play_count %d", uid, value);

	if (ret < 0) 
	{
		xt_log.error("incr achievement count error uid[%d] vid[%d] type[%d] value[%d].\n", uid, my_vid, type, value);
		return -1;
	}

	return 0;
}

int Player::incr_achievement_card(int type, int value)
{
	if (value == 0) 
	{
		return 0;
	}

	int ret = 0;
	switch (type) 
	{
		case 1:
			ret = gBull.main_rc[index]->command("hincrby st:%d game_s2 %d", uid, value);
			break;
		case 2:
			ret = gBull.main_rc[index]->command("hincrby st:%d game_s3 %d", uid, value);
			break;
		default:
			break;
	}

	if (ret < 0) 
	{
		xt_log.error("incr achievement card error uid[%d] type[%d] value[%d].\n", uid, type, value);
		return -1;
	}
	return 0;
}

void Player::start_offline_timer()
{
	ev_timer_start(gBull.loop, &_offline_timer);
}

void Player::stop_offline_timer()
{
	ev_timer_stop(gBull.loop, &_offline_timer);
}

///离线超时处理
void Player::offline_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
	/* player logout
	 * remove from offline table */
	Player* self = (Player*)w->data;
	gBull.game->del_player(self);
}

int Player::eventlog_update(int type, int value)
{
	int ret;
	if (type == 0) 
	{
		ret = commit_eventlog(uid, tid, value, money, 200, 2);
	} 
	else if (type == 1) 
	{
		ret = commit_eventlog(uid, tid, -value, money, 200, 2);
	}
	else if (type == 2) 
	{
		ret = commit_eventlog(0, tid, value, -1, 200, 2);
	} 
	else if (type == 3) 
	{
		ret = commit_eventlog(uid, tid, value, coin, 203, 3);
	} 
	else if (type == 4) 
	{
		ret = commit_eventlog(uid, tid, value, rmb, 204, 1);
	} 
	else if (type == 5) 
	{
		ret = commit_eventlog(uid, tid, -value, money, 206, 2);
	}

	if (ret < 0) 
	{
		xt_log.error("eventlog update error type[%d] value[%d].\n", type, value);
		return -1;
	}
	return 0;
}

int Player::interaction_emotion(int price)
{
	if (price == 0) 
	{
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


