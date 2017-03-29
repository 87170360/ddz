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
	Player();
	virtual ~Player();

	void set_client(Client *c);
	int init();
	void reset(void);
	int update_info();
	int set_money(int value);
	void start_offline_timer();
	void stop_offline_timer();
	static void offline_timeout(struct ev_loop *loop, ev_timer *w, int revents);

    void changeMoney(int value);

public:
	int 				index;
	// table info
	int					m_tid;
	unsigned int		m_seatid;

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

	// connect to client
	Client              *client;
	
	int					idle_count;
	
	int 				logout_type;
	int					time_cnt;
	
private:
    ev_timer			_offline_timer;
    ev_tstamp			_offline_timeout;

};

#endif
