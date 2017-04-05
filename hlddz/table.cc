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
const int OUTTIME          = 10;
const int ENDTIME           = 10;
const int KICKTIME          = 1;
const int UPDATETIME        = 1;

const int SHOWTIME          = 3;    //发牌动画时间
const int ROOMSCORE         = 10;   //房间底分
const int ROOMTAX           = 10;   //房间抽水

Table::Table()
{
    m_timerCall.data = this;
    ev_timer_init(&m_timerCall, Table::callCB, ev_tstamp(CALLTIME), ev_tstamp(CALLTIME));

    m_timerDouble.data = this;
    ev_timer_init(&m_timerDouble, Table::doubleCB, ev_tstamp(DOUBLETIME), ev_tstamp(DOUBLETIME));

    m_timerCard.data = this;
    ev_timer_init(&m_timerCard, Table::cardCB, ev_tstamp(OUTTIME), ev_tstamp(OUTTIME));

    m_timerKick.data = this;
    ev_timer_init(&m_timerKick, Table::kickCB, ev_tstamp(KICKTIME), ev_tstamp(KICKTIME));

    m_timerUpdate.data = this;
    ev_timer_init(&m_timerUpdate, Table::updateCB, ev_tstamp(UPDATETIME), ev_tstamp(UPDATETIME));
}

Table::~Table()
{
    ev_timer_stop(hlddz.loop, &m_timerCall);
    ev_timer_stop(hlddz.loop, &m_timerDouble);
    ev_timer_stop(hlddz.loop, &m_timerCard);
    ev_timer_stop(hlddz.loop, &m_timerKick);
    ev_timer_stop(hlddz.loop, &m_timerUpdate);
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
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        //重新下一局， 座位信息不能删除
        m_callScore[i] = 0;
        m_famerDouble[i] = false;
        m_seatCard[i].reset();
        m_bomb[i] = 0;
        m_outNum[i] = 0;
        m_money[i] = 0;
    }
    m_bottomCard.clear();
    m_lastCard.clear();
    m_deck.fill();
    m_deck.shuffle(m_tid);
    m_curSeat = 0;
    m_preSeat = 0;
    m_lordSeat = 0;
    m_outSeat = 0;
    m_topCall = 0;
    m_win = 0;
    prepareProc();
    m_time = 0;

    ev_timer_stop(hlddz.loop, &m_timerCall);
    ev_timer_stop(hlddz.loop, &m_timerDouble);
    ev_timer_stop(hlddz.loop, &m_timerCard);
    ev_timer_stop(hlddz.loop, &m_timerKick);
    ev_timer_stop(hlddz.loop, &m_timerUpdate);
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

void Table::vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
    if (cards.empty()) 
    {
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
    if(!val.isMember(key))
    {
        return;
    }

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
    table->onCall();
}

void Table::onCall(void)
{
}

void Table::doubleCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerDouble);
    table->onDouble();
}

void Table::onDouble(void)
{
}

void Table::cardCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerCard);
    table->onCard();
}

void Table::onCard(void)
{
}

void Table::kickCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerKick);
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
        hlddz.game->del_player(*it);
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
    --m_time;
    sendTime();
}

int Table::login(Player *player)
{
    xt_log.debug("player login m_uid:%d, tid:%d\n", player->m_uid, m_tid);
    if(m_players.find(player->m_uid) != m_players.end())
    {
        xt_log.error("%s:%d, login fail! player was existed! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        return 0;
    }

    //给机器人加钱
    addRobotMoney(player);

    //检查入场费
    if(player->m_money < ROOMTAX)
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
    int ret_code = CODE_SUCCESS;
    if(m_players.find(player->m_uid) == m_players.end())
    {
        xt_log.error("%s:%d, player was not existed! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        ret_code = CODE_RELOGIN;
    }

    //准备阶段
    loginUC(player, ret_code);
}

void Table::msgPrepare(Player* player)
{
    //xt_log.debug("msg prepare m_uid:%d, seatid:%d, size:%d\n", player->m_uid, player->m_seatid, m_players.size());
    //检查入场费
    if(player->m_money < ROOMTAX)
    {
        xt_log.error("%s:%d, prepare fail!, player was no enouth money! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        sendError(player, CLIENT_PREPARE, CODE_MONEY);
        return; 
    }

    //重复准备
    if(m_opState[player->m_seatid] == OP_PREPARE_REDAY)
    {
        xt_log.error("%s:%d, prepare fail!, player repeat prepare! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
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

    m_opState[player->m_seatid] = OP_PREPARE_REDAY; 
    if(!allSeatFit(OP_PREPARE_REDAY))
    {
        //xt_log.debug("not all is prepare.\n");
        return;
    }
    else if(m_players.size() != SEAT_NUM)
    {
        //xt_log.debug("not enouth player, size:%d\n", m_players.size());
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
    {//座位通知过
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

    //有效叫分
    Json::Value &msg = player->client->packet.tojson();
    int score = msg["score"].asInt();
    if(score < 0 || score > 3)
    {
        xt_log.error("%s:%d, call fail!, score error. uid:%d, score:%d\n", __FILE__, __LINE__, player->m_uid, score); 
        sendError(player, CLIENT_CALL, CODE_SCORE);
        return; 
    }

    //保存当前叫分
    m_callScore[m_curSeat] = score;
    //记录状态
    m_opState[m_curSeat] = OP_CALL_RECEIVE;
    //xt_log.debug("call score, m_uid:%d, seatid:%d, score :%d\n", player->m_uid, player->m_seatid, m_callScore[player->m_seatid]);

    //是否已经选出地主
    if(selecLord())
    {
        //选地主，进入加倍环节
        doubleProc();
        sendCallResult(); 
    }
    else if(getNext())
    {
        //广播当前叫分和下一个叫分
        sendCallAgain(); 
    }
    else
    {//重新发牌
        xt_log.debug("nobody call, need send card again.\n");
        gameRestart();
    }
}

void Table::msgDouble(Player* player)
{
    //check
    //地主不能加倍
    //不能重复加倍
    Json::Value &msg = player->client->packet.tojson();
    m_famerDouble[player->m_seatid] = msg["double"].asBool();
    //xt_log.debug("msgdouble, m_uid:%d, seatid:%d, double:%s\n", player->m_uid, player->m_seatid, m_famerDouble[player->m_seatid] ? "true" : "false");

    //加倍不分先后
    m_opState[player->m_seatid] = OP_DOUBLE_RECEIVE;

    //xt_log.debug("double continue!\n");
    sendDouble(player->m_uid, m_famerDouble[player->m_seatid]);

    if(isDoubleFinish())
    {
        xt_log.debug("=======================================start out card, double finish!\n");
        showGame();
        outProc();
        sendDoubleResult(); 
    }
}

void Table::msgOut(Player* player)
{
    //check
    Json::Value &msg = player->client->packet.tojson();

    vector<XtCard> curCard;
    json_array_to_vector(curCard, player->client->packet, "card");

    //不出校验
    bool keep = msg["keep"].asBool();
    if(keep && !curCard.empty())
    {
        xt_log.error("%s:%d, not allow keep && not empty card. m_uid:%d, seatid:%d, keep:%s\n", __FILE__, __LINE__, player->m_uid, player->m_seatid, keep ? "true" : "false"); 
        xt_log.debug("curCard:\n");
        show(curCard);
        return;
    }

    if(!curCard.empty())
    {
        //牌型校验
        XtCard::sortByDescending(curCard);
        int cardtype = m_deck.getCardType(curCard);
        if(cardtype == CT_ERROR)
        {
            xt_log.error("%s:%d, cardtype error. m_uid:%d, seatid:%d, keep:%s\n", __FILE__, __LINE__, player->m_uid, player->m_seatid, keep ? "true" : "false"); 
            xt_log.debug("curCard:\n");
            show(curCard);
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

    //xt_log.debug("msgOut, m_uid:%d, seatid:%d, keep:%s\n", player->m_uid, player->m_seatid, keep ? "true" : "false");
    //xt_log.debug("curCard:\n");
    //show(curCard);
    //xt_log.debug("lastCard:\n");
    //show(m_lastCard);

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
        if(!m_deck.compareCard(curCard, m_lastCard))
        {
            xt_log.error("%s:%d, compare fail.", __FILE__, __LINE__); 
            xt_log.error("curCard:\n");
            show(curCard);
            xt_log.error("lastCard:\n");
            show(m_lastCard);
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

    //判定结束
    if(m_seatCard[player->m_seatid].m_cards.empty())
    {
        xt_log.debug("=======================================gameover\n");
        m_win = player->m_seatid;
        endProc();
    }
    else if(getNext())
    {
        //如果没人接出牌者的牌
        if(m_curSeat == m_outSeat)
        {
            //xt_log.debug("无人接牌， 新一轮\n");
            m_lastCard.clear(); 
        }
        sendOutAgain();    
    }
}

void Table::msgChange(Player* player)
{
    xt_log.debug("msgChange, m_uid:%d, seatid:%d\n", player->m_uid, player->m_seatid);

    if(m_state == STATE_OUT)
    {
        //地主退，农民赢，反之亦然 
        if(player->m_seatid == m_lordSeat)  
        {
            m_win = (m_lordSeat + 1) % SEAT_NUM;
        }
        else
        {
            m_win = m_lordSeat;
        }

        m_state = STATE_END;
        setAllSeatOp(OP_GAME_END);

        //计算各座位输赢
        ////////////////////////////////////////////////////////////////////////
        int doubleNum = max(getAllDouble(), 1);
        int score = ROOMSCORE * doubleNum;
        //获取最小本钱
        int minMoney = getMinMoney(); 
        //最终数值
        int finalScore = min(score, minMoney);
        //xt_log.debug("endProc, befor score:%d, min money:%d, finalScore:%d\n", score, minMoney, finalScore);
        //计算各座位输赢
        calculate(finalScore);
        //修改玩家金币
        payResult();
        //通知结算
        sendChangeEnd(player, doubleNum, finalScore);
        ////////////////////////////////////////////////////////////////////////
    }

    //重置游戏
    reset();
    //清理座位信息
    setSeat(0, player->m_seatid);

    hlddz.game->change_table(player);
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
        xt_log.error("%s:%d, no empty seat.\n", __FILE__, __LINE__); 
        return false; 
    }

    player->m_seatid = seatid;
    player->m_tid = m_tid;
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
        jval["vlevel"]  = pl->m_vlevel;
        jval["sex"]     = pl->m_sex;
        jval["avatar"]  = pl->m_avatar;
        jval["state"]   = m_opState[pl->m_seatid];
        packet.val["userinfo"].append(jval);
    }

    vector_to_json_array(m_seatCard[player->m_seatid].m_cards, packet, "card");

    packet.end();
    unicast(player, packet.tostring());
}

void Table::loginBC(Player* player)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_LOGIN;
    packet.val["uid"]       = player->m_uid;
    packet.val["seatid"]    = player->m_seatid;
    packet.val["name"]      = player->m_name;
    packet.val["money"]     = player->m_money;
    packet.val["vlevel"]    = player->m_vlevel;
    packet.val["sex"]       = player->m_sex;
    packet.val["avatar"]    = player->m_avatar;
    packet.val["state"]     = m_state;

    packet.end();
    broadcast(player, packet.tostring());
}

bool Table::allocateCard(void)
{
    //底牌    
    if(!m_deck.getHoleCards(m_bottomCard, BOTTON_CARD_NUM))
    {
        xt_log.error("%s:%d, get bottom card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
        return false;
    }

    xt_log.debug("allocateCard, bottonCard:\n");
    show(m_bottomCard);

    //手牌
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(!m_deck.getHoleCards(m_seatCard[i].m_cards, HAND_CARD_NUM))
        {
            xt_log.error("%s:%d, get hand card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
            return false;
        }
        xt_log.debug("uid:%d\n", getSeat(i));
        show(m_seatCard[i].m_cards);
    }

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
    m_time = CALLTIME;
    //ev_timer_again(hlddz.loop, &m_timerCall);
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::doubleProc(void)
{
    m_state = STATE_DOUBLE; 
    setAllSeatOp(OP_DOUBLE_NOTIFY);
    m_time = DOUBLETIME;
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::outProc(void)
{
    m_state = STATE_OUT;
    setAllSeatOp(OP_OUT_WAIT);
    m_curSeat = m_lordSeat;
    m_preSeat = m_curSeat;
    m_time = OUTTIME;
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
    }

    if(m_players.empty())
    {
        reset(); 
        //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
    }

    bool findHuman = false;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        if(!it->second->isRobot()) 
        {
            findHuman = true;
            break;
        }
    }

    //通知机器人重新准备
    if(!findHuman)
    {
        reset();
        xt_log.debug("state: %s\n", DESC_STATE[m_state]);
        for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
        {
            Jpacket packet;
            packet.val["cmd"]           = SERVER_REPREPARE;
            packet.val["uid"]           = it->first;
            packet.end();
            unicast(it->second, packet.tostring());
        }
    }

    //清理座位信息
    setSeat(0, player->m_seatid);
}

void Table::endProc(void)
{
    m_state = STATE_END;
    setAllSeatOp(OP_GAME_END);
    xt_log.debug("state: %s\n", DESC_STATE[m_state]);

    //计算各座位输赢
    ////////////////////////////////////////////////////////////////////////
    int doubleNum = max(getAllDouble(), 1);
    int score = ROOMSCORE * doubleNum;
    //获取最小本钱
    int minMoney = getMinMoney(); 
    //最终数值
    int finalScore = min(score, minMoney);
    //xt_log.debug("endProc, befor score:%d, min money:%d, finalScore:%d\n", score, minMoney, finalScore);
    //计算各座位输赢
    calculate(finalScore);
    //修改玩家金币
    payResult();
    //通知结算
    sendEnd(doubleNum, finalScore);
    ////////////////////////////////////////////////////////////////////////

    //重置游戏
    reset();
    xt_log.debug("state: %s\n", DESC_STATE[m_state]);

    //检查入场费, 踢出不够的
    kick();
}

void Table::sendCard1(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_CARD_1;
        vector_to_json_array(m_seatCard[pl->m_seatid].m_cards, packet, "card");
        packet.val["time"]          = CALLTIME;
        packet.val["show_time"]     = SHOWTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
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
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["pre_id"]        = getSeat(m_preSeat);
        packet.val["score"]         = m_callScore[m_preSeat];
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
        packet.val["time"]          = OUTTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["count"]         = getCount();
        packet.end();
        unicast(pl, packet.tostring());
        //xt_log.debug("sendDoubleResult: cmd:%d, cur_id:%d, count:%d\n", SERVER_RESULT_DOUBLE, getSeat(m_curSeat), getCount());
    }
}

void Table::sendOutAgain(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_AGAIN_OUT;
        packet.val["time"]          = OUTTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["pre_id"]        = getSeat(m_preSeat);
        packet.val["out_id"]        = getSeat(m_outSeat);
        packet.val["keep"]          = (m_preSeat != m_outSeat);
        packet.val["num"]           = static_cast<int>(m_seatCard[m_curSeat].m_cards.size());
        vector_to_json_array(m_lastCard, packet, "card");
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendEnd(int doubleNum, int score)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_END;
    packet.val["code"]      = CODE_SUCCESS;
    packet.val["double"]    = doubleNum;
    packet.val["bomb"]      = getBombNum();
    packet.val["score"]     = score;

    for(map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        Json::Value jval;          
        Player* pl = it->second;
        jval["uid"]     = pl->m_uid;
        jval["name"]    = pl->m_name;
        jval["money"]   = m_money[pl->m_seatid];
        jval["isLord"]  = (pl->m_seatid == m_lordSeat);
        packet.val["info"].append(jval);
    }

    packet.end();
    broadcast(NULL, packet.tostring());
}

void Table::sendChangeEnd(Player* player, int doubleNum, int score)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_CHANGE_END;
    packet.val["code"]      = CODE_SUCCESS;
    packet.val["double"]    = doubleNum;
    packet.val["bomb"]      = getBombNum();
    packet.val["score"]     = score;

    for(map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        Json::Value jval;          
        Player* pl = it->second;
        jval["uid"]     = pl->m_uid;
        jval["name"]    = pl->m_name;
        jval["money"]   = m_money[pl->m_seatid];
        jval["isLord"]  = (pl->m_seatid == m_lordSeat);
        packet.val["info"].append(jval);
    }

    packet.end();
    broadcast(player, packet.tostring());
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
        
void Table::sendError(Player* player, int msgid, int errcode)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_RESPOND;
    packet.val["code"]      = errcode;
    packet.val["msgid"]     = msgid;
    packet.end();
    unicast(player, packet.tostring());
}

void Table::gameStart(void)
{
    payTax();

    m_curSeat = rand() % SEAT_NUM;

    //for test
    //m_curSeat = 2;

    callProc();

    allocateCard();
    sendCard1();

    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_again(hlddz.loop, &m_timerUpdate);
    xt_log.debug("=======================================start send card, cur_id:%d, seateid:%d\n", getSeat(m_curSeat), m_curSeat);
    //ev_timer_again(hlddz.loop, &m_timerCall);
}

void Table::gameRestart(void)
{
    //重置部分数据
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        m_callScore[i] = 0;
        m_famerDouble[i] = false;
        m_seatCard[i].reset();
        m_bomb[i] = 0;
        m_outNum[i] = 0;
        m_money[i] = 0;
    }
    m_bottomCard.clear();
    m_lastCard.clear();
    m_deck.fill();
    m_deck.shuffle(m_tid);
    m_curSeat = 0;
    m_preSeat = 0;
    m_lordSeat = 0;
    m_outSeat = 0;
    m_topCall = 0;
    m_win = 0;

    m_curSeat = rand() % SEAT_NUM;

    callProc();

    allocateCard();
    sendCard1();

    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_again(hlddz.loop, &m_timerUpdate);
    xt_log.debug("=======================================restart send card, cur_id:%d, seateid:%d\n", getSeat(m_curSeat), m_curSeat);
}

bool Table::getNext(void)
{
    int nextSeat = (m_curSeat + 1) % SEAT_NUM;
    switch(m_state)
    {
        case STATE_CALL:
            {
                if(m_opState[nextSeat] == OP_CALL_WAIT) 
                {
                    m_preSeat = m_curSeat;
                    m_curSeat = nextSeat;
                    m_opState[m_curSeat] = OP_CALL_NOTIFY;
                    //xt_log.debug("get next success, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
                    return true; 
                }
            }
            break;
        case STATE_OUT:
            {//校验出牌时间戳
                m_preSeat = m_curSeat;
                m_curSeat = nextSeat;
                //xt_log.debug("get next success, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
                return true;
            }
            break;
    }

    xt_log.debug("get next finish, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
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

bool Table::selecLord(void)
{
    //如果有3分直接地主, 
    if(m_callScore[m_curSeat] == 3)
    {
        m_topCall = m_callScore[m_curSeat];
        m_lordSeat = m_curSeat;
        //xt_log.debug("selectLord success, score:%d, seatid:%d, uid:%d\n", m_topCall, m_lordSeat, getSeat(m_lordSeat));
        return true;
    }

    //如果3人都已经叫过分，选择最高分地主
    bool isAll = true;
    unsigned int seatid = 0;
    int score = 0;
    for(unsigned int i = 0; i < SEAT_NUM; ++i) 
    {
        if(m_callScore[i] > score) 
        {
            score = m_callScore[i]; 
            seatid = i;
        }

        if(m_opState[i] != OP_CALL_RECEIVE)
        {
            isAll = false;
        }
    }
    //not all respond
    if(!isAll)
    {
        //xt_log.debug("selectLord fail, no all respond.\n");
        return false;
    }
    // no one give score
    if(score == 0)
    {
        //xt_log.debug("selectLord fail, no one give score.\n");
        return false;
    }

    m_topCall = score;
    m_lordSeat = seatid;

    //xt_log.debug("selectLord success, score:%d, seatid:%d, uid:%d\n", m_topCall, m_lordSeat, getSeat(m_lordSeat));
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

void Table::show(const vector<XtCard>& card)
{
    printStr.clear();
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        //xt_log.debug("%s\n", it->getCardDescription());
        printStr.append(it->getCardDescriptionString());
        printStr.append(" ");
    }
    xt_log.debug("%s\n", printStr.c_str());
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

    //叫分加倍
    int callDouble = 0;
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
        if(m_callScore[i] > callDouble)
        {
            callDouble = m_callScore[i];
        }
    }
    //xt_log.debug("double: callDouble:%d, bombDouble:%d, bottomDouble:%d, springDouble:%d, antiSpringDouble:%d\n", callDouble, bombDouble, bottomDouble, springDouble, antiSpringDouble);
    ret = callDouble + bombDouble + bottomDouble + springDouble + antiSpringDouble;
    return ret;
}

int Table::getBottomDouble(void)
{
    bool littleJoke = false;
    bool bigJoke = false;
    set<int> suitlist; 
    set<int> facelist; 
    bool isContinue = m_deck.isNContinue(m_bottomCard, 1);
    for(vector<XtCard>::const_iterator it = m_bottomCard.begin(); it != m_bottomCard.end(); ++it)
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
        tmpplayer->changeMoney(-ROOMTAX);
    }
}

void Table::calculate(int finalScore)
{
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        //地主赢
        if(m_win == m_lordSeat)    
        {
            if(i == m_lordSeat)
            {
                m_money[i] = finalScore; 
            }
            else
            {
                m_money[i] = -finalScore / 2; 
            }
            continue;
        }
        else
        {//农民赢
            if(i == m_lordSeat)
            {
                m_money[i] = -finalScore; 
            }
            else
            {
                m_money[i] = finalScore / 2; 
            }
            continue;
        }
    }
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
        if(pl->m_money < ROOMTAX)
        {
            Jpacket packet;
            packet.val["cmd"]           = SERVER_KICK;
            packet.val["uid"]           = pl->m_uid;
            packet.end();
            unicast(NULL, packet.tostring());
            xt_log.debug("%s:%d, kick player for not enough money, uid:%d, seatid:%d, money:%d, roomtax:%d\n",__FILE__, __LINE__, pl->m_uid, pl->m_seatid, pl->m_money, ROOMTAX); 
            m_delPlayer.push_back(pl);
            //不能这里删除，否则logout里有对m_players的删除操作,导致容器错误, 且要保证发送消息完毕
        }
    }

    ev_timer_again(hlddz.loop, &m_timerKick);
}
        
void Table::addRobotMoney(Player* player)
{
    if(!player->isRobot())
    {
        return;
    }

    int addval = ROOMTAX * (rand() % 9 + 1);
    xt_log.debug("%s:%d, addRobotMoney, uid:%d, money:%d \n",__FILE__, __LINE__, player->m_uid, addval); 
    player->changeMoney(addval);
}
