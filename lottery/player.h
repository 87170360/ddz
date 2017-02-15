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
#include <queue>

#include <json/json.h>
#include <ev.h>

class Client;

class Player {
public:
	int          index;
	// player information
	int          uid;
	std::string  skey;
	std::string  name;
	int          sex;
	std::string  birthday;
	std::string  zone;
	std::string  contact;
	std::string  ps;
	std::string  avatar;
	int          exp;
	int          rmb;
	int          money;
	int          coin;
	int          total_board;
	int          total_win;
	std::string  best_cards;
	int          pcount;
	int          vtime;
	int          vlevel;

	Client       *client;

private:
	ev_timer     _offline_timer;
	ev_tstamp    _offline_timeout;

public:
	Player();
	virtual ~Player();

	int init();
	void set_client(Client *c);

	void start_offline_timer();
	void stop_offline_timer();
	static void offline_timeout(struct ev_loop *loop, ev_timer *w, int revents);
};


#endif  /*_PLAYER_H_*/
