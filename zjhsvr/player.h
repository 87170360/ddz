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

class Player
{		
public:
	int 				index;
	// table router information
	int					vid;
	int					zid;
	int					tid;
	int					seatid;
	// player information
	int                 uid;
	std::string			skey;
	std::string			name;
	int					sex;
	std::string			avatar;
	std::string			birthday;
	std::string			zone;
	int					exp;
	int					rmb;
	int					money;
	int					coin;
	int					total_board;
	int					total_win;
	int					pcount;
	int					vtime;
	int					vlevel;
	std::string			ps;

	// cfc add 20140321
	int forbidden_card;
	int change_card;
	int double_card;

	// connect to client
	Client              *client;
	
	int					idle_count;
	
	/*
	int					ready;
	int					see;
	int					role; // 0 : Player 1 : Dealer
	HoleCards			hole_cards;
	*/
	int 				logout_type;
	int					time_cnt;
	
private:
    ev_timer			_offline_timer;
    ev_tstamp			_offline_timeout;

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
	int incr_achievement_count(int my_vid, int type, int value); // cfc add 20140220
	int incr_achievement_card(int type, int value);  // cfc add 20140220
	void start_offline_timer();
	void stop_offline_timer();
	static void offline_timeout(struct ev_loop *loop, ev_timer *w, int revents);
	int eventlog_update(int type, int value);  // cfc add 20140102
	int interaction_emotion(int price);  // cfc add 20140310
	int incr_forbidden_card(int value);  // cfc add 20140321
	int incr_change_card(int value); // cfc add 20140321
	int incr_double_card(int value); // cfc add 20140321
private:

};

#endif
