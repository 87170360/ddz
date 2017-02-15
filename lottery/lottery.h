/*
 * shishicai.h
 *
 *  Created on: 2014年2月22日
 *      Author: chenfc
 */

#ifndef SHSHICAI_H_
#define SHSHICAI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ev++.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <ev.h>
#include <json/json.h>

#include "jpacket.h"

class Player;
class Client;
class Server;

enum CardType
{
	CARD_TYPE_ERROR   = 0,			// 错误类型
	CARD_TYPE_BAOZI   = 1,			// 豹子
	CARD_TYPE_SHUNJIN = 2,		    // 顺金
	CARD_TYPE_JINHUA  = 3,			// 金花
	CARD_TYPE_SHUNZI  = 4,		    // 顺子
	CARD_TYPE_DUIZI   = 5,			// 对子
	CARD_TYPE_DANPAI  = 6,			// 单牌
	CARD_TYPE_TESHU	  = 7,	        // 特殊
};

class Lottery {
public:
	int issue;  // 彩票期号
	int lottery_money;  // 每注钱
	int total_money;  // 总奖池
	int total_count;  // 总次数
	int single_money;  // 单注中奖
	int lstate;  // 状态 0:不能下注 1:下注
	int card_type;  // card type 1~6
	int awards_count;  // 中奖的人数

	std::map<int, Client*>  action_client;  //活动的client
	std::map<int, int> awards_client;    // 所有中奖 <uid, count>
	std::map<int, std::map<int, int> > bet_client;  //所有下注的 <card_type <uid, count> >

	Server *server;

public:
	Lottery();
	~Lottery();

	int init(Server *s);
	int start();
	int handler_betting(Client *client, int type,int bet_nu);  // 下注
	int handler_action(Client *client, int action_type);
	int handler_login(Client *client);
	int handler_del_action(int uid);

	static void wait_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	static void lottery_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	static void update_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);

private:
	ev_timer wait_timer;
	ev_tstamp wait_timer_stamp;

	ev_timer lottery_timer;
	ev_tstamp lottery_timer_stamp;

	ev_timer update_timer;
	ev_tstamp update_timer_stamp;

	// 0  方块  1 梅花   2  红桃    3黑桃
	std::vector<std::vector<int> > all_cards;
	std::vector<int> c_diamonds;
	std::vector<int> c_clubs;
	std::vector<int> c_hearts;
	std::vector<int> c_spades;
	std::list<int> c_awards;  // 中奖的三张牌

private:
	void reset();
	int start_bet();
	int stop_bet();
	int handler_awards();  // 发奖

	int action_broadcast(const std::string &packet);
	int send_awards_unicast(int uid, int value);

	int shuffle_card();
	int handler_baozi();
	int handler_shunjin();
	int handler_jinhua();
	int handler_shunzi();
	int handler_duizi();
	int handler_gaopai();
	int random_type(int start, int end, int seed);

	int get_betting_remain();
	int get_wait_remain();
	int action_update();

	int incr_money(int uid, int value);
	int eventlog_update(int uid, int value, int money);
	int incr_achievement_count(int uid, int value);

};



#endif /* SHSHICAI_H_ */
