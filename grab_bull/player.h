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
//��װ�����Ϣ
class Player
{		
public:
	int 				index;      //redis�����±�
	// table router information     
	int					vid;        //����id
	int					tid;        //����id
	int					seatid;     //��λid

	// player information 
	int                 uid;       
	std::string			skey;
	std::string			name;
	int					sex;
	std::string			avatar;        ///ͷ��
	std::string			birthday;
	std::string			zone;
	int					exp;
	int					rmb;
	int					money;         ///���
	int					coin;
	int					total_board;   ///�ܴ���
	int					total_win;     ///Ӯ����
	int					pcount;        //���նԾ���play count (��N�ֽ�������Ǯʱreset)
	int					vtime;
	int					vlevel;
	std::string			ps;

	///[+++ 2016-4-13
	int                 max_win_money;    //���Ӯȡ 
	int                 best_board;       //���������
	std::string         best_board_detail;//�����飬��ʽΪ�Զ��ŷָ���16������(01,02,03,04,05) 
	///+++]

	// connect to client
	Client              *client;       ///ͨѶ�ͻ���
	
	int					idle_count;    ///�����޲�������
	
	int 				logout_type;
	int					time_cnt;

	int                 stand_up;     //�����뿪����             
	
private:
    ev_timer			_offline_timer;        ///����timer
    ev_tstamp			_offline_timeout;      ///��ʱʱ��

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


	///+++�������Ӯȡ
	int update_max_win_money(int value);

	//+++���������
	int update_best_board(int card_type, std::string card_detail);

};

#endif
