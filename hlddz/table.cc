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

const ev_tstamp prepareTime(10); 
const ev_tstamp callTime(10); 
const ev_tstamp doubleTime(10); 
const ev_tstamp cardTime(10); 
const ev_tstamp endTime(10); 

Table::Table()
{
    m_timerPrepare.data = this;
    ev_timer_init(&m_timerPrepare, Table::prepareCB, prepareTime, prepareTime);

    m_timerCall.data = this;
    ev_timer_init(&m_timerCall, Table::callCB, callTime, callTime);

    m_timerDouble.data = this;
    ev_timer_init(&m_timerDouble, Table::doubleCB, doubleTime, doubleTime);

    m_timerCard.data = this;
    ev_timer_init(&m_timerCard, Table::cardCB, cardTime, cardTime);

    m_timerEnd.data = this;
    ev_timer_init(&m_timerEnd, Table::endCB, endTime, endTime);
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
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        m_seats[i] = 0; 
    }
    m_bottomCard.clear();
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
    if (cards.empty()) 
    {
        packet.val[key].append(0);
        return;
    }

    for (unsigned int i = 0; i < cards.size(); i++) 
    {
        packet.val[key].append(cards[i].m_value);
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

void Table::prepareCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerPrepare);
    table->prepare();
}

void Table::callCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerCall);
    table->call();
}

void Table::doubleCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerDouble);
    table->doubl();
}

void Table::cardCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerCard);
    table->card();
}

void Table::endCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerEnd);
    table->end();
}

int Table::login(Player *player)
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
    if(m_players.size() == 3)
    {
        gameStart();
    }

    return 0;
}

void Table::prepare(void)
{
}

void Table::call(void)
{
}

void Table::doubl(void)
{
}

void Table::card(void)
{
}

void Table::end(void)
{
}

bool Table::sitdown(Player* player)
{
    int seatid = -1;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
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

bool Table::allocateCard(void)
{
    //底牌    
    if(!m_deck.getHoleCards(m_bottomCard, BOTTON_CARD_NUM))
    {
        xt_log.error("%s:%d, get bottom card error,  tid:%d", __FILE__, __LINE__, m_tid); 
        return false;
    }
    //手牌
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        if(!m_deck.getHoleCards(pl->m_holecard.m_cards, HAND_CARD_NUM))
        {
            xt_log.error("%s:%d, get hand card error,  tid:%d", __FILE__, __LINE__, m_tid); 
            return false;
        }
    }

    return true;
}
    
void Table::sendCard1(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]   = SERVER_CARD_1;
        vector_to_json_array(pl->m_holecard.m_cards, packet, "card");
        packet.end();
        unicast(pl, packet.tostring());
    }
}
    
void Table::gameStart(void)
{
    allocateCard();
    sendCard1();
}
