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

const int SEATNUM = 3;

class Player;
class Client;

class Table
{
public:
    Table();
    virtual ~Table();
	int init(int tableid);
    int broadcast(Player *player, const std::string &packet);
    int unicast(Player *player, const std::string &packet);
	int random(int start, int end);
	void vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key);
	void map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key);
	void json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key);

    int handler_login(Player* player);

    bool sitdown(Player* player);
    //发牌
    void sendCard(void);

    //msg
    void loginUC(Player* player);
    void loginBC(Player* player);

public:
    int							m_tid;
    int             			m_vid;
    int                         m_state;
	std::map<int, Player*>		m_players;
    int                         m_seats[SEATNUM];
};

#endif
