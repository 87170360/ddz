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

const int CALLTIME          = 3;
const int DOUBLETIME        = 3;
const int CARDTIME          = 10;
const int ENDTIME           = 10;

Table::Table()
{
    m_timerCall.data = this;
    ev_timer_init(&m_timerCall, Table::callCB, ev_tstamp(CALLTIME), ev_tstamp(CALLTIME));

    m_timerDouble.data = this;
    ev_timer_init(&m_timerDouble, Table::doubleCB, ev_tstamp(DOUBLETIME), ev_tstamp(DOUBLETIME));

    m_timerCard.data = this;
    ev_timer_init(&m_timerCard, Table::cardCB, ev_tstamp(CARDTIME), ev_tstamp(CARDTIME));

    m_timerEnd.data = this;
    ev_timer_init(&m_timerEnd, Table::endCB, ev_tstamp(ENDTIME), ev_tstamp(ENDTIME));
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
        m_score[i] = 0;
        m_count[i] = 0;
    }
    setAllSeatOp(CALL_WAIT);
    m_bottomCard.clear();
    m_deck.fill();
    m_deck.shuffle(m_tid);
    m_state = STATE_WAIT;
    m_curSeat = 0;
    m_preSeat = 0;
    m_lordSeat = 0;
    m_topCall = 0;
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
    xt_log.debug("player login uid:%d\n", player->uid);
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
    
void Table::msgCall(Player* player)
{
    //xt_log.debug("msg Call uid:%d\n", player->uid);
    if(m_state != STATE_CALL)
    {
        xt_log.error("%s:%d, no call state.\n", __FILE__, __LINE__); 
        return;
    }

    //座位玩家是否匹配
    if(player->m_seatid >= SEAT_NUM || player->m_seatid < 0 || m_seats[player->m_seatid] != player->uid)
    {
        xt_log.error("%s:%d, seat info error. player seatid:%d, uid:%d, seatuid:%d\n", __FILE__, __LINE__, player->m_seatid, player->uid, m_seats[player->m_seatid]); 
        return;
    }

    if(m_opState[player->m_seatid] != CALL_NOTIFY)
    {//座位通知过
        xt_log.error("%s:%d, player callstate error. player seatid:%d, uid:%d, callstate:%d\n", __FILE__, __LINE__, player->m_seatid, player->uid, m_opState[player->m_seatid]); 
        return; 
    }

    //是否当前操作者
    if(m_curSeat != player->m_seatid)
    {
        xt_log.error("%s:%d, operator error. m_curSeat:%d, playerSeat:%d\n", __FILE__, __LINE__, m_curSeat, player->m_seatid); 
        return; 
    }

    Json::Value &msg = player->client->packet.tojson();
    m_score[player->m_seatid] = msg["score"].asInt();
    xt_log.debug("call score, uid:%d, seatid:%d, score :%d\n", player->uid, player->m_seatid, m_score[player->m_seatid]);
    
    //叫3分直接地主
    //广播当前叫分和下一个叫分
    if(getNext())
    {
        sendCallAgain(); 
    }
    else if(selecLord())
    {//选地主，进入加倍环节
        doubleProc();
        sendCallResult(); 
    }
    else
    {//重新发牌
    
    }
}
    
void Table::msgDouble(Player* player)
{
    xt_log.debug("msg double, uid:%d, seatid:%d\n", player->uid, player->m_seatid);
    if(getNext())
    {
        getNext();
        sendDoubleAgain();
    }
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
    
void Table::doubleProc(void)
{
    m_state = STATE_DOUBLE; 
    setAllSeatOp(DOUBLE_WAIT);
    m_opState[m_lordSeat] = DOUBLE_NONE;

    //选择一个农民加倍
    m_curSeat = (m_lordSeat + 1) % SEAT_NUM;
    m_preSeat = m_curSeat;
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
    player->m_tid = m_tid;
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
        packet.val["cmd"]           = SERVER_CARD_1;
        vector_to_json_array(pl->m_holecard.m_cards, packet, "card");
        packet.val["time"]          = CALLTIME;
        packet.val["cur_id"]        = getSeatUid(m_curSeat);
        packet.end();
        unicast(pl, packet.tostring());
    }
}
    
void Table::sendCallAgain(void) 
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_AGAIN_CALL;
        packet.val["time"]          = CALLTIME;
        packet.val["cur_id"]        = getSeatUid(m_curSeat);
        packet.val["pre_id"]        = getSeatUid(m_preSeat);
        packet.val["score"]         = m_score[m_preSeat];
        packet.end();
        unicast(pl, packet.tostring());
    }
}
    
void Table::sendCallResult(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_RESULT_CALL;
        packet.val["time"]          = DOUBLETIME;
        packet.val["score"]         = m_topCall;
        packet.val["lord"]          = getSeatUid(m_lordSeat);
        packet.val["cur_id"]        = getSeatUid(m_curSeat);
        packet.end();
        unicast(pl, packet.tostring());
    }
}
    
void Table::sendDoubleAgain(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_AGAIN_DOUBLE;
        packet.val["time"]          = CALLTIME;
        packet.val["cur_id"]        = getSeatUid(m_curSeat);
        packet.val["pre_id"]        = getSeatUid(m_preSeat);
        packet.val["count"]         = m_count[m_preSeat];
        packet.end();
        unicast(pl, packet.tostring());
    }
}
    
void Table::gameStart(void)
{
    m_curSeat = rand() % SEAT_NUM;
    m_opState[m_curSeat] = CALL_NOTIFY;
    m_state = STATE_CALL;

    allocateCard();
    sendCard1();

    xt_log.debug("game start, cur_id:%d, seateid:%d\n", getSeatUid(m_curSeat), m_curSeat);

    ev_timer_again(hlddz.loop, &m_timerCall);
}
    
bool Table::getNext(void)
{
    int nextSeat = (m_curSeat + 1) % SEAT_NUM;
    int targetState = 0;
    switch(m_state)
    {
        case STATE_CALL:        targetState = CALL_WAIT;        break;
        case STATE_DOUBLE:      targetState = DOUBLE_WAIT;      break;
        case STATE_CARD:        targetState = CARD_WAIT;        break;
    }

    if(m_opState[nextSeat] == targetState) 
    {
        m_preSeat = m_curSeat;
        m_curSeat = nextSeat;
        m_opState[m_curSeat] = CALL_NOTIFY;
        xt_log.debug("get next success, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
        return true; 
    }
    return false;
}
    
int Table::getSeatUid(unsigned int seatid)
{
   if(seatid < 0 || seatid >= SEAT_NUM) 
   {
        xt_log.error("%s:%d, getSeatUid error! seatid:%d", __FILE__, __LINE__, seatid); 
        return 0;
   }
   return m_seats[seatid];
}
    
void Table::setAllSeatOp(int state)
{
   for(unsigned int i = 0; i < SEAT_NUM; ++i) 
   {
        m_opState[i] = state; 
   }
}
    
bool Table::selecLord(void)
{
   unsigned int seatid = 0;
   int score = 0;
   for(unsigned int i = 0; i < SEAT_NUM; ++i) 
   {
       if(m_score[i] > score) 
       {
            score = m_score[i]; 
            seatid = i;
       }
   }
   if(score == 0)
   {
        return false;
   }

   m_topCall = score;
   m_lordSeat = seatid;

   xt_log.debug("selectLord success, score:%d, seatid:%d, uid:%d\n", m_topCall, m_lordSeat, getSeatUid(m_lordSeat));
   return true;
}
