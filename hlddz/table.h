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

class Player;
class Client;

typedef struct
{
	int enable; // 0 : enable 1 : disable
	int seatid; //
	int occupied; // seat
	int ready; // to play
	int betting; // betting
	
	int uid; // user id
	int see; // 0 : no see 1 : to see
	int role; // 0 : Player 1 : Dealer
	XtHoleCards hole_cards;
	int status; // 0 betting 1 fold 2 lose
	int bet;
	
	// cfc add 20140321
	int is_change;     // change card
	int is_forbidden;  // forbidden compare card 0:no >0:yes
	int is_double;     // compare multiple bet

    Player *player;
	
    void clear(void)
    {
    	// enable = 0; // 手否激活
        // seatid = 0; // 座位id
		occupied = 0; // 是否被坐下

		ready = 0; // 是否准备（如果没有准备为0，准备了为1）
		betting = 0; // 是否在玩（如果弃牌即为0，玩牌中为1）

		uid = 0; // 用户id
		see = 0; // 是否看牌
		role = 0; // 是否是庄家
		hole_cards.clear(); // 保存三张底牌
		status = 0; // 1、表示弃牌， 2、表示比牌失败
		bet = 0; // 个人投注

		is_change = 0;
		is_forbidden = 0;
		is_double = 1;

		player = NULL;
    }

    void reset(void)
    {
		ready = 0;
		betting = 0;

		// uid = 0;
		see = 0;
		role = 0;
		hole_cards.clear();
		status = 0;
		bet = 0;

		is_change = 0;
		is_forbidden = 0;
		is_double = 1;
    }
} Seat;


typedef enum
{
	READY = 0,
	BETTING,
    END_GAME,

	PRE_READY,
	WAIT,

} State;


enum UserState
{
   us_none         = 0,
   us_all_robot    = 1, 
   us_all_human    = 2, 
   us_mix          = 3,
};


class Table
{
public:
    int							tid;
	std::map<int, Player*>		players;
	State						state; // game state

private:
    int             			vid;
	int							zid;
	int							type; // is 老千
    int             			min_money;
	int							max_money;
	int							base_money;
	int							min_round;
	int							max_round;
	float                 fee;
	int                   lose_exp;
	int                   win_exp;

    int             			cur_players;
	int							ready_players;
	int 						betting_players;
	
	int							seat_max;
	Seat                        seats[5];

	XtTypeDeck					deck;
	
	//
	int							dealer;
	int							cur_round; // cur_round
	int							start_seat;
	int							cur_seat;
	int							cur_action;
	int							cur_bet;
	int							total_bet;
	int							win_bet;
	int                   cur_allin_bet;  // all in bet cfc add 20140124
	
    ev_timer                    preready_timer;
    ev_tstamp                   preready_timer_stamp;	

	// int							cur_player;
    ev_timer                    ready_timer;
    ev_tstamp                   ready_timer_stamp;

    ev_timer                    start_timer;
    ev_tstamp                   start_timer_stamp;
	
    ev_timer                    bet_timer;
    ev_tstamp                   bet_timer_stamp;

    ev_timer                    compare_timer;
    ev_tstamp                   compare_timer_stamp;

    ev_timer                    robot_timer;
    ev_tstamp                   robot_timer_stamp;

    UserState                   m_userState;

	
public:
    Table();
    virtual ~Table();
	int init(int my_tid, int my_vid, int my_zid, int my_type, int my_min_money,
				int my_max_money, int my_base_money, int my_min_round, int my_max_round, float my_fee,
				int my_lose_exp, int my_win_exp);
    int broadcast(Player *player, const std::string &packet);
    int unicast(Player *player, const std::string &packet);
	int random(int start, int end);
	void reset();
	void vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key);
	void map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key);
	void json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key);
	int sit_down(Player *player);
	void stand_up(Player *player);
	int del_player(Player *player);
	int handler_login(Player *player);
	int handler_login_succ_uc(Player *player);
	int handler_login_succ_bc(Player *player);
	int handler_table_info(Player *player);
	int handler_ready();
	int handler_preready();
	static void preready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	static void ready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	static void start_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	int test_game_start();
	int game_start();
	int count_next_bet();
	int start_next_bet();
	static void bet_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	static void compare_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	int handler_bet(Player *player);
	void bet_timeout();
	void compare_timeout();
	void lose_update(Player *player);
	void win_update(Player *player);
	int handler_call(Player *player);
	int handler_raise(Player *player);
	int handler_see(Player *player);
	int handler_compare(Player *player);
	int handler_fold(Player *player);
	int handler_allin(Player *player);  // cfc add 20140120
	int test_game_end();
	int game_end();
	int handler_logout(Player *player);
	int handler_chat(Player *player);
	int handler_face(Player *player);
	int next_seat(int pos);
	int next_betting_seat(int pos);
	int prev_betting_seat(int pos);
	int count_betting_seats();
	int get_last_betting();
	static void robot_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	void lose_exp_update(Player *player);
	void win_exp_update(Player *player);
	int handler_info(Player *player);
	int handler_allin_compare(Player *player);  // cfc add 20140123
	int handler_interaction_emotion(Player *player); // cfc add 20140310
	int handler_prop(Player *player);  // cfc add 20140321
	int handler_change_card(Player *player);
	int handler_forbidden_card(Player *player);
	int handler_double_card(Player *player, int multiple);
	void handler_prop_error(Player *player, int prop_id);
	void recover_forbidden_card();
    void saveChangeValue(double value);

private:
    void refreshUserState(void);
	UserState getUserState(void) const; 

    bool isRobotChange(void) const;
    void robotChange(void);
	int handler_robot_change_uc(Player *player);

    void resetRobotMoney(Player* player);
    void updateInfoBC(Player* player);

public:
    static int getRandBetween(int _min, int _max);

};

#endif
