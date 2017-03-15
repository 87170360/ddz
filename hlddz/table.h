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

#include "XtShuffleDeck.h"
#include "XtHoleCards.h"
#include "jpacket.h"

const unsigned int SEAT_NUM          = 3;
const unsigned int HAND_CARD_NUM     = 17;
const unsigned int BOTTON_CARD_NUM     = 3;

class Player;
class Client;

class Table
{
public:
    Table();
    virtual ~Table();
	int init(int tid);

    int broadcast(Player *player, const std::string &packet);
    int unicast(Player *player, const std::string &packet);
	int random(int start, int end);
	void vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key);
	void map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key);
	void json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key);

    static void callCB(struct ev_loop *loop, struct ev_timer *w, int revents);
    static void doubleCB(struct ev_loop *loop, struct ev_timer *w, int revents);
    static void cardCB(struct ev_loop *loop, struct ev_timer *w, int revents);
    static void endCB(struct ev_loop *loop, struct ev_timer *w, int revents);
    void call(void);
    void doubl(void);
    void card(void);
    void end(void);

    //receive msg
    int login(Player* player);
    void msgCall(Player* player);
    void msgDouble(Player* player);

    //加倍处理
    void doubleProc(void);
    //坐下
    bool sitdown(Player* player);
    //分牌
    bool allocateCard(void);

    // send msg
    void loginUC(Player* player);
    void loginBC(Player* player);
    //第一次发牌
    void sendCard1(void);
    //叫分
    void sendCall(void);
    //继续叫分
    void sendCallAgain(void); 
    //叫分结果
    void sendCallResult(void);
    //继续加倍
    void sendDoubleAgain(void);

    void gameStart(void);
    //获取下一个操作用户
    bool getNext(void);
    //获取用户id
    int getSeatUid(unsigned int seatid);
    //设置座位状态
    void setAllSeatOp(int state);
    //评选地主
    bool selecLord(void);

private:
    void reset(void);

public:
    int							m_tid;
    int             			m_vid;
    int                         m_state;
	std::map<int, Player*>		m_players;

private:
    int                         m_seats[SEAT_NUM];              //各座位玩家id
    int                         m_opState[SEAT_NUM];            //各座位操作状态            
    int                         m_score[SEAT_NUM];              //各座位叫分
    int                         m_count[SEAT_NUM];             //各座位加倍 0:不加倍 1:加倍
    unsigned int                m_curSeat;                      //当前操作者座位
    unsigned int                m_preSeat;                      //上个操作者座位
    unsigned int                m_lordSeat;                     //地主座位
    int                         m_topCall;                      //最高叫分

    XtShuffleDeck               m_deck;
    std::vector<XtCard>         m_bottomCard;                   //底牌

    ev_timer                    m_timerCall;                    //叫分
    ev_timer                    m_timerDouble;                  //加倍
    ev_timer                    m_timerCard;                    //出牌
    ev_timer                    m_timerEnd;                     //结算
};

#endif
