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
const unsigned int BOTTON_CARD_NUM     = 17;

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

    int handler_login(Player* player);

    bool sitdown(Player* player);
    //分牌
    bool allocateCard(void);

    //msg
    void loginUC(Player* player);
    void loginBC(Player* player);
    //第一次发牌
    void sendCard1(void);

    void gameStart(void);

private:
    void reset(void);

public:
    int							m_tid;
    int             			m_vid;
    int                         m_state;
	std::map<int, Player*>		m_players;
    int                         m_seats[SEAT_NUM];

private:
    XtShuffleDeck               m_deck;
    std::vector<XtCard>         m_bottomCard;

    ev_timer                    m_timerPrepare;                 //准备
    ev_timer                    m_timerCall;                    //抢地主
    ev_timer                    m_timerDouble;                  //加倍
    ev_timer                    m_timerCard;                    //出牌
    ev_timer                    m_timerEnd;                     //结算
};

#endif
