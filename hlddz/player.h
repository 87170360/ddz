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
    bool isRobot(void) { return m_uid < XT_ROBOT_UID_MAX; }
    //统计玩家比赛次数和胜场
    void keepTotal(bool win);
    //破产补助
    void allowance(void);

public:
	int 				index;
	// table info
	int					m_tid;
	unsigned int		m_seatid;

	// player information
	int                 m_uid;
	std::string			m_skey;
	std::string			m_name;
	std::string			m_avatar;
	int					m_sex;
	int					m_money;
	int					m_level;
    //补助领取剩余次数
    int                 m_allowance_num;
    //补助领取时间戳
    time_t                 m_allowance_stamp;

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
