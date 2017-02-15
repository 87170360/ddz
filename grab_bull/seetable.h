#ifndef _TABLE_H_
#define _TABLE_H_

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

#include <json/json.h>

#include <map>
#include <set>

#include "XtDeck.h"
#include "XtTypeDeck.h"
#include "XtHoleCards.h"
#include "jpacket.h"

const int GROUP_TIME = 10;       ///---组牌时间
const int GROUP_TIME_EXTRA = 3;  ///---组牌附加时间

class Player;
class Client;

///座位定义
typedef struct
{
	int seatid;                //座位ID
	int occupied;              //是否占用（0:否, 1:占用）
	
	int uid;                   //用户id
	int role;                  //角色 0:闲家, 1:庄家
	int status;                //玩家状态 0:离开 1:玩牌

	unsigned long long  goup_time; //组牌所花时间

	XtHoleCards hole_cards;    //手牌
	XtHoleCards group_cards;   //组好的牌
    Player *player;
	
	///清除坐位上的玩家
    void clear(void)
    {
		occupied = 0;        // 是否被坐下
		uid = 0;             // 用户id
		role = 0;            // 是否是庄家
		status = 0;          // 离开
		player = NULL;
		hole_cards.clear();  // 清空手牌
		group_cards.clear();
		goup_time = GROUP_TIME + GROUP_TIME_EXTRA; //组牌所花时间初始为最大值
    }

	///复位（玩家保留）
    void reset(void)
    {
		role = 0;
		status = 1;         ///玩牌中 		
		hole_cards.clear();
		group_cards.clear();
		goup_time = GROUP_TIME + GROUP_TIME_EXTRA; //组牌所花时间初始为最大值
    }

} Seat;

///游戏状态（游戏场景）
typedef enum
{
	READY = 0,     ///准备开始
	BANKER,        ///随机选庄
	SEND_CARD,     ///发牌
	GROUP_CARD,    ///组牌
	COMPARE,       ///比牌
    END_GAME,      ///结束

	PRE_READY,     ///预备下一回合
	WAIT,          ///等待
} State;

///匹配→抽水→发4张牌→抢庄→非庄下注→发第5张牌→配牌→与庄家比牌→结算
///牌桌（看牌抢庄的table）
class SeeTable
{
public:
    int					   tid;          ///桌子ID
    int             	   vid;          ///场馆ID

    int             	   min_money;    ///最低携带
	int					   max_money;    ///最高携带
	int					   base_money;   ///底注（ 闲家结算=当前房间底分×赢方牌型倍数×时间加成   庄家结算=-∑闲家结算）

	int                    fee;           ///抽水金额（系统对每个玩家扣税）
	int                    lose_exp;      ///输赢的经验值
	int                    win_exp;

	std::map<int, Player*> players;      ///玩家map(uid->Player)
    int             	   cur_players;  ///当前玩家个数
	int					   ready_players;
	
	int					   seat_max;     ///最大坐位数    

	Seat                   seats[5];     // 坐位数组(一张桌子5个位置)
	State				   state;        // game state(即游戏场景)
	int					   dealer;       // 庄家

	unsigned long long     start_group_time;  //组牌开始时间	

	XtTypeDeck			   deck;              ///---发牌平台
	

	///1准备定时器
    ev_timer               ready_timer;          
    ev_tstamp              ready_timer_stamp;

	///2随机庄家定时器
	ev_timer               banker_timer;         
	ev_tstamp              banker_timer_stamp;   

	///3发4张牌定时器
	ev_timer               send_card_timer;      
	ev_tstamp              send_card_timer_stamp;   

	///4组牌定时器
	ev_timer               group_card_timer;      
	ev_tstamp              group_card_timer_stamp;   

	///5比牌定时器
    ev_timer               compare_timer;         
    ev_tstamp              compare_timer_stamp;

	///6预备定时器
	ev_timer               preready_timer;         
	ev_tstamp              preready_timer_stamp;	

    ev_timer                robot_timer;          ///---机器人定时器
    ev_tstamp               robot_timer_stamp;
	
public:
    SeeTable();
    virtual ~SeeTable();
	
	///初始化
	int init(int my_tid, 
		     int my_vid, 
			 int my_min_money,
			 int my_max_money, 
			 int my_base_money, 
			 int my_fee, 
			 int my_lose_exp, 
			 int my_win_exp);

	///广播
    int broadcast(Player *player, const std::string &packet);
	///单播
    int unicast(Player *player, const std::string &packet);
	///产生随机数
	int random(int start, int end);
	///桌子复位
	void reset();

	///vector转为json数组
	void vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key);
	///map转为json数组
	void map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key);
	///json数组转为vector
	void json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key);

private:
	///用户登录
	int handler_login(Player *player);
	///登录成功应答
	int handler_login_succ_uc(Player *player);
	///登录成功广播,“我”的信息广播出去
	int handler_login_succ_bc(Player *player);
	///下行房间数据给登录者
	int handler_table_info(Player *player);

	///用户坐下
	int sit_down(Player *player);
	///用户离开
	void stand_up(Player *player);

	///删除用户
	int del_player(Player *player);


	///1准备超时
	static void ready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	int handler_ready();

	///2随机选家超时
	static void banker_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	void banker_timeout(); 

	///3发牌超时
	static void send_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	void send_card_timeout(); 

	///4组牌超时
	static void group_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	void group_card_timeout(); 

	///4-1用户自已组牌
	int handler_group_card();

	///5比牌超时
	static void compare_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	void compare_timeout();

	///6预备超时
	static void preready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	int handler_preready();

	///游戏是否开始
	int test_game_start();   
	///游戏开始
	int game_start();

	///输更新
	void lose_update(Player *player);  
	///赢更新
	void win_update(Player *player);    

	///游戏是否结束
	int test_game_end();       
	int game_end();

	int handler_logout(Player *player);

	///发聊天
	int handler_chat(Player *player);
	///发表情
	int handler_face(Player *player);
	

	static void robot_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	void lose_exp_update(Player *player);
	void win_exp_update(Player *player);

	int handler_info(Player *player);

	int handler_interaction_emotion(Player *player); // cfc add 20140310

};

#endif
