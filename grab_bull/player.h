#ifndef _PLAYER_H_
#define _PLAYER_H_

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

#include <json/json.h>
#include <ev.h>

#include "XtHoleCards.h"

class Client;
//封装玩家信息
class Player
{		
public:
	int 				index;      //redis数组下标
	// table router information     
	int					vid;        //场馆id
	int					tid;        //桌子id
	int					seatid;     //坐位id

	// player information 
	int                 uid;       
	std::string			skey;
	std::string			name;
	int					sex;
	std::string			avatar;        ///头像
	std::string			birthday;
	std::string			zone;
	int					exp;
	int					rmb;
	int					money;         ///金币
	int					coin;
	int					total_board;   ///总次数
	int					total_win;     ///赢场数
	int					pcount;        //今日对局数play count (玩N局奖励，领钱时reset)
	int					vtime;
	int					vlevel;
	std::string			ps;

	///[+++ 2016-4-13
	int                 max_win_money;    //最大赢取 
	int                 best_board;       //最大牌类型
	std::string         best_board_detail;//牌详情，格式为以逗号分隔的16进制数(01,02,03,04,05) 
	///+++]

	// connect to client
	Client              *client;       ///通讯客户端
	
	int					idle_count;    ///闲置无操作次数
	
	int 				logout_type;
	int					time_cnt;

	int                 stand_up;     //起身离开桌子             
	
private:
    ev_timer			_offline_timer;        ///离线timer
    ev_tstamp			_offline_timeout;      ///超时时长

public:
	Player();
	virtual ~Player();
	void set_client(Client *c);
	int init();
	void reset();
	int update_info();
	int set_money(int value);
	int incr_money(int type, int value);
	int incr_coin(int value);
	int incr_exp(int value);
	int incr_rmb(int value);
	int incr_total_board(int vid, int value);
	int incr_total_win(int vid, int value);
	int incr_pcount(int value);

	void start_offline_timer();
	void stop_offline_timer();
	static void offline_timeout(struct ev_loop *loop, ev_timer *w, int revents);

	int eventlog_update(int type, int value);  // cfc add 20140102
	int interaction_emotion(int price);        // cfc add 20140310
	int incr_achievement_count(int my_vid, int type, int value); // cfc add 20140220
	int incr_achievement_card(int type, int value);  // cfc add 20140220


	///+++更新最大赢取
	int update_max_win_money(int value);

	//+++更新最好牌
	int update_best_board(int card_type, std::string card_detail);

};

#endif
