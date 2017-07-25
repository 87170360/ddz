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
    //添加经验
    void addExp(int exp);
    //升级
    bool levelUp(void);
    //获取升级奖励
    int upMoney(void);
    //最大赢局的钱
    void updateTopMoney(int money);
    //最大赢局倍数
    void updateTopCount(int count);
    //兑换券处理
    int coupon(int score); 
    //话费券处理 type = 0 金币 1 话费
    bool communication(void);
    //获取年月日yyyy-mm-dd
    string getTimeYY(void);
    //使用记牌器
    bool useRecored(void);
    //刷新配置的牌
    void updateConfigCard(void);

private:
    //兑换券上限
    int couponLimit(void);
    bool isToday(int stamp);

public:
	int 				index;
	// table info
	int					m_tid;
	unsigned int		m_seatid;
    int                 m_table_count;

	// player information
	int                 m_uid;
	std::string			m_skey;
	std::string			m_name;
	std::string			m_avatar;
	int					m_sex;
	int					m_money;
	int					m_level;
    int                 m_exp;
    //单局赢过的最多钱
    int                 m_top_money;
    //单局赢过的最到倍数
    int                 m_top_count;
    //指定必发的牌
    std::string         m_config_card;

	// connect to client
	Client              *client;
	int					idle_count;

    //可领取的奖励
    int                 m_type; //0 金币 1 话费券
    float                 m_num;  //数量
	
private:
    ev_timer			_offline_timer;
    ev_tstamp			_offline_timeout;
};

#endif
