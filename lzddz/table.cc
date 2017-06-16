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

#include "lzddz.h"
#include "game.h"
#include "log.h"
#include "table.h"
#include "client.h"
#include "player.h"
#include "proto.h"
#include "card.h"

extern LZDDZ lzddz;
extern Log xt_log;

Table::Table() : m_count(0)
{
    m_timerLord.data = this;
    ev_timer_init(&m_timerLord, Table::lordCB, ev_tstamp(lzddz.game->CALLTIME), ev_tstamp(lzddz.game->CALLTIME));

    m_timerGrab.data = this;
    ev_timer_init(&m_timerGrab, Table::grabCB, ev_tstamp(lzddz.game->GRABTIME), ev_tstamp(lzddz.game->GRABTIME));

    m_timerDouble.data = this;
    ev_timer_init(&m_timerDouble, Table::doubleCB, ev_tstamp(lzddz.game->DOUBLETIME), ev_tstamp(lzddz.game->DOUBLETIME));

    m_timerOut.data = this;
    ev_timer_init(&m_timerOut, Table::OutCB, ev_tstamp(lzddz.game->OUTTIME), ev_tstamp(lzddz.game->OUTTIME));

    m_timerKick.data = this;
    ev_timer_init(&m_timerKick, Table::kickCB, ev_tstamp(lzddz.game->KICKTIME), ev_tstamp(lzddz.game->KICKTIME));

    m_timerUpdate.data = this;
    ev_timer_init(&m_timerUpdate, Table::updateCB, ev_tstamp(lzddz.game->UPDATETIME), ev_tstamp(lzddz.game->UPDATETIME));

    m_timerEntrustOut.data = this;
    ev_timer_init(&m_timerEntrustOut, Table::entrustOutCB, ev_tstamp(ENTRUST_OUT_TIME), ev_tstamp(ENTRUST_OUT_TIME));

    m_timerWaitCall.data = this;
    ev_timer_init(&m_timerWaitCall, Table::waitCallCB, ev_tstamp(WAIT_CALL_TIME), ev_tstamp(WAIT_CALL_TIME));
}

Table::~Table()
{
    xt_log.debug("~Table \n");
    ev_timer_stop(lzddz.loop, &m_timerDouble);
    ev_timer_stop(lzddz.loop, &m_timerOut);
    ev_timer_stop(lzddz.loop, &m_timerKick);
    ev_timer_stop(lzddz.loop, &m_timerUpdate);
    ev_timer_stop(lzddz.loop, &m_timerEntrustOut);
    ev_timer_stop(lzddz.loop, &m_timerLord);
    ev_timer_stop(lzddz.loop, &m_timerGrab);
}

int Table::init(int tid)
{
    // xt_log.debug("begin to init table [%d]\n", table_id);
    m_tid = tid;
    reset();
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        setSeat(0, i);
    }
    return 0;
}

void Table::reset(void)
{
    //xt_log.debug("reset.\n");
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        m_seats[i] = 0;
        m_famerDouble[i] = false;
        m_seatCard[i].reset();
        m_bomb[i] = 0;
        m_outNum[i] = 0;
        m_money[i] = 0;
        m_entrust[i] = false;
        m_timeout[i] = false;
        m_opState[i] = OP_PREPARE_WAIT; 
    }
    m_bottomCard.clear();
    m_lastCard.clear();
    m_players.clear();
    m_deck.shuffle(m_tid);
    m_curSeat = 0;
    m_preSeat = 0;
    m_lordSeat = 0;
    m_outSeat = 0;
    m_topCall = 0;
    m_win = 0;
    m_time = 0;
    m_state = STATE_PREPARE; 
    m_grabDoulbe = 0;
    m_firstSeat = 0;
    m_act.clear();

    ev_timer_stop(lzddz.loop, &m_timerDouble);
    ev_timer_stop(lzddz.loop, &m_timerOut);
    ev_timer_stop(lzddz.loop, &m_timerKick);
    ev_timer_stop(lzddz.loop, &m_timerUpdate);
    ev_timer_stop(lzddz.loop, &m_timerEntrustOut);
    ev_timer_stop(lzddz.loop, &m_timerLord);
    ev_timer_stop(lzddz.loop, &m_timerGrab);
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
    if (p && p->client)
    {
        return p->client->send(packet);
    }
    return -1;
}

int Table::random(int start, int end)
{
    return start + rand() % (end - start + 1);
}

void Table::vector_to_json_array(std::vector<Card> &cards, Jpacket &packet, string key)
{
    if (cards.empty()) 
    {
        return;
    }

    for (unsigned int i = 0; i < cards.size(); i++) 
    {
        packet.val[key].append(cards[i].m_oldvalue);
    }
}

void Table::json_array_to_vector(std::vector<Card> &cards, Jpacket &packet, string key)
{
    Json::Value &val = packet.tojson();
    if(!val.isMember(key))
    {
        return;
    }

    for (unsigned int i = 0; i < val[key].size(); i++)
    {
        Card card(val[key][i].asInt());
        cards.push_back(card);
    }
}
        
void Table::jsonArrayToVector(std::vector<int> &change, Jpacket &packet, string key)
{
    Json::Value &val = packet.tojson();
    if(!val.isMember(key))
    {
        return;
    }

    for (unsigned int i = 0; i < val[key].size(); i++)
    {
        change.push_back(val[key][i].asInt());
        //xt_log.debug("lz card change value:%d\n", change.back());
    }
}
        
void Table::vectorToJsonArray(const std::vector<Card> &card, Jpacket &packet, string key)
{
    if(card.empty()) 
    {
        return;
    }

    for(vector<Card>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        if(it->m_value != it->m_oldvalue)
        {
            packet.val[key].append(it->m_value);
            //xt_log.debug("sendOutAgain, lzvalue:%d\n", it->m_value);
        }
    }
}

void Table::doubleCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(lzddz.loop, &table->m_timerDouble);
    //xt_log.debug("stop m_timerDouble for timerup.\n");
    table->onDouble();
}

void Table::onDouble(void)
{
    //xt_log.debug("onDouble\n");
    //农民加倍
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(i != m_lordSeat && m_opState[i] == OP_DOUBLE_NOTIFY) 
        {
            m_famerDouble[i] = false;
            m_opState[i] = OP_DOUBLE_RECEIVE;
            sendDouble(m_seats[i], m_famerDouble[i]);
        }
    }
    logicDouble(false);
}

void Table::OutCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(lzddz.loop, &table->m_timerOut);
    //xt_log.debug("stop m_timerOut for timerup.\n");
    table->onOut();
}

void Table::onOut(void)
{
    //xt_log.debug("onOut. m_curSeat:%d\n", m_curSeat);
    Player* player = getSeatPlayer(m_curSeat);
    bool keep = false;
    vector<Card> curCard;
    vector<Card> &myCard = m_seatCard[m_curSeat].m_cards;

    Card::sortByDescending(myCard);
    Card::sortByDescending(m_lastCard);

    if(m_entrust[m_curSeat])
    {
        xt_log.error("%s:%d, entrust player shall not timerout. seatid:%d\n", __FILE__, __LINE__, m_curSeat); 
    }

    //前一次已经超时
    if(m_timeout[m_curSeat])
    {
        //首轮出牌
        //没人跟自己的牌
        if(m_lastCard.empty() || m_curSeat == m_outSeat)
        {
            m_deck.getFirst(myCard, curCard);
        }
        //跟别人的牌
        else
        {
            m_deck.getFollow(myCard, m_lastCard, curCard);
        }
        m_entrust[m_curSeat] = true;
    }
    //第一次超时
    else
    {
        //记录超时
        m_timeout[m_curSeat] = true;
        //首轮出牌
        //没人跟自己的牌
        if(m_lastCard.empty() || m_curSeat == m_outSeat)
        {
            m_deck.getFirst(myCard, curCard);
            m_entrust[m_curSeat] = true;
        }
        //跟别人的牌
        else
        {
            curCard.clear();
        }
    }

    keep = curCard.empty() ? true : false; 

    //判断是否结束和通知下一个出牌人，本轮出牌
    logicOut(player, curCard, keep);
}

void Table::kickCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(lzddz.loop, &table->m_timerKick);
    table->onKick();
}

void Table::onKick(void)
{
    Player* pl = NULL;
    for(vector<Player*>::iterator it = m_delPlayer.begin(); it != m_delPlayer.end(); ++it)
    {
        pl = *it;
        if(pl->isRobot())
        {
            continue;
        }
        xt_log.debug("%s:%d, del player active! m_uid:%d \n",__FILE__, __LINE__, pl->m_uid); 
        //删除后，最后流程走回这里的logout
        lzddz.game->del_player(*it);
    }
    m_delPlayer.clear();
}

void Table::updateCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    table->onUpdate();
}

void Table::onUpdate(void)
{
    //xt_log.debug("onUpdate.\n");
    if(--m_time >= 0)
    {
        sendTime();
    }
}

void Table::entrustOutCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(lzddz.loop, &table->m_timerEntrustOut);
    table->onEntrustOut();
}

void Table::onEntrustOut(void)
{
    entrustOut();
}

void Table::lordCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    //xt_log.debug("lordCB.\n");
    Table *table = (Table*) w->data;
    ev_timer_stop(lzddz.loop, &table->m_timerLord);
    table->onLord();
}

void Table::onLord(void)
{
    //超时不叫
    bool isCall = false;
    //记录状态
    m_opState[m_curSeat] = isCall ? OP_CALL_RECEIVE_Y : OP_CALL_RECEIVE_N;
    //记录行为
    m_act[isCall ? SS_CALL : SS_NOCA];
    //xt_log.debug("onLord.\n");
    logicLord();
}

void Table::grabCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(lzddz.loop, &table->m_timerGrab);
    //xt_log.debug("%s:%d, stop m_timerGrab for timeup.\n", __FILE__, __LINE__);
    table->onGrab();
}

void Table::onGrab(void)
{
    bool isGrab = false;
    m_opState[m_curSeat] = isGrab ? OP_GRAB_RECEIVE_Y : OP_GRAB_RECEIVE_N;
    m_act.push_back(isGrab ? SS_GRAB : SS_NOGR);
    xt_log.debug("onGrab. m_curSeat:%d\n", m_curSeat);
    logicLord();
}

void Table::waitCallCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(lzddz.loop, w);
    //xt_log.debug("%s:%d, stop m_timerGrab for timeup.\n", __FILE__, __LINE__);
    table->onWaitCall();
}

void Table::onWaitCall(void)
{
    //xt_log.debug("onWaitCall. \n");
    sendCall();
}

int Table::login(Player *player)
{
    //xt_log.debug("player login m_uid:%d, tid:%d\n", player->m_uid, m_tid);
    if(m_players.find(player->m_uid) != m_players.end())
    {
        xt_log.error("%s:%d, login fail! player was existed! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        return 0;
    }

    //给机器人加钱
    addRobotMoney(player);

    //检查入场费
    if(player->m_money < lzddz.game->ROOMTAX)
    {
        xt_log.error("%s:%d, player was no enouth money! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        loginUC(player, CODE_MONEY);
        return 0; 
    }

    if(!sitdown(player))
    {
        return 0;
    }

    //登录回复
    loginUC(player, CODE_SUCCESS);

    //广播玩家信息
    loginBC(player);

    return 0;
}

void Table::reLogin(Player* player) 
{
    xt_log.debug("player relogin m_uid:%d\n", player->m_uid);
    if(player->m_table_count != m_count)
    {
        xt_log.error("%s:%d, not in same table game m_uid:%d, table_count:%d, count:%d\n", __FILE__, __LINE__, player->m_uid, player->m_table_count, m_count); 
        return;
    }

    if(m_players.find(player->m_uid) == m_players.end())
    {
        xt_log.error("%s:%d, player was not existed! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        sendError(player, CLIENT_LOGIN, CODE_RELOGIN);
        return;
    }

    if(player->m_seatid < 0 || player->m_seatid > SEAT_NUM)
    {
        xt_log.error("%s:%d, player seat error! uid:%d, seatid:%d\n", __FILE__, __LINE__, player->m_uid, player->m_seatid); 
        sendError(player, CLIENT_LOGIN, CODE_SEAT);
        return;
    }

    //给机器人加钱
    addRobotMoney(player);

    //检查入场费
    if(player->m_money < lzddz.game->ROOMTAX)
    {
        xt_log.error("%s:%d, player was no enouth money! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        sendError(player, CLIENT_LOGIN, CODE_MONEY);
        return; 
    }

    //断线时候托管了，重连取消托管
    m_entrust[player->m_seatid] = false;

    sendRelogin(player);
}

void Table::msgPrepare(Player* player)
{
    //xt_log.debug("msg prepare m_uid:%d, seatid:%d, size:%d\n", player->m_uid, player->m_seatid, m_players.size());
    //检查入场费
    if(player->m_money < lzddz.game->ROOMTAX)
    {
        xt_log.error("%s:%d, prepare fail!, player was no enouth money! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        sendError(player, CLIENT_PREPARE, CODE_MONEY);
        return; 
    }

    //重复准备
    if(m_opState[player->m_seatid] == OP_PREPARE_REDAY)
    {
        xt_log.error("%s:%d, prepare fail!, player repeat prepare! m_uid:%d, seatid:%d\n", __FILE__, __LINE__, player->m_uid, player->m_seatid); 
        sendError(player, CLIENT_PREPARE, CODE_PREPARE);
        return; 
    }

    //检查状态
    if(m_state != STATE_PREPARE)
    {
        xt_log.error("%s:%d, prepare fail!, game state not state_prepare, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_PREPARE, CODE_STATE);
        return; 
    }
    
    sendPrepare(player);

    m_opState[player->m_seatid] = OP_PREPARE_REDAY; 
    if(!allSeatFit(OP_PREPARE_REDAY))
    {
        //xt_log.debug("not all is prepare.\n");
        return;
    }
    else if(m_players.size() != SEAT_NUM)
    {
        xt_log.debug("not enouth player, size:%d\n", m_players.size());
        return;
    }
    else
    {
        gameStart(); 
    }
}

void Table::msgCall(Player* player)
{
    //xt_log.debug("msg Call m_uid:%d\n", player->m_uid);
    //检查状态
    if(m_state != STATE_CALL)
    {
        xt_log.error("%s:%d, call fail!, game state not call_state, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_CALL, CODE_STATE);
        return;
    }

    //座位玩家是否匹配
    if(player->m_seatid >= SEAT_NUM || player->m_seatid < 0 || getSeat(player->m_seatid) != player->m_uid)
    {
        xt_log.error("%s:%d, call fail!, seat info error. player seatid:%d, m_uid:%d, seatuid:%d\n", __FILE__, __LINE__, player->m_seatid, player->m_uid, getSeat(player->m_seatid)); 
        sendError(player, CLIENT_CALL, CODE_SEAT);
        return;
    }

    if(m_opState[player->m_seatid] != OP_CALL_NOTIFY)
    {        
        xt_log.error("%s:%d, call fail!, player callstate error. player seatid:%d, m_uid:%d, callstate:%s\n", __FILE__, __LINE__, player->m_seatid, player->m_uid, DESC_OP[m_opState[player->m_seatid]]); 
        sendError(player, CLIENT_CALL, CODE_NOTIFY);
        return; 
    }

    //是否当前操作者
    if(m_curSeat != player->m_seatid)
    {
        xt_log.error("%s:%d, call fail!, operator error. m_curSeat:%d, playerSeat:%d\n", __FILE__, __LINE__, m_curSeat, player->m_seatid); 
        sendError(player, CLIENT_CALL, CODE_CURRENT);
        return; 
    }

    //托管中
    if(m_entrust[player->m_seatid])
    {
        xt_log.error("%s:%d, call fail!, entrusting. playerSeat:%d\n", __FILE__, __LINE__, player->m_seatid); 
        sendError(player, CLIENT_CALL, CODE_ENTRUST);
        return; 
    }

    //有效叫地主
    Json::Value &msg = player->client->packet.tojson();
    bool act = msg["act"].asBool();

    //停止定时器
    //xt_log.debug("stop m_timerLord for msg.\n");
    ev_timer_stop(lzddz.loop, &m_timerLord);

    //记录状态
    m_opState[m_curSeat] = act ? OP_CALL_RECEIVE_Y : OP_CALL_RECEIVE_N;
    //记录行为
    m_act.push_back(act ? SS_CALL : SS_NOCA);
    //xt_log.debug("call msg, m_uid:%d, seatid:%d, act:%s \n", player->m_uid, player->m_seatid, act ? "true" : "false");
    logicLord();
}

void Table::msgGrab(Player* player)
{
    //检查状态
    if(m_state != STATE_GRAB)
    {
        xt_log.error("%s:%d, grab fail!, game state not call_state, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_GRAB, CODE_STATE);
        return;
    }

    //是否当前操作者
    if(m_curSeat != player->m_seatid)
    {
        xt_log.error("%s:%d, grab fail!, operator error. m_curSeat:%d, playerSeat:%d\n", __FILE__, __LINE__, m_curSeat, player->m_seatid); 
        sendError(player, CLIENT_GRAB, CODE_CURRENT);
        return; 
    }

    if(m_opState[player->m_seatid] != OP_CALL_NOTIFY)
    {        
        xt_log.error("%s:%d, grab fail!, player callstate error. player seatid:%d, m_uid:%d, callstate:%s\n", __FILE__, __LINE__, player->m_seatid, player->m_uid, DESC_OP[m_opState[player->m_seatid]]); 
        sendError(player, CLIENT_GRAB, CODE_NOTIFY);
        return; 
    }

    Json::Value &msg = player->client->packet.tojson();
    bool act = msg["act"].asBool();
    
    //停止定时器
    //xt_log.debug("stop m_timerGrab for msg.\n");
    ev_timer_stop(lzddz.loop, &m_timerGrab);

    //记录状态
    m_opState[m_curSeat] = act ? OP_GRAB_RECEIVE_Y : OP_GRAB_RECEIVE_N;
    m_act.push_back(act ? SS_GRAB : SS_NOGR);
    //xt_log.debug("msg grab, m_uid:%d, seatid:%d, act:%s\n", player->m_uid, player->m_seatid, act ? "true" : "false");
    logicLord();
}

void Table::msgDouble(Player* player)
{
    //检查状态
    if(m_state != STATE_DOUBLE)
    {
        xt_log.error("%s:%d, double fail!, game state not double_state, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_DOUBLE, CODE_STATE);
        return;
    }

    //地主不能加倍
    if(player->m_seatid == m_lordSeat) 
    {
        xt_log.error("%s:%d, double fail!, lord no allow double. uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        sendError(player, CLIENT_DOUBLE, CODE_LORD);
        return; 
    }

    //处于等待加倍
    if(m_opState[player->m_seatid] != OP_DOUBLE_NOTIFY) 
    {
        xt_log.error("%s:%d, double fail!, not in double_notify uid:%d, opstate:%s\n", __FILE__, __LINE__, player->m_uid, DESC_OP[m_opState[player->m_seatid]]); 
        sendError(player, CLIENT_DOUBLE, CODE_NOTIFY);
        return; 
    }

    //不能重复加倍
    if(m_famerDouble[player->m_seatid]) 
    {
        xt_log.error("%s:%d, double fail!, no allow double repeat. uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        sendError(player, CLIENT_DOUBLE, CODE_DOUBLE);
        return; 
    }

    //托管中
    if(m_entrust[player->m_seatid])
    {
        xt_log.error("%s:%d, double fail!, entrusting. playerSeat:%d\n", __FILE__, __LINE__, player->m_seatid); 
        sendError(player, CLIENT_CALL, CODE_ENTRUST);
        return; 
    }

    Json::Value &msg = player->client->packet.tojson();
    m_famerDouble[player->m_seatid] = msg["double"].asBool();
    xt_log.debug("msgdouble, m_uid:%d, seatid:%d, double:%s\n", player->m_uid, player->m_seatid, m_famerDouble[player->m_seatid] ? "true" : "false");

    //加倍不分先后
    m_opState[player->m_seatid] = OP_DOUBLE_RECEIVE;

    //xt_log.debug("double continue!\n");
    sendDouble(player->m_uid, m_famerDouble[player->m_seatid]);

    logicDouble(true);
}

void Table::msgOut(Player* player)
{
    //检查状态
    if(m_state != STATE_OUT)
    {
        xt_log.error("%s:%d, out fail! game state not out_state, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_OUT, CODE_STATE);
        return;
    }

    //是否当前操作者
    if(m_curSeat != player->m_seatid)
    {
        xt_log.error("%s:%d, out fail!, operator error. m_curSeat:%d, playerSeat:%d\n", __FILE__, __LINE__, m_curSeat, player->m_seatid); 
        sendError(player, CLIENT_OUT, CODE_CURRENT);
        return; 
    }

    //托管中
    if(m_entrust[player->m_seatid])
    {
        xt_log.error("%s:%d, out fail!, entrusting. playerSeat:%d\n", __FILE__, __LINE__, player->m_seatid); 
        sendError(player, CLIENT_OUT, CODE_ENTRUST);
        return; 
    }

    Json::Value &msg = player->client->packet.tojson();

    //原来牌型
    vector<Card> curCard;
    json_array_to_vector(curCard, player->client->packet, "card");

    //解析癞子变化
    vector<int> lzvalue;
    jsonArrayToVector(lzvalue, player->client->packet, "change");

    //不出校验
    bool keep = msg["keep"].asBool();

    //xt_log.debug("msgOut, m_uid:%d, seatid:%d, keep:%s\n", player->m_uid, player->m_seatid, keep ? "true" : "false");
    //xt_log.debug("curCard:\n");
    //show(curCard);
    //xt_log.debug("lastCard:\n");
    //show(m_lastCard);
    //
    if(keep && !curCard.empty())
    {
        xt_log.error("%s:%d, out fail! not allow keep && not empty card. m_uid:%d, seatid:%d, keep:%s\n", __FILE__, __LINE__, player->m_uid, player->m_seatid, keep ? "true" : "false"); 
        xt_log.debug("curCard:\n");
        show(curCard);
        sendError(player, CLIENT_OUT, CODE_KEEP);
        return;
    }

    //校验手牌存在
    if(!checkCard(player->m_seatid, curCard))
    {
        xt_log.error("%s:%d, out fail! outcard not in hand!. m_uid:%d, seatid:%d, \n", __FILE__, __LINE__, player->m_uid, player->m_seatid); 
        //show(m_bottomCard);
        show(curCard);
        show(m_seatCard[player->m_seatid].m_cards);
        sendError(player, CLIENT_OUT, CODE_CARD_EXIST);
        return;
    }

    //没有上轮牌（新一轮）的时候，不能出pass
    if(keep && m_lastCard.empty())
    {
        xt_log.error("%s:%d, out fail! wrong pass. m_uid:%d, seatid:%d, \n", __FILE__, __LINE__, player->m_uid, player->m_seatid); 
        show(m_seatCard[player->m_seatid].m_cards);
        return;
    }

    //变化后的牌型
    //xt_log.debug("lz size:%d\n", lzvalue.size()); 
    if(!lzvalue.empty())
    {
        /*
        for(size_t i = 0; i < lzvalue.size(); ++i)
        {
            xt_log.debug("lz value:%d\n", lzvalue[i]); 
        }
        */
        //show(curCard, "befor change:");
        //要校验不能变成大小王
        m_deck.changeCard(curCard, lzvalue);
        //show(curCard, "after change:");
    }

    //清除超时用户
    m_timeout[player->m_seatid] = false;

    //停止出牌定时器
    //xt_log.debug("stop m_timerOut for msg.\n");
    ev_timer_stop(lzddz.loop, &m_timerOut);

    logicOut(player, curCard, keep);
}

void Table::msgChange(Player* player)
{
    if(m_state != STATE_PREPARE)
    {
        sendError(player, CLIENT_CHANGE, CODE_STATE);
        return;
    }

    xt_log.debug("msgChange, m_uid:%d, seatid:%d\n", player->m_uid, player->m_seatid);

    //清理座位信息
    map<int, Player*>::iterator it = m_players.find(player->m_uid);
    if(it != m_players.end())
    {
        m_players.erase(it);
        //清理座位信息
        setSeat(0, player->m_seatid);
        m_opState[player->m_seatid] = OP_PREPARE_WAIT; 
    }

    lzddz.game->change_table(player);
}

void Table::msgView(Player* player)
{
    Json::Value &msg = player->client->packet.tojson();
    int uid = msg["uid"].asInt();
    int index = uid % lzddz.main_size;

    if (lzddz.main_rc[index]->command("hgetall hu:%d", uid) < 0) {
        xt_log.error("msg view fail! 1, get player infomation error. uid:%d\n", uid);
        sendError(player, CLIENT_VIEW, CODE_NOEXIST);
    }

    if (lzddz.main_rc[index]->is_array_return_ok() < 0) {
        xt_log.error("msg view fail! 2, get player infomation error. uid:%d\n", uid);
        sendError(player, CLIENT_VIEW, CODE_NOEXIST);
    }

    int money = lzddz.main_rc[index]->get_value_as_int("money");
    double total = lzddz.main_rc[index]->get_value_as_int("total");
    int victory = lzddz.main_rc[index]->get_value_as_int("victory");
    double victory_prob = (total > 0) ? (victory / total) : 0; 

    Jpacket packet;
    packet.val["cmd"]           = SERVER_RESPOND;
    packet.val["msgid"]         = CLIENT_VIEW;
    packet.val["uid"]           = uid;
    packet.val["code"]          = CODE_SUCCESS;
    packet.val["name"]          = lzddz.main_rc[index]->get_value_as_string("name");
    packet.val["avatar"]        = lzddz.main_rc[index]->get_value_as_string("avatar");
    packet.val["sex"]           = lzddz.main_rc[index]->get_value_as_int("sex");
    packet.val["money"]         = money;
    packet.val["level"]         = lzddz.main_rc[index]->get_value_as_int("level");
    packet.val["title"]         = getTitle(money);                                   //头衔
    packet.val["victory_num"]   = victory;                                           //胜场
    packet.val["victory_prob"]  = victory_prob;                                      //胜率
    packet.end();
    unicast(player, packet.tostring());
}

void Table::msgEntrust(Player* player)
{
    //检查状态
    if(m_state == STATE_PREPARE || m_state == STATE_END)
    {
        xt_log.error("%s:%d, entrust fail! game state not out_state, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_ENTRUST, CODE_OUT_ENTRUST);
        return;
    }

    Json::Value &msg = player->client->packet.tojson();
    bool entrust = msg["active"].asBool();

    //重复
    if(m_entrust[player->m_seatid] == entrust)
    {
        xt_log.error("%s:%d, entrust fail! op repeat, entrust:%s\n", __FILE__, __LINE__, entrust ? "true" : "false"); 
        sendError(player, CLIENT_ENTRUST, CODE_REPEAT_ENTRUST);
        return;
    }

    //当前操作人正好是托管人
    m_entrust[player->m_seatid] = entrust;
    if(entrust && player->m_seatid == m_curSeat)
    {
        entrustProc(true, m_curSeat);
    }
    sendEntrust(player->m_uid, entrust);
}

void Table::msgChat(Player* player)
{
    Json::Value &msg = player->client->packet.tojson();
    Jpacket packet;
    packet.val["cmd"]         = SERVER_CHAT;
    packet.val["uid"]         = player->m_uid;
    packet.val["chatid"]      = msg["chatid"].asInt();
    packet.val["content"]     = msg["content"].asString();
    packet.end();
    broadcast(NULL, packet.tostring());
}

void Table::msgMotion(Player* player)
{
    if(player->m_money < lzddz.game->MOTIONMONEY || player->m_money < lzddz.game->ROOMLIMIT)
    {
        sendError(player, CLIENT_MOTION, CODE_MONEY);
        return;
    }

    player->changeMoney(-lzddz.game->MOTIONMONEY);

    Json::Value &msg = player->client->packet.tojson();
    Jpacket packet;
    packet.val["cmd"]         = SERVER_MOTION;
    packet.val["target_id"]   = msg["target_id"].asInt();
    packet.val["src_id"]      = player->m_uid;
    packet.val["type"]        = msg["type"].asInt();
    packet.val["price"]       = lzddz.game->MOTIONMONEY;
    packet.end();
    broadcast(NULL, packet.tostring());
}

bool Table::sitdown(Player* player)
{
    int seatid = -1;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(getSeat(i) == 0) 
        {
            seatid = i; 
            break;
        }
    }
    if(seatid < 0)
    {
        xt_log.error("%s:%d, no empty seat. tid:%d, uid:%d\n", __FILE__, __LINE__, m_tid, player->m_uid); 
        return false; 
    }

    player->m_seatid = seatid;
    player->m_tid = m_tid;
    player->m_table_count = m_count;
    setSeat(player->m_uid, seatid);
    m_players[player->m_uid] = player;
    //xt_log.debug("sitdown uid:%d, seatid:%d\n", player->m_uid, seatid);
    return true;
}

void Table::loginUC(Player* player, int code)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_RESPOND;
    packet.val["code"]      = code;
    packet.val["msgid"]     = CLIENT_LOGIN;
    packet.val["tid"]       = m_tid;
    packet.val["seatid"]    = player->m_seatid;

    //pack other player info
    for(map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        if(it->first == player->m_uid)  
        {
            continue; 
        }
        Json::Value jval;          
        Player* pl = it->second;
        jval["uid"]     = pl->m_uid;
        jval["seatid"]  = pl->m_seatid;
        jval["name"]    = pl->m_name;
        jval["money"]   = pl->m_money;
        jval["level"]   = pl->m_level;
        jval["sex"]     = pl->m_sex;
        jval["avatar"]  = pl->m_avatar;
        jval["state"]   = m_opState[pl->m_seatid];
        packet.val["userinfo"].append(jval);
    }

    /*
    vector_to_json_array(m_seatCard[player->m_seatid].m_cards, packet, "card");

    //重登处理
    packet.val["state"]       = m_state;

    switch(m_state)
    {
        case STATE_PREPARE:
            {
            }
            break;
        case STATE_CALL:
            {
                //叫分
                for(unsigned int i = 0; i < SEAT_NUM; ++i)
                {
                    //packet.val["callScore"].append(m_callScore[i]);
                }
                //剩余时间
                packet.val["time"] = m_time;
            }
            break;
        case STATE_DOUBLE:
            {
                //加倍
                for(unsigned int i = 0; i < SEAT_NUM; ++i)
                {
                    packet.val["famerDouble"].append(m_famerDouble[i]);
                }
                //剩余时间
                packet.val["time"] = m_time;
            }
            break;
        case STATE_OUT:
            {
                //发牌
                //自己的手牌 
                vector_to_json_array(m_seatCard[player->m_seatid].m_cards, packet, "myCard");
                //上轮出的牌
                vector_to_json_array(m_lastCard, packet, "lastCard");
                //底牌
                vector_to_json_array(m_bottomCard, packet, "bottomCard");
                //上轮出牌者座位
                packet.val["outSeat"] = m_outSeat;
                //当前操作者座位
                packet.val["currentSeat"] = m_curSeat;
                //剩余时间
                packet.val["time"] = m_time;
            }
            break;
    }
    */

    packet.end();
    unicast(player, packet.tostring());
}
        
void Table::sendRelogin(Player* player)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_RELOGIN;

    //玩家信息
    for(map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        Json::Value jval;          
        Player* pl = it->second;
        jval["uid"]     = pl->m_uid;
        jval["seatid"]  = pl->m_seatid;
        jval["name"]    = pl->m_name;
        jval["money"]   = pl->m_money;
        jval["level"]   = pl->m_level;
        jval["sex"]     = pl->m_sex;
        jval["avatar"]  = pl->m_avatar;
        jval["userstate"]   = m_opState[pl->m_seatid];
        packet.val["userinfo"].append(jval);
    }

    //游戏状态
    packet.val["state"]         = m_state;
    //地主位置
    packet.val["lordSeat"]      = m_lordSeat;
    //自己的手牌 
    vector_to_json_array(m_seatCard[player->m_seatid].m_cards, packet, "myCard");
    //上轮出的牌
    vector_to_json_array(m_lastCard, packet, "lastCard");
    //底牌
    vector_to_json_array(m_bottomCard, packet, "bottomCard");
    //上轮出牌者座位
    packet.val["outSeat"]       = m_outSeat;
    //当前操作者座位
    packet.val["currentSeat"]   = m_curSeat;
    //剩余时间
    packet.val["time"]          = m_time;
    //农民加倍
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        packet.val["famerDouble"].append(m_famerDouble[i]);
    }
    packet.end();
    unicast(player, packet.tostring());
}

void Table::loginBC(Player* player)
{
    Jpacket packet;
    Json::Value jval;          
    Player* pl = player;
    jval["uid"]     = pl->m_uid;
    jval["seatid"]  = pl->m_seatid;
    jval["name"]    = pl->m_name;
    jval["money"]   = pl->m_money;
    jval["level"]   = pl->m_level;
    jval["sex"]     = pl->m_sex;
    jval["avatar"]  = pl->m_avatar;
    jval["state"]   = m_state;

    packet.val["userinfo"].append(jval);
    packet.val["cmd"]       = SERVER_LOGIN;

    /* //直接这样发，客户端解析有错误
       Jpacket packet;
       packet.val["uid"]       = player->m_uid;
       packet.val["seatid"]    = player->m_seatid;
       packet.val["name"]      = player->m_name;
       packet.val["money"]     = player->m_money;
       packet.val["level"]     = player->m_level;
       packet.val["sex"]       = player->m_sex;
       packet.val["avatar"]    = player->m_avatar;
       packet.val["state"]     = m_state;
       */

    packet.end();
    broadcast(player, packet.tostring());
}

bool Table::allocateCard(void)
{
    //底牌    
    if(!m_deck.getHoldCard(m_bottomCard, BOTTON_CARD_NUM))
    {
        xt_log.error("%s:%d, get bottom card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
        return false;
    }

    //xt_log.debug("allocateCard, bottonCard:\n");
    //show(m_bottomCard);

    //手牌
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(!m_deck.getHoldCard(m_seatCard[i].m_cards, HAND_CARD_NUM))
        {
            xt_log.error("%s:%d, get hand card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
            return false;
        }
        //xt_log.debug("uid:%d\n", getSeat(i));
        //show(m_seatCard[i].m_cards);
    }

    return true;
}

bool Table::allocateCardControl(void)
{
    //设置癞子
    m_deck.setLZ(15);
    
    //1张癞子牌给真人玩家 
    int lzface = m_deck.getLZ();
    vector<Card> specard; 
    m_deck.getFaceCard(lzface, specard, 1);

    //给真人玩家一副炸弹(癞子点+1)
    //face 范围3-15, 先把点数还原到0-13范围，再加1,(lzface - 3 + 1) 再恢复
    int bombface = (lzface - 2) % 13 + 3;
    m_deck.getFaceCard(bombface, specard, 4);

    int bombface2 = (lzface - 1) % 13 + 3;
    m_deck.getFaceCard(bombface2, specard, 4);

    //给真人玩家一对(癞子点+2)
    //int doubleface = (lzface - 1) % 13 + 3;
    //m_deck.getFaceCard(doubleface, specard, 2);

    //不包括癞子的连续3-14, 癞子已经给过
    /*
    for(int i = 3; i <= 14; ++i)
    {
        if(i != lzface)
        {
            m_deck.getFaceCard(i, specard, 1);
        }
    }
    */

    //飞机
    /*
    for(int i = 3; i <= 5; ++i)
    {
        m_deck.getFaceCard(i, specard, 3);
    }

    for(int i = 6; i <= 8; ++i)
    {
        m_deck.getFaceCard(i, specard, 2);
    }
    */
    
    m_deck.delCard(specard);

    vector<Card> bottomcard; 
    m_deck.getFaceCard(lzface, bottomcard, 3);
    m_deck.delCard(bottomcard);

    
    //手牌
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(m_curSeat == i)
        {
            m_seatCard[i].m_cards.assign(specard.begin(), specard.end());
            if(!m_deck.getHoldCard(m_seatCard[i].m_cards, HAND_CARD_NUM - specard.size()))
            {
                xt_log.error("%s:%d, get hand card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
                return false;
            }
        }
        else
        {
            if(!m_deck.getHoldCard(m_seatCard[i].m_cards, HAND_CARD_NUM))
            {
                xt_log.error("%s:%d, get hand card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
                return false;
            }
        }
        //xt_log.debug("uid:%d\n", getSeat(i));
        //show(m_seatCard[i].m_cards);
    }

    //底牌    
    m_bottomCard.assign(bottomcard.begin(), bottomcard.end());
    /*
    if(!m_deck.getHoldCard(m_bottomCard, BOTTON_CARD_NUM))
    {
        xt_log.error("%s:%d, get bottom card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
        return false;
    }
    */

    //xt_log.debug("allocateCard, bottonCard:\n");
    //show(m_bottomCard);

    return true;
}

void Table::prepareProc(void)
{
    m_state = STATE_PREPARE; 
    setAllSeatOp(OP_PREPARE_WAIT);
}

void Table::callProc(void)
{
    m_state = STATE_CALL; 
    setAllSeatOp(OP_CALL_WAIT);
    m_opState[m_curSeat] = OP_CALL_NOTIFY;
    m_time = lzddz.game->CALLTIME;
    ev_timer_again(lzddz.loop, &m_timerLord);
    //xt_log.debug("m_timerLord first start.\n");
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::grabProc(void)
{
    //m_state = STATE_GRAB; 
    //setAllSeatOp(OP_GRAB_WAIT);
    //m_opState[m_curSeat] = OP_GRAB_NOTIFY;
    m_time = lzddz.game->CALLTIME;
    ev_timer_again(lzddz.loop, &m_timerGrab);
    //xt_log.debug("m_timerGrab first start.\n");
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::doubleProc(void)
{
    //xt_log.debug("doubleProc \n");
    m_state = STATE_DOUBLE; 
    setAllSeatOp(OP_DOUBLE_NOTIFY);
    m_time = lzddz.game->DOUBLETIME;

    ev_timer_again(lzddz.loop, &m_timerDouble);
    //xt_log.debug("m_timerDouble first start \n");
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::outProc(void)
{
    m_state = STATE_OUT;
    setAllSeatOp(OP_OUT_WAIT);
    m_curSeat = m_lordSeat;
    m_preSeat = m_curSeat;
    m_time = lzddz.game->OUTTIME;
    ev_timer_again(lzddz.loop, &m_timerOut);
    payTax();
    //xt_log.debug("m_timerOut first start \n");
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::logout(Player* player)
{
    xt_log.debug("player logout, uid:%d\n", player->m_uid);
    //退出发生后，牌桌内只有机器人,重新进入准备状态，方便测试
    map<int, Player*>::iterator it = m_players.find(player->m_uid);
    if(it != m_players.end())
    {
        m_players.erase(it);
        //清理座位信息
        setSeat(0, player->m_seatid);
        m_opState[player->m_seatid] = OP_PREPARE_WAIT; 
    }

    /* test
    reset(); 
    //通知机器人重新准备
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        xt_log.debug("reparepare: uid%d \n", it->first);
        Jpacket packet;
        packet.val["cmd"]           = SERVER_REPREPARE;
        packet.val["uid"]           = it->first;
        packet.end();
        unicast(it->second, packet.tostring());
    }
    */

}

void Table::leave(Player* player)
{
    xt_log.debug("%s %d player :%d %s leave.\n", __FILE__, __LINE__, player->m_uid, player->m_name.c_str());
    //如果游戏中，进入托管
    if(m_state != STATE_PREPARE)
    {
        xt_log.debug("entrust for leave!\n");
        m_entrust[player->m_seatid] = true;
        entrustProc(true, player->m_seatid);
    }
    sendLogout(player);

    //准备阶段断线，清理玩家，避免断线重连relogin
    if(m_state == STATE_PREPARE)
    {
        lzddz.game->del_player(player);
    }
}

void Table::endProc(void)
{
    m_state = STATE_END;
    setAllSeatOp(OP_GAME_END);
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);

    //累计一次牌局次数
    m_count++;
    //计算各座位输赢
    calculate();
    //修改玩家金币
    payResult();
    //统计局数和胜场
    total();
    //通知结算
    sendEnd();
    //增加经验
    addPlayersExp(); 
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
    //清空seatid
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        it->second->m_seatid = 0;
    }
    winProc();

    //重置游戏
    reset();
}

void Table::entrustProc(bool killtimer, int entrustSeat)
{
    switch(m_state)
    {
        case STATE_CALL:
            {
                if(killtimer)
                {
                    ev_timer_stop(lzddz.loop, &m_timerLord);
                    xt_log.debug("stop m_timerLord for entrust. \n");
                }
                logicLord();
                //sendEntrustCall(getSeatPlayer(entrustSeat), m_callScore[entrustSeat]); 
            }
            break;
        case STATE_GRAB:
            {
                if(killtimer)
                {
                    xt_log.debug("stop m_timerGrab for entrust. \n");
                    ev_timer_stop(lzddz.loop, &m_timerGrab);
                }
                logicLord();
                //sendEntrustCall(getSeatPlayer(entrustSeat), m_callScore[entrustSeat]); 
            }
            break;
        case STATE_DOUBLE:
            {
                if(killtimer)
                {
                    ev_timer_stop(lzddz.loop, &m_timerDouble);
                }
                m_famerDouble[entrustSeat] = true;
                m_opState[entrustSeat] = OP_DOUBLE_RECEIVE;

                sendEntrustDouble(getSeatPlayer(entrustSeat), true);

                sendDouble(m_seats[entrustSeat], m_famerDouble[entrustSeat]);
                logicDouble(false);
            }
            break;
        case STATE_OUT:
            {
                if(killtimer)
                {
                    ev_timer_stop(lzddz.loop, &m_timerOut);
                }
                ev_timer_again(lzddz.loop, &m_timerEntrustOut);
            }
            break;
    }
}
        
void Table::logicLord(void)
{
    //无人叫地主，重现发牌
    if(isNoCall())
    {
        xt_log.debug("nobody call, send card again.\n");
        gameRestart(); 
        return;
    }

    //异常，重新发牌
    if(m_act.size() > 6)
    {
        xt_log.error("act size > 6, key:%d, send card again.\n", act2key());
        showAct();
        gameRestart(); 
        return;
    }

    //选择地主, 开始发底牌
    if(selectLord(m_lordSeat))
    {
        xt_log.debug("select lord uid:%d\n", getSeat(m_lordSeat));
        doubleProc();
        addBottom2Lord();
        sendGrabResult();
        return; 
    }

    //选择下一个人
    getNext();
    bool notifycall = selectCall(); 
    if(notifycall)
    {
        sendCall();
        //叫地主托管
        if(m_entrust[m_curSeat])
        {
            entrustProc(true, m_curSeat);
        }
        else
        {
            //xt_log.debug("%s:%d, m_timerLord again.\n",__FILE__, __LINE__); 
            ev_timer_again(lzddz.loop, &m_timerLord);
        }
    }
    else
    {
        m_state = STATE_GRAB;
        sendGrab();
        //抢地主托管
        if(m_entrust[m_curSeat])
        {
            entrustProc(false, m_curSeat);
        }
        else
        {
            //xt_log.debug("m_timerGrab again.\n");
            ev_timer_again(lzddz.loop, &m_timerGrab);
        }
    }
}

void Table::logicDouble(bool isMsg)
{
    //xt_log.debug("logicDouble\n");
    if(isDoubleFinish())
    {
        xt_log.debug("=======================================start out card, double finish!\n");
        //showGame();
        outProc();
        //发送加倍结果，开始出牌
        sendDoubleResult(); 

        //如果托管直接自动处理
        if(m_entrust[m_curSeat])
        {
            entrustProc(true, m_curSeat);
        }

        if(isMsg)
        {
            //停止加倍定时器
            //xt_log.debug("stop m_timerDouble for msg.\n");
            ev_timer_stop(lzddz.loop, &m_timerDouble);
        }
    }
}

void Table::logicOut(Player* player, vector<Card>& curCard, bool keep)
{
    if(!curCard.empty())
    {
        //牌型校验
        Card::sortByDescending(curCard);
        int cardtype = m_deck.getCardType(curCard);
        if(cardtype == CT_ERROR)
        {
            xt_log.error("%s:%d,out fail! cardtype error. m_uid:%d, seatid:%d, keep:%s\n", __FILE__, __LINE__, player->m_uid, player->m_seatid, keep ? "true" : "false"); 
            xt_log.debug("curCard:\n");
            show(curCard);
            sendError(player, CLIENT_OUT, CODE_CARD);
            return;
        }
        //记录炸弹
        if(cardtype == CT_BOMB || cardtype == CT_ROCKET)
        {
            m_bomb[player->m_seatid]++; 
        }
    }

    //出牌次数
    m_outNum[player->m_seatid] += 1;

    if(m_lastCard.empty())
    {//首轮出牌
        //xt_log.debug("first round\n");
        m_lastCard = curCard;
        m_outSeat = player->m_seatid;
    }
    else if(keep && curCard.empty())
    {//不出
        //xt_log.debug("keep\n");
    }
    else if(m_outSeat == player->m_seatid)
    {//自己的牌
        //xt_log.debug("self card, new start\n");
    }
    else if(!curCard.empty())
    {//出牌
        //xt_log.debug("compare\n");
        if(!m_deck.compare(curCard, m_lastCard))
        {
            xt_log.error("%s:%d, compare fail.\n", __FILE__, __LINE__); 
            xt_log.error("curCard:%d\n", curCard.size());
            show(curCard);
            xt_log.error("lastCard:%d\n", m_lastCard.size());
            show(m_lastCard);
            sendError(player, CLIENT_OUT, CODE_COMPARE);
            return;
        }
        m_lastCard = curCard;
        m_outSeat = player->m_seatid;
    }


    //扣除手牌
    if(!curCard.empty())
    {
        m_seatCard[player->m_seatid].popCard(curCard);
    }
    //xt_log.debug("logicOut, cur size:%d\n", m_seatCard[player->m_seatid].m_cards.size());

    //判定结束
    if(m_seatCard[player->m_seatid].m_cards.empty())
    {
        //发送最后一轮出牌
        getNext(); 
        sendOutAgain(true);    
        xt_log.debug("=======================================gameover\n");
        m_win = player->m_seatid;
        endProc();
    }
    //挑选下一个操作者
    else if(getNext())
    {
        //如果没人接出牌者的牌
        if(m_curSeat == m_outSeat)
        {
            //xt_log.debug("无人接牌， 新一轮\n");
            m_lastCard.clear(); 
        }

        m_time = lzddz.game->OUTTIME;
        //发送出牌
        sendOutAgain(false);    

        //如果下一个出牌人托管
        if(m_entrust[m_curSeat])
        {
            entrustProc(false, m_curSeat);
        }
        else
        {
            if(m_timeout[m_curSeat])
            {//上次有超时过，缩短计时
                ev_timer_set(&m_timerOut, ev_tstamp(lzddz.game->SECOND_OUTTIME), ev_tstamp(lzddz.game->SECOND_OUTTIME));
            }
            else
            {
                ev_timer_set(&m_timerOut, ev_tstamp(lzddz.game->OUTTIME), ev_tstamp(lzddz.game->OUTTIME));
            }
            //开启出牌定时器
            //xt_log.debug("m_timerOut again start \n");
            ev_timer_again(lzddz.loop, &m_timerOut);
        }
    }
}

void Table::sendCard1(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_CARD_1;
        packet.val["face"]          = m_deck.getLZ();
        vector_to_json_array(m_seatCard[pl->m_seatid].m_cards, packet, "card");
        packet.end();
        unicast(pl, packet.tostring());
    }

    /*
    xt_log.debug("sendCard1, face:%d\n", m_deck.getLZ());
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        xt_log.debug("seatid:%d, uid:%d\n", pl->m_seatid, pl->m_uid);
        show(m_seatCard[pl->m_seatid].m_cards);    
    }
    */
}

void Table::sendCall(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_CALL;
        packet.val["time"]          = lzddz.game->CALLTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["show_time"]     = lzddz.game->SHOWTIME;
        packet.end();
        unicast(pl, packet.tostring());
    }
    //xt_log.debug("sendCall, cur_id:%d\n", getSeat(m_curSeat));
}

void Table::sendCallRsp(bool act)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_CALL_RSP;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["act"]           = act;
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendGrab(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_GRAB;
        packet.val["time"]          = lzddz.game->CALLTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.end();
        unicast(pl, packet.tostring());
    }
    //xt_log.debug("sendGrab, cur_id:%d\n", getSeat(m_curSeat));
}

void Table::sendGrabRsp(bool act)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_GRAB_RSP;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["act"]           = act;
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendGrabResult(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_RESULT_GRAB;
        packet.val["time"]          = lzddz.game->DOUBLETIME;
        packet.val["lord"]          = getSeat(m_lordSeat);
        vector_to_json_array(m_bottomCard, packet, "card");
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendDouble(int uid, bool isDouble)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_DOUBLE;
        packet.val["pre_id"]        = uid;
        packet.val["count"]         = getCount();
        packet.val["double"]        = isDouble;
        packet.end();
        unicast(pl, packet.tostring());
        //xt_log.debug("sendDouble: cmd:%d, uid:%d, count:%d, isDouble:%s\n", SERVER_DOUBLE, uid, getCount(), isDouble ? "true" : "false");
    }
}

void Table::sendDoubleResult(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_RESULT_DOUBLE;
        packet.val["time"]          = lzddz.game->OUTTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["count"]         = getCount();
        packet.end();
        unicast(pl, packet.tostring());
    }
    //xt_log.debug("sendDoubleResult: cmd:%d, lord_seat:%d, lord_uid:%d, count:%d\n", SERVER_RESULT_DOUBLE, m_curSeat, getSeat(m_curSeat), getCount());
}

void Table::sendOutAgain(bool last)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_AGAIN_OUT;
        packet.val["time"]          = lzddz.game->OUTTIME;
        packet.val["last"]          = last;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["pre_id"]        = getSeat(m_preSeat);
        packet.val["out_id"]        = getSeat(m_outSeat);
        packet.val["keep"]          = (m_preSeat != m_outSeat);
        packet.val["num"]           = static_cast<int>(m_seatCard[m_outSeat].m_cards.size());
        vector_to_json_array(m_lastCard, packet, "card");
        vectorToJsonArray(m_lastCard, packet, "change");
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendLogout(Player* player)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_LOGOUT;
    packet.val["uid"]       = player->m_uid;
    packet.end();
    broadcast(player, packet.tostring());
}

void Table::sendEnd(void)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_END;
    packet.val["code"]      = CODE_SUCCESS;
    packet.val["double"]    = getResultDoulbe();
    packet.val["bomb"]      = getBombNum();
    packet.val["score"]     = lzddz.game->ROOMSCORE;

    //xt_log.debug("end info: double:%d, bomb:%d, score:%d\n", doubleNum, getBombNum(), lzddz.game->ROOMSCORE);

    for(map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        Json::Value jval;          
        Player* pl = it->second;
        jval["uid"]     = pl->m_uid;
        jval["name"]    = pl->m_name;
        jval["money"]   = m_money[pl->m_seatid];
        jval["isLord"]  = (pl->m_seatid == m_lordSeat);
        packet.val["info"].append(jval);
        //xt_log.debug("end info: uid:%d, name:%s, money:%d\n", pl->m_uid, pl->m_name.c_str(), m_money[pl->m_seatid]);
    }

    packet.end();
    broadcast(NULL, packet.tostring());

}

void Table::sendTime(void)
{
    //xt_log.debug("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$sendTime\n");
    Jpacket packet;
    packet.val["cmd"]       = SERVER_TIME;
    packet.val["time"]      = m_time;
    packet.end();
    broadcast(NULL, packet.tostring());
}

void Table::sendPrepare(Player* player)
{
    //xt_log.debug("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$sendPrepare\n");
    Jpacket packet;
    packet.val["cmd"]       = SERVER_PREPARE;
    packet.val["uid"]       = player->m_uid;
    packet.end();
    broadcast(NULL, packet.tostring());
}

void Table::sendError(Player* player, int msgid, int errcode)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_RESPOND;
    packet.val["code"]      = errcode;
    packet.val["msgid"]     = msgid;
    packet.end();
    unicast(player, packet.tostring());
    xt_log.error("error msg, msgid:%d, code:%d\n", msgid, errcode);
}

void Table::sendEntrustOut(Player* player, vector<Card>& curCard, bool keep)
{
    Jpacket packet;
    packet.val["cmd"]           = SERVER_ENTRUST_OUT;
    packet.val["keep"]          = keep;
    vector_to_json_array(curCard, packet, "card");
    packet.end();
    unicast(player, packet.tostring());
}

void Table::sendEntrustCall(Player* player, int score)
{
    Jpacket packet;
    packet.val["cmd"]           = SERVER_ENTRUST_CALL;
    packet.val["score"]         = score;
    packet.end();
    unicast(player, packet.tostring());
}

void Table::sendEntrustDouble(Player* player, bool dou)
{
    Jpacket packet;
    packet.val["cmd"]           = SERVER_ENTRUST_DOUBLE;
    packet.val["double"]        = dou;
    packet.end();
    unicast(player, packet.tostring());
}

void Table::sendEntrust(int uid, bool active)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_ENTRUST;
    packet.val["uid"]       = uid;
    packet.val["active"]    = active;
    packet.end();
    broadcast(NULL, packet.tostring());
}

void Table::gameStart(void)
{
    /*测试真人优先
    {
        for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
       {
            if(it->second->isRobot() == false)
            {
                m_curSeat = it->second->m_seatid; 
            }
       }
    }
    */

    m_curSeat = rand() % SEAT_NUM;

    m_firstSeat = m_curSeat;
    xt_log.debug("=======================================start send card, cur_id:%d, next_id:%d, next_id:%d, tid:%d\n",
            getSeat(m_curSeat), getSeat((m_curSeat + 1) % SEAT_NUM), getSeat((m_curSeat + 2) % SEAT_NUM), m_tid);

    callProc();

    allocateCard();
    //allocateCardControl();

    sendCard1();

    ev_timer_stop(lzddz.loop, &m_timerWaitCall);
    ev_timer_again(lzddz.loop, &m_timerWaitCall);

    ev_timer_stop(lzddz.loop, &m_timerUpdate);
    ev_timer_again(lzddz.loop, &m_timerUpdate);


    //如果托管直接自动处理
    if(m_entrust[m_curSeat])
    {
        entrustProc(true, m_curSeat);
    }
}

void Table::gameRestart(void)
{
    //重置部分数据
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        m_famerDouble[i] = false;
        m_seatCard[i].reset();
        m_bomb[i] = 0;
        m_outNum[i] = 0;
        m_money[i] = 0;
    }
    m_bottomCard.clear();
    m_lastCard.clear();
    m_deck.shuffle(m_tid);
    m_curSeat = 0;
    m_preSeat = 0;
    m_lordSeat = 0;
    m_outSeat = 0;
    m_topCall = 0;
    m_win = 0;
    m_act.clear();

    xt_log.debug("restart.\n");
    gameStart();
}

bool Table::getNext(void)
{
    int nextSeat = (m_curSeat + 1) % SEAT_NUM;
    switch(m_state)
    {
        case STATE_CALL:
        case STATE_GRAB:
            {
                m_preSeat = m_curSeat;
                m_curSeat = nextSeat;
                m_opState[m_curSeat] = OP_CALL_NOTIFY;
                //xt_log.debug("get next call success, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
                return true; 
            }
        case STATE_OUT:
            {//校验出牌时间戳
                m_preSeat = m_curSeat;
                m_curSeat = nextSeat;
                //xt_log.debug("get next success, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
                return true;
            }
            break;
    }

    xt_log.debug("get next finish false, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
    return false;
}

Player* Table::getSeatPlayer(unsigned int seatid)
{
    int uid = getSeat(seatid);
    map<int, Player*>::const_iterator it = m_players.find(uid);
    if(it != m_players.end())
    {
        return it->second; 
    }

    xt_log.error("%s:%d, getSeatPlayer error! seatid:%d\n",__FILE__, __LINE__, seatid); 
    return NULL;
}

void Table::setAllSeatOp(int state)
{
    for(unsigned int i = 0; i < SEAT_NUM; ++i) 
    {
        m_opState[i] = state; 
    }
}

bool Table::allSeatFit(int state)
{
    for(unsigned int i = 0; i < SEAT_NUM; ++i) 
    {
        if(m_opState[i] != state) 
        {
            //xt_log.debug("seatid:%d state is %s, check state is %s\n", i, DESC_OP[m_opState[i]], DESC_OP[state]);
            return false; 
        }
    }
    return true;
}

int Table::getCount(void)
{
    int ret = 1;
    for(unsigned int i = 0; i < SEAT_NUM; ++i) 
    {
        if(m_famerDouble[i])
        {
            ret *= 2;
        }
    }
    return ret;
}

static string printStr;
void Table::show(const vector<Card>& card)
{
    printStr.clear();
    for(vector<Card>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        //xt_log.debug("%s\n", it->getCardDescription());
        printStr.append(it->getCardDescriptionString());
        printStr.append(" ");
    }
    xt_log.debug("%s\n", printStr.c_str());
}
        
void Table::show(const vector<Card>& card, const char* msg)
{
    xt_log.debug("%s\n", msg);
    show(card);
}

void Table::showGame(void)
{
    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        xt_log.debug("uid:%d, money:%d, name:%s\n", tmpplayer->m_uid, tmpplayer->m_money, tmpplayer->m_name.c_str());
    }

    xt_log.debug("tid:%d, seat0:%d, seat1:%d, seat2:%d\n", m_tid, getSeat(0), getSeat(1), getSeat(2));
}
        
void Table::showAct(void)
{
    printStr.clear();
    for(vector<int>::const_iterator it = m_act.begin(); it != m_act.end(); ++it)
    {
        if( 0 < *it && *it < 5)
        {
            printStr.append(DESC_SS[*it]);
        }
        else
        {
            xt_log.error("error act val:%d\n", *it);
        }
        printStr.append(" ");
    }
    xt_log.error("act list: %s\n", printStr.c_str());
}

bool Table::isDoubleFinish(void)
{
    //所以农民已经回复加倍
    for(unsigned int i = 0; i < SEAT_NUM; ++i) 
    {
        if(m_opState[i] != OP_DOUBLE_RECEIVE && m_lordSeat != i)
        {
            return false; 
        }
    }
    return true;
}

int Table::getAllDouble(void)
{
    int ret = 0;

    //抢地主加倍
    int callDouble = m_grabDoulbe;
    //炸弹加倍
    int bombDouble = 0;
    //底牌加倍
    int bottomDouble = getBottomDouble();
    //春天加倍
    int springDouble = isSpring() ? 2 : 0;
    //反春天加倍
    int antiSpringDouble = isAntiSpring() ? 2 : 0;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        bombDouble += m_bomb[i];
    }
    //xt_log.debug("double: callDouble:%d, bombDouble:%d, bottomDouble:%d, springDouble:%d, antiSpringDouble:%d\n", callDouble, bombDouble, bottomDouble, springDouble, antiSpringDouble);
    ret = callDouble + bombDouble + bottomDouble + springDouble + antiSpringDouble;
    return max(ret, 1);
}

int Table::getBottomDouble(void)
{
    bool littleJoke = false;
    bool bigJoke = false;
    set<int> suitlist; 
    set<int> facelist; 
    bool isContinue = m_deck.isNContinue(m_bottomCard, 1);
    for(vector<Card>::const_iterator it = m_bottomCard.begin(); it != m_bottomCard.end(); ++it)
    {
        if(it->m_value == 0x00) 
        {
            littleJoke = true;
        }
        else if(it->m_value == 0x10) 
        {
            bigJoke = true; 
        }
        suitlist.insert(it->m_suit);
        facelist.insert(it->m_face);
    }

    //火箭
    if(bigJoke && littleJoke)
    {        
        //printf("火箭");
        return 4;
    }

    //大王
    if(bigJoke && !littleJoke)
    {
        //printf("大王");
        return 2;
    }

    //小王
    if(!bigJoke && littleJoke)
    {
        //printf("小王");
        return 2;
    }

    //同花
    if(!isContinue && suitlist.size() == 1)
    {
        //printf("同花");
        return 3; 
    }

    //顺子
    if(isContinue && suitlist.size() != 1)
    {
        //printf("顺子");
        return 3; 
    }

    //同花顺
    if(isContinue && suitlist.size() == 1)
    {
        //printf("同花顺");
        return 4; 
    }

    //三同
    if(facelist.size() == 1)
    {
        //printf("三同");
        return 4; 
    }
    return 0;
}

bool Table::isSpring(void)
{
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(i != m_lordSeat && m_outNum[i] != 0)
        {
            return false; 
        }
    }
    return true;
}

bool Table::isAntiSpring(void)
{
    return m_outNum[m_lordSeat] == 1;
}

int Table::getBombNum(void)
{
    int ret = 0;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        ret +=  m_bomb[i]; 
    }
    return ret;
}

int Table::getMinMoney(void)
{
    int money = 0; 
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        Player* pl = getSeatPlayer(i);
        if(pl)
        {
            if(money == 0)
            {
                money = pl->m_money;
            }
            else if(pl->m_money < money)
            {
                money = pl->m_money;
            }
        }
    }
    return money;
}

void Table::payResult(void)
{
    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        tmpplayer->changeMoney(m_money[tmpplayer->m_seatid]);
    }
}

void Table::payTax(void)
{
    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        tmpplayer->changeMoney(-lzddz.game->ROOMTAX);
    }
}

void Table::total(void)
{
    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        tmpplayer->keepTotal(tmpplayer->m_seatid == m_win);
    }
}

void Table::calculate(void)
{
    //台面额度
    double score = getTableQuota();
    Player* lord = getSeatPlayer(m_lordSeat); 
    Player* big = getSeatPlayer((m_lordSeat + 1) % 3); 
    Player* small = getSeatPlayer((m_lordSeat + 2) % 3); 
    if(big->m_money < small->m_money)
    {
        std::swap(big, small);
    }

    //地主钱, 农民大，农民小
    double lordmoney = static_cast<double>(lord->m_money);
    double bigmoney = static_cast<double>(big->m_money);
    double smallmoney = static_cast<double>(small->m_money);

    ///////////////////////////////////////////////////////////////////以小博大限制
    //地主赢 地主持有≥2*台面  小农＜台面&大农≥台面 
    if(m_win == m_lordSeat && (lordmoney >= 2 * score) && smallmoney < score && bigmoney >= score)
    {
        m_money[small->m_seatid] = -smallmoney;
        m_money[big->m_seatid] = -score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("1\n");
        return;
    }

    //地主赢 地主持有≥2*台面 两个农民＜台面
    if(m_win == m_lordSeat && (lordmoney >= 2 * score) && (smallmoney + bigmoney) < score)
    {
        m_money[small->m_seatid] = -smallmoney;
        m_money[big->m_seatid] = -bigmoney;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("2\n");
        return;
    }

    //地主赢 地主持有＜2*台面
    if(m_win == m_lordSeat && lordmoney < 2 * score)
    {
        m_money[small->m_seatid] = -min(smallmoney, min((lordmoney * smallmoney / (smallmoney + bigmoney)), score));
        m_money[big->m_seatid] = -min(bigmoney, min((lordmoney * bigmoney / (smallmoney + bigmoney)), score));
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("3\n");
        return;
    }

    //农民赢 地主持有≥2*台面      小农＜台面&大农≥台面
    if(m_win != m_lordSeat && (lordmoney >= 2 * score) && smallmoney < score && bigmoney >= score)
    {
        m_money[small->m_seatid] = smallmoney;
        m_money[big->m_seatid] = score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("4\n");
        return;
    }

    //农民赢 地主持有≥2*台面     两个农民＜台面 
    if(m_win != m_lordSeat && (lordmoney >= 2 * score) && (smallmoney + bigmoney) < score)
    {
        m_money[small->m_seatid] = smallmoney;
        m_money[big->m_seatid] = score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("5\n");
        return;
    }

    //农民赢 地主持有＜2*台面
    if(m_win != m_lordSeat && (lordmoney < 2 * score))
    {
        m_money[small->m_seatid] = min(smallmoney, min((lordmoney * smallmoney / (smallmoney + bigmoney)), score));
        m_money[big->m_seatid] = min(bigmoney, min((lordmoney * bigmoney / (smallmoney + bigmoney)), score));
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("6\n");
        return;
    }

    ///////////////////////////////////////////////////////////////////正常情况
    //地主赢 正常结算
    if(m_win == m_lordSeat && (lordmoney >= 2 * score) && smallmoney >= score)
    {
        m_money[small->m_seatid] = -score;
        m_money[big->m_seatid] = -score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("7\n");
        return;
    }

    //农民赢 正常结算
    if(m_win != m_lordSeat && (lordmoney >= 2 * score) && smallmoney >= score)
    {
        m_money[small->m_seatid] = score;
        m_money[big->m_seatid] = score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("8\n");
        return;
    }

    xt_log.error("caculate error, lordname:%s, money:%f, big:%s, money:%f, small:%s, money:%f, score:%f\n"
            , lord->m_name.c_str(), lordmoney, big->m_name.c_str(), bigmoney, small->m_name.c_str(), smallmoney, score);
}

void Table::setSeat(int uid, int seatid)
{
    if(seatid < 0 || seatid >= static_cast<int>(SEAT_NUM))
    {
        xt_log.error("%s:%d, setSeat error seatid:%d, uid:%d\n", __FILE__, __LINE__, seatid, uid); 
        return; 
    }
    m_seats[seatid] = uid;
}

int Table::getSeat(int seatid)
{
    if(seatid < 0 || seatid >= static_cast<int>(SEAT_NUM))
    {
        xt_log.error("%s:%d, getSeat error seatid:%d\n", __FILE__, __LINE__, seatid); 
        return 0; 
    }
    return m_seats[seatid];
}

void Table::kick(void)
{
    //xt_log.debug("check kick.\n");
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        if(pl->m_money < lzddz.game->ROOMTAX || m_entrust[pl->m_seatid])
        {
            Jpacket packet;
            packet.val["cmd"]           = SERVER_KICK;
            packet.val["uid"]           = pl->m_uid;
            packet.end();
            unicast(NULL, packet.tostring());
            xt_log.debug("%s:%d, kick player for not enough money, uid:%d, seatid:%d, money:%d, roomtax:%d\n",__FILE__, __LINE__, pl->m_uid, pl->m_seatid, pl->m_money, lzddz.game->ROOMTAX); 
            m_delPlayer.push_back(pl);
            //不能这里删除，否则logout里有对m_players的删除操作,导致容器错误, 且要保证发送消息完毕
        }
    }

    ev_timer_again(lzddz.loop, &m_timerKick);
}

void Table::addRobotMoney(Player* player)
{
    if(!player->isRobot() || player->m_money > lzddz.game->ROOMTAX)
    {
        return;
    }

    int addval = lzddz.game->ROOMTAX * (rand() % 9 + 1) + 100000;
    //xt_log.debug("%s:%d, addRobotMoney, uid:%d, money:%d \n",__FILE__, __LINE__, player->m_uid, addval); 
    player->changeMoney(addval);
}

void Table::addPlayersExp(void)
{
    int exp = 0;
    Player* player = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        player = it->second;
        exp = money2exp(m_money[player->m_seatid]); 
        player->addExp(exp);
    }
}

void Table::winProc(void)
{
    Player* player = NULL;
    int score = static_cast<int>(getTableQuota());
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        player = it->second;
        if(m_money[player->m_seatid] > 0)
        {
            player->updateTopMoney(m_money[player->m_seatid]); 
            player->updateTopCount(score);
            player->coupon(score);
        }
    }
}
        
int Table::getTableQuota(void)
{
    //台面额度 = 底分 * 底牌加倍（没有为1） * 农民加倍（没有为1） * 叫分 * 2 ^（炸弹个数 + 春天或反春天1）
    //底分
    int score = lzddz.game->ROOMSCORE;
    //底牌加倍
    int bottomDouble = getBottomDouble();
    //农民加倍 范围1, 2, 4
    int famerDouble = getFamerDouble();
    //炸弹个数
    int bombnum = getBombNum();
    //春天
    int spring = isSpring() ? 1 : 0;
    //反春天
    int anti = isAntiSpring() ? 1 : 0;

    //2 ^（炸弹个数 + 春天或反春天1）
    double baseval = 2;
    double exponetval = bombnum + spring + anti;
    double tmp = pow(baseval, exponetval);

    int ret = static_cast<int>(score * bottomDouble * famerDouble * tmp); 
    /*
    xt_log.debug("%s:%d, getTableQuota, score:%d, bottomDouble:%d, famerDouble:%d, bombnum:%d, spring:%d, anti:%d, result:%d\n", __FILE__, __LINE__,
            score, bottomDouble, famerDouble, bombnum, spring, anti, ret); 
    */
    return ret;
}

int Table::getFamerDouble(void)
{
    int ret = 0;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(i != m_lordSeat && m_famerDouble[i]) 
        {
            ret++;  
        }
    }
    return max(ret * 2, 1);
}

int Table::getResultDoulbe(void)
{
    //结算框倍数 = 农民加倍（没有为1）* 叫分
    //农民加倍 范围1, 2, 4
    int famerDouble = getFamerDouble();
    /*
    xt_log.debug("%s:%d, getResultDoulbe, famerDouble:%d\n", __FILE__, __LINE__, famerDouble); 
     */
    return famerDouble;
}

int Table::money2exp(int money)
{
    if(money <= 0)
    {
        return 0; 
    }
    else if(money <= 1000)
    {
        return 1;
    }
    else if(money >= 1001 && money <= 10000)
    {
        return 2;
    }
    else if(money >= 10001 && money <= 100000)
    {
        return 3;
    }
    else if(money >= 100001 && money <= 1000000)
    {
        return 4;
    }
    else if(money >= 1000001)
    {
        return 5;
    }
    else
    {
        return 0;
    }
}

void Table::entrustOut(void)
{
    //xt_log.debug("entrustOut. m_curSeat:%d\n", m_curSeat);
    Player* player = getSeatPlayer(m_curSeat);
    bool keep = false;
    vector<Card> curCard;
    vector<Card> &myCard = m_seatCard[m_curSeat].m_cards;

    Card::sortByDescending(myCard);
    Card::sortByDescending(m_lastCard);
    //首轮出牌
    if(m_lastCard.empty())
    {
        m_deck.getFirst(myCard, curCard);
    }
    //没人跟自己的牌
    else if(m_curSeat == m_outSeat)
    {
        m_deck.getFirst(myCard, curCard);
    }
    //跟别人的牌
    else
    {
        m_deck.getFollow(myCard, m_lastCard, curCard);
        /*
           if(!curCard.empty() && CT_ERROR == m_deck.getCardType(curCard))
           {
           xt_log.debug("my card\n");
           show(myCard);
           xt_log.debug("last card\n");
           show(m_lastCard);
           xt_log.debug("select card\n");
           show(curCard);
           }
           */
    }
    keep = curCard.empty() ? true : false; 

    sendEntrustOut(player, curCard, keep); 
    //xt_log.debug("entrust out, uid:%d, keep:%s\n", player->m_uid, keep ? "true" : "false");
    //show(curCard);

    //判断是否结束和通知下一个出牌人，本轮出牌
    logicOut(player, curCard, keep);
}
        
bool Table::checkCard(unsigned int seatid, const vector<Card>& outcard)  
{
    if(seatid < 0 || seatid > SEAT_NUM)
    {
        return false;
    }

    if(outcard.empty())
    {
        return true;
    }

    set<int> tmpset;
    const vector<Card>& holdcard = m_seatCard[seatid].m_cards;
    for(vector<Card>::const_iterator it = holdcard.begin(); it != holdcard.end(); ++it)
    {
        tmpset.insert(it->m_value);    
    }

    for(vector<Card>::const_iterator it = outcard.begin(); it != outcard.end(); ++it)
    {
        if(tmpset.find(it->m_value) == tmpset.end())
        {
            return false; 
        }
    }

    return true;
}
        
void Table::addBottom2Lord(void)
{
    for(vector<Card>::const_iterator it = m_bottomCard.begin(); it != m_bottomCard.end(); ++it)
    {
        m_seatCard[m_lordSeat].m_cards.push_back(*it);
    }
}
        
bool Table::isNoCall(void)
{
   if(m_act.size() != 3) 
   {
        return false;
   }

   if(m_act[0] == m_act[1] && m_act[1] == m_act[2] && m_act[2] == static_cast<int>(SS_NOCA))
   {
        return true;
   }
   return false;
}
        
bool Table::selectLord(unsigned int& lordseat)
{
    if(m_act.size() < 3)
    {
        return false;
    }

    if(m_act.size() > 6)
    {
        //xt_log.debug("selectLord size:%d\n", m_act.size());
        return false;
    }

    int key = act2key();
    std::map<int, int>::const_iterator it = lzddz.game->m_select.find(key);
    if(it == lzddz.game->m_select.end())
    {
        return false; 
    }

    //xt_log.debug("m_firstSeat:%d, key:%d, value:%d\n", m_firstSeat, key, it->second);
    lordseat = (m_firstSeat + it->second) % SEAT_NUM;
    return true;
}
        
bool Table::selectCall(void)
{
    for(std::vector<int>::const_iterator it = m_act.begin(); it != m_act.end(); ++it)
    {
        if((*it) == SS_CALL) 
        {
            return false;
        }
    }
    return true;
}
        
int Table::act2key(void)
{
    if(m_act.size() < 2)
    {
        return 0;
    }

    int key = m_act[0];
    for(size_t i = 1; i < m_act.size(); ++i)
    {
        key = key | (m_act[i] << i * 3);
    }
    return key;
}
