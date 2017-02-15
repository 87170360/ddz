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

#include "bull_game_logic.h"
#include "global_define.h"

#include "jpacket.h"


///定义各阶段时间
const int READY_TIME     = 5;    // 准备时间
const int BANKER_TIME    = 2;    // 选庄时间
const int GROUP_TIME     = 15;   // 组牌时间
const int COMPARE_TIME   = 5;    // 比牌时间    


const int GROUP_TIME_EXTRA = 3;  // 用户没组牌时的附加时间


class Player;
class Client;

//坐位
typedef struct
{
	int seatid;                //坐位ID
	int uid;                   //用户id

	enChairStatus status;      //坐位状态 (空位,旁观,游戏中)	
	enRole role;               //角色 0:闲家, 1:庄家	
	Player *player;            //玩家 


	unsigned long long  goup_time; //组牌所花时间
	int group_time_diff;           //与庄家的组牌时间差值

	_uint8	m_cbUserHandCard[MAX_HAND_CARD_COUNT]; //最大手牌数

	CT_BULL_TYPE m_cardType;                          //牌型 
	_uint8	     m_cbChoiceCard[MAX_HAND_CARD_COUNT]; //选牌组合
	enEndKind    end_kind;                            //结束类型（输赢）
	bool         bPlayerChoiced;                            //是否选牌

	///清除坐位上的玩家
	void clearSeat(void)
	{
		uid = 0;              // 用户id
		player = NULL;
		status = STATUS_NULL; // 初始为空位 
		role   = ROLE_NULL;   // 角色（是否庄家）

		resetSeat();
	}

	///复位（玩家保留）
	void resetSeat(void)
	{
		goup_time = GROUP_TIME + GROUP_TIME_EXTRA; //组牌最大时间
		group_time_diff = 0;

		memset(m_cbUserHandCard, 0, sizeof(m_cbUserHandCard));
		memset(m_cbChoiceCard, 0, sizeof(m_cbChoiceCard));

		m_cardType = CT_NOBULL;
		end_kind   = KIND_NULL;
		bPlayerChoiced = false;   ///玩家有没选牌
	}

} Seat;


///游戏状态（游戏场景）
typedef enum
{
	READY = 0,     ///准备
	BANKER,        ///选庄（游戏开始）
	SEND_CARD,     ///发牌
	COMPARE,       ///比牌（游戏结束）
	WAIT,          ///等待
} EnGameState;


///牌桌（以牌桌为单位game）
class Table
{
public:
	int					   tid;          ///桌子ID
	int             	   vid;          ///场馆ID
	int             	   min_money;    ///最低携带
	int					   max_money;    ///最高携带
	int					   base_money;   ///底注（闲家结算=当前房间底分×赢方牌型倍数×时间加成   庄家结算=-∑闲家结算）
	int                    fee;          ///抽水金额（系统对每个玩家扣税）
	int                    lose_exp;     ///输的经验值
	int                    win_exp;      ///赢的经验值
	EnGameState			   game_state;   // game state(即游戏场景)

	int					   dealer;         //庄家坐位ID
	std::map<int, Player*> playersMap;     //玩家map(uid->Player)
	int             	   cur_players;    //当前玩家个数
	int					   ready_players;

	Seat                   seats[MAX_CHAIR_COUNT]; // 坐位数组(一张桌子5个位置)

	unsigned long long     start_group_time;  //组牌开始时刻	

	unsigned long long     start_ready_time;   //准备开始时刻	
	unsigned long long     start_banker_time;  //选庄开始时刻	
	unsigned long long     start_compare_time; //比牌开始时刻	



	CBullGameLogic		   *m_GameLogic;	  //游戏逻辑

	///1“准备”定时器
	ev_timer               ready_timer;          
	ev_tstamp              ready_timer_stamp;

	///2“随机庄家”定时器
	ev_timer               banker_timer;         
	ev_tstamp              banker_timer_stamp;   

	///3“发牌”定时器
	ev_timer               send_card_timer;      
	ev_tstamp              send_card_timer_stamp;   

	///4“比牌”定时器
	ev_timer               compare_timer;         
	ev_tstamp              compare_timer_stamp;

	///机器人定时器
	ev_timer                robot_timer;         
	ev_tstamp               robot_timer_stamp;

public:
	Table();
	virtual ~Table();

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

//private:
	///用户登录
	int handler_login(Player *player);
	///下行登录成功应答
	int send_login_succ_uc(Player *player);
	///下行登录成功广播（“我”的信息广播出去）
	int send_login_succ_bc(Player *player);
	///下行房间数据
	int send_table_info_uc(Player *player);

	///用户坐下
	int sit_down(Player *player);
	///用户离开
	void stand_up(Player *player);

	///删除用户
	int del_player(Player *player);


	// 1广播游戏准备
	int send_game_ready_bc();
	// 2-1随机选庄
	int game_start();
	///确定庄家
	int make_banker();
	// 2-2扣税（抽水）
	int pay_tax();
	// 2-3广播随机选庄
	int send_make_banker_bc();
	// 3发牌
	void send_card();
	// 4比牌
	int compare_card();
	// 5预备处理
	int pre_ready();

	/// 下行广播所有玩家最新信息
	int send_all_player_info_bc();

	// 1“准备”时间结束
	static void ready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	// 2“随机选庄”时间结束
	static void banker_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	// 3“发牌”时间结束
	static void send_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	// 4“比牌”时间结束
	static void compare_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	// 6“等待”时间结束
	static void wait_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);



	///4-1用户自已组牌
	int handler_group_card(Player *player);

	void compare_timeout();


	///游戏结束    
	int game_end();

	int handler_logout(Player *player);

	///发聊天
	int handler_chat(Player *player);
	///发表情
	int handler_face(Player *player);


	static void robot_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	void lose_exp_update(Player *player);
	void win_exp_update(Player *player);

	///把单个玩家信息广播出去
	int handler_info(Player *player);

	int handler_interaction_emotion(Player *player); // cfc add 20140310

	///设置玩家状态
	int setPlayStatus(int bankSeatid);

	//计算与庄家的组牌时间差值
	int CalGroupTimeDiff();

	//组牌
	void group_cards(Seat &seat);

	///下一局庄家
	int next_banker();

	//是否所有玩家组牌都组牌了
	bool isAllPlayerChoiced();
};

#endif
