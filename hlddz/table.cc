#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <algorithm>
#include <assert.h>

#include "hlddz.h"
#include "game.h"
#include "log.h"
#include "table.h"
#include "client.h"
#include "player.h"
#include "proto.h"
#include "XtCard.h"

extern HLDDZ hlddz;
extern Log xt_log;

Table::Table()
{
}

Table::~Table()
{
}

int Table::init(int tid)
{
	// xt_log.debug("begin to init table [%d]\n", table_id);
    m_tid = tid;

    reset();

    return 0;
}
    
void Table::reset(void)
{
    for(int i = 0; i < SEAT_NUM; ++i)
    {
        m_seats[i] = 0; 
    }
    m_deck.fill();
    m_deck.shuffle(m_tid);
}

int Table::broadcast(Player *p, const std::string &packet)
{
    Player *player;
    std::map<int, Player*>::iterator it;
    for (it = m_players.begin(); it != m_players.end(); it++)
    {
        player = it->second;
        if (player == p || player->client == NULL)
        {
            continue;
        }
        player->client->send(packet);
    }

    return 0;
}

int Table::unicast(Player *p, const std::string &packet)
{
    if (p->client)
    {
        return p->client->send(packet);
    }
    return -1;
}

int Table::random(int start, int end)
{
	return start + rand() % (end - start + 1);
}

void Table::vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
	for (unsigned int i = 0; i < cards.size(); i++) {
		packet.val[key].append(cards[i].m_value);
	}

	if (cards.size() == 0) {
		packet.val[key].append(0);
	}
}

void Table::map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key)
{
	std::map<int, XtCard>::iterator it;
	for (it = cards.begin(); it != cards.end(); it++)
	{
		XtCard &card = it->second;
		packet.val[key].append(card.m_value);
	}
}

void Table::json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
	Json::Value &val = packet.tojson();
	
	for (unsigned int i = 0; i < val[key].size(); i++)
	{
		XtCard card(val[key][i].asInt());
		
		cards.push_back(card);
	}
}

int Table::handler_login(Player *player)
{
    if(m_players.find(player->uid) != m_players.end())
    {
        xt_log.error("%s:%d, player was existed! uid:%d", __FILE__, __LINE__, player->uid); 
        return 0;
    }
    if(!sitdown(player))
    {
        return 0;
    }

    //登录回复
    loginUC(player);

    //广播玩家信息
    loginBC(player);

    //人满开始, 定时器

    return 0;
}
    
bool Table::sitdown(Player* player)
{
    int seatid = -1;
    for(int i = 0; i < SEAT_NUM; ++i)
    {
        if(m_seats[i] == 0) 
        {
            seatid = i; 
            break;
        }
    }
    if(seatid < 0)
    {
        xt_log.error("%s:%d, no empty seat.", __FILE__, __LINE__); 
        return false; 
    }

    player->m_seatid = seatid;
    m_seats[seatid] = player->uid;
    m_players[player->uid] = player;
    return true;
}
    
void Table::loginUC(Player* player)
{
    Jpacket packet;
    packet.val["cmd"]   = SERVER_RESPOND;
    packet.val["code"]  = CODE_SUCCESS;
    packet.val["msgid"] = CLIENT_LOGIN;
    packet.val["tid"]   = m_tid;
    packet.end();
    unicast(player, packet.tostring());
}

void Table::loginBC(Player* player)
{
    Jpacket packet;
    packet.val["cmd"]   = SERVER_LOGIN;
    packet.val["uid"]   = player->uid;
    packet.end();
    broadcast(player, packet.tostring());
}
    
void Table::allocateCard(void)
{
    
}
