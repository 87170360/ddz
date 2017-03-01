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

int Table::init(int my_tid, int my_vid, int my_zid, int my_type, int my_min_money,
				int my_max_money, int my_base_money, int my_min_round, int my_max_round, float my_fee,
				int my_lose_exp, int my_win_exp)
{
	// xt_log.debug("begin to init table [%d]\n", table_id);
	tid = my_tid;
    return 0;
}

int Table::broadcast(Player *p, const std::string &packet)
{
    Player *player;
    std::map<int, Player*>::iterator it;
    for (it = players.begin(); it != players.end(); it++)
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
