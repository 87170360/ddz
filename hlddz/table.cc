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

Table::Table() : m_count(0)
{
    //xt_log.debug("%s%d CALLTIME:%d\n", __FILE__, __LINE__, hlddz.game->CALLTIME);
    m_timerCall.data = this;
    ev_timer_init(&m_timerCall, Table::callCB, ev_tstamp(hlddz.game->CALLTIME), ev_tstamp(hlddz.game->CALLTIME));

    m_timerOut.data = this;
    ev_timer_init(&m_timerOut, Table::OutCB, ev_tstamp(hlddz.game->OUTTIME), ev_tstamp(hlddz.game->OUTTIME));

    m_timerKick.data = this;
    ev_timer_init(&m_timerKick, Table::kickCB, ev_tstamp(hlddz.game->KICKTIME), ev_tstamp(hlddz.game->KICKTIME));

    m_timerUpdate.data = this;
    ev_timer_init(&m_timerUpdate, Table::updateCB, ev_tstamp(hlddz.game->UPDATETIME), ev_tstamp(hlddz.game->UPDATETIME));

    m_timerEntrustOut.data = this;
    ev_timer_init(&m_timerEntrustOut, Table::entrustOutCB, ev_tstamp(ENTRUST_OUT_TIME), ev_tstamp(ENTRUST_OUT_TIME));
}

Table::~Table()
{
    ev_timer_stop(hlddz.loop, &m_timerCall);
    ev_timer_stop(hlddz.loop, &m_timerOut);
    ev_timer_stop(hlddz.loop, &m_timerKick);
    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_stop(hlddz.loop, &m_timerEntrustOut);
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
        m_callScore[i] = 0;
        m_famerDouble[i] = false;
        m_seatCard[i].reset();
        m_bomb[i] = 0;
        m_outNum[i] = 0;
        m_money[i] = 0;
        m_coupon[i] = 0;
        m_record[i] = false;
        m_entrust[i] = false;
        m_timeout[i] = false;
        m_opState[i] = OP_PREPARE_WAIT; 
    }
    m_bottomCard.clear();
    m_lastCard.clear();
    m_players.clear();
    m_deck.fill();
    m_deck.shuffle(m_tid);
    m_curSeat = 0;
    m_preSeat = 0;
    m_lordSeat = 0;
    m_outSeat = 0;
    m_topCall = 0;
    m_win = 0;
    m_time = 0;
    m_state = STATE_PREPARE; 

    ev_timer_stop(hlddz.loop, &m_timerCall);
    ev_timer_stop(hlddz.loop, &m_timerOut);
    ev_timer_stop(hlddz.loop, &m_timerKick);
    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_stop(hlddz.loop, &m_timerEntrustOut);
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
    //xt_log.debug("stop m_timerCall for timerup.\n");
    table->onCall();
}

void Table::onCall(void)
{
    //xt_log.debug("onCall, curseat:%d, score:%d\n", m_curSeat, 0);
    m_callScore[m_curSeat] = 0;
    //记录状态
    m_opState[m_curSeat] = OP_CALL_RECEIVE;
    logicCall();
}

void Table::OutCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerOut);
    //xt_log.debug("stop m_timerOut for timerup.\n");
    table->onOut();
}

void Table::onOut(void)
{
    //xt_log.debug("onOut. m_curSeat:%d\n", m_curSeat);
    Player* player = getSeatPlayer(m_curSeat);
    if(player == NULL)
    {
        xt_log.error("%s:%d, player null. seatid:%d\n", __FILE__, __LINE__, m_curSeat); 
        return;
    }
    bool keep = false;
    vector<XtCard> curCard;
    vector<XtCard> &myCard = m_seatCard[m_curSeat].m_cards;

    XtCard::sortByDescending(myCard);
    XtCard::sortByDescending(m_lastCard);

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
            m_deck.getOut(myCard, m_lastCard, curCard);
        }
        m_entrust[m_curSeat] = true;
        sendEntrust(player->m_uid, true);
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
            sendEntrust(player->m_uid, true);
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
    if(--m_time >= 0)
    {
        sendTime();
    }
}

void Table::entrustOutCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerEntrustOut);
    table->onEntrustOut();
}

void Table::onEntrustOut(void)
{
    entrustOut();
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
    if(player->m_money < hlddz.game->ROOMTAX)
    {
        xt_log.error("%s:%d, player was no enouth money! m_uid:%d, money:%d, roomtax:%d\n", __FILE__, __LINE__, player->m_uid, player->m_money, hlddz.game->ROOMTAX); 
        loginUC(player, CODE_MONEY, false);
        return 0; 
    }

    if(!sitdown(player))
    {
        return 0;
    }

    //登录回复
    loginUC(player, CODE_SUCCESS, false);

    //广播玩家信息
    loginBC(player);

    return 0;
}

bool Table::reLogin(Player* player) 
{
    xt_log.debug("player relogin m_uid:%d\n", player->m_uid);
    if(player->m_table_count != m_count)
    {
        xt_log.error("%s:%d, not in same table game m_uid:%d, table_count:%d, count:%d\n", __FILE__, __LINE__, player->m_uid, player->m_table_count, m_count); 
        return false;
    }

    if(m_players.find(player->m_uid) == m_players.end())
    {
        xt_log.error("%s:%d, player was not existed! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        return false;
    }

    if(player->m_seatid < 0 || player->m_seatid > SEAT_NUM)
    {
        xt_log.error("%s:%d, player seat error! uid:%d, seatid:%d\n", __FILE__, __LINE__, player->m_uid, player->m_seatid); 
        return false;
    }

    //给机器人加钱
    addRobotMoney(player);

    //检查入场费
    if(player->m_money < hlddz.game->ROOMTAX)
    {
        xt_log.error("%s:%d, player was no enouth money! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        sendError(player, CLIENT_LOGIN, CODE_MONEY);
        return false; 
    }

    //断线时候托管了，重连取消托管
    //m_entrust[player->m_seatid] = false;

    loginUC(player, CODE_SUCCESS, true);

    //记牌器
    if(m_record[player->m_seatid])
    {
        sendRecord(player);
    }

    return true;
}

void Table::msgPrepare(Player* player)
{
    //xt_log.debug("msg prepare m_uid:%d, seatid:%d, size:%d\n", player->m_uid, player->m_seatid, m_players.size());
    //检查入场费
    if(player->m_money < hlddz.game->ROOMTAX)
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

    sendPrepare(player);

    m_opState[player->m_seatid] = OP_PREPARE_REDAY; 
    if(!allSeatFit(OP_PREPARE_REDAY))
    {
        xt_log.debug("not all is prepare. tid:%d\n", m_tid);
        return;
    }
    else if(m_players.size() != SEAT_NUM)
    {
        xt_log.debug("not enouth player, size:%d, tid:%d\n", m_players.size(), m_tid);
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

    //座位通知过或者已经已经自动叫分
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

    //有效叫分
    Json::Value &msg = player->client->packet.tojson();
    int score = msg["score"].asInt();
    //xt_log.debug("msg Call m_uid:%d, score:%d, name:%s\n", player->m_uid, score, player->m_name.c_str());
    if(score < 0 || score > 3)
    {
        xt_log.error("%s:%d, call fail!, score error. uid:%d, score:%d\n", __FILE__, __LINE__, player->m_uid, score); 
        sendError(player, CLIENT_CALL, CODE_SCORE);
        return; 
    }

    //停止叫分定时器
    //xt_log.debug("stop m_timerCall for msg.\n");
    ev_timer_stop(hlddz.loop, &m_timerCall);

    //保存当前叫分
    m_callScore[m_curSeat] = score;
    //记录状态
    m_opState[m_curSeat] = OP_CALL_RECEIVE;
    //xt_log.debug("call score, m_uid:%d, seatid:%d, score :%d\n", player->m_uid, player->m_seatid, m_callScore[player->m_seatid]);
    logicCall();
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

    vector<XtCard> curCard;
    json_array_to_vector(curCard, player->client->packet, "card");

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

    //清除超时用户
    m_timeout[player->m_seatid] = false;

    //停止出牌定时器
    //xt_log.debug("stop m_timerOut for msg.\n");
    ev_timer_stop(hlddz.loop, &m_timerOut);

    logicOut(player, curCard, keep);
}

void Table::msgChange(Player* player)
{
    if(m_state != STATE_PREPARE)
    {
        sendError(player, CLIENT_CHANGE, CODE_STATE);
        return;
    }

    //xt_log.debug("msgChange, m_uid:%d, seatid:%d\n", player->m_uid, player->m_seatid);
    map<int, Player*>::iterator it = m_players.find(player->m_uid);
    if(it != m_players.end())
    {
        m_players.erase(it);
        //清理座位信息
        setSeat(0, player->m_seatid);
        m_opState[player->m_seatid] = OP_PREPARE_WAIT; 
    }

    hlddz.game->change_table(player);
}

void Table::msgView(Player* player)
{
    Json::Value &msg = player->client->packet.tojson();
    int uid = msg["uid"].asInt();
    int index = uid % hlddz.main_size;

    if (hlddz.main_rc[index]->command("hgetall hu:%d", uid) < 0) {
        xt_log.error("msg view fail! 1, get player infomation error. uid:%d\n", uid);
        sendError(player, CLIENT_VIEW, CODE_NOEXIST);
    }

    if (hlddz.main_rc[index]->is_array_return_ok() < 0) {
        xt_log.error("msg view fail! 2, get player infomation error. uid:%d\n", uid);
        sendError(player, CLIENT_VIEW, CODE_NOEXIST);
    }

    int money = hlddz.main_rc[index]->get_value_as_int("money");
    double total = hlddz.main_rc[index]->get_value_as_int("total");
    int victory = hlddz.main_rc[index]->get_value_as_int("victory");
    double victory_prob = (total > 0) ? (victory / total) : 0; 

    Jpacket packet;
    packet.val["cmd"]           = SERVER_RESPOND;
    packet.val["msgid"]         = CLIENT_VIEW;
    packet.val["uid"]           = uid;
    packet.val["code"]          = CODE_SUCCESS;
    packet.val["name"]          = hlddz.main_rc[index]->get_value_as_string("name");
    packet.val["avatar"]        = hlddz.main_rc[index]->get_value_as_string("avatar");
    packet.val["sex"]           = hlddz.main_rc[index]->get_value_as_int("sex");
    packet.val["money"]         = money;
    packet.val["level"]         = hlddz.main_rc[index]->get_value_as_int("level");
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
    if(player->m_money < hlddz.game->MOTIONMONEY || player->m_money < hlddz.game->ROOMLIMIT)
    {
        sendError(player, CLIENT_MOTION, CODE_MONEY);
        return;
    }

    player->changeMoney(-hlddz.game->MOTIONMONEY);

    Json::Value &msg = player->client->packet.tojson();
    Jpacket packet;
    packet.val["cmd"]         = SERVER_MOTION;
    packet.val["target_id"]   = msg["target_id"].asInt();
    packet.val["src_id"]      = player->m_uid;
    packet.val["type"]        = msg["type"].asInt();
    packet.val["price"]       = hlddz.game->MOTIONMONEY;
    packet.end();
    broadcast(NULL, packet.tostring());
}

void Table::msgIdle(Player* player)
{
    //xt_log.debug("%s,%d, msgIdle, uid:%d\n", __FILE__, __LINE__, player->m_uid);
}
        
void Table::msgRecord(Player* player)
{
    xt_log.debug("%s,%d, msgRecord, uid:%d\n", __FILE__, __LINE__, player->m_uid);


    //检查记牌器数量
    bool use = player->useRecored();
    
    m_record[player->m_seatid] = use ? true : false;

    Jpacket packet;
    packet.val["cmd"]       = SERVER_RESPOND;
    packet.val["msgid"]     = CLIENT_RECORD;
    packet.val["code"]      = use ? CODE_SUCCESS : CODE_RECORED;
    packet.end();
    unicast(player, packet.tostring());

    if(use)
    {
        sendRecord(player);
    }
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
    player->m_table_count = m_count;
    setSeat(player->m_uid, seatid);
    m_players[player->m_uid] = player;
    //xt_log.debug("sitdown uid:%d, seatid:%d\n", player->m_uid, seatid);
    return true;
}

void Table::loginUC(Player* player, int code, bool relogin)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_RESPOND;
    packet.val["code"]      = code;
    packet.val["msgid"]     = CLIENT_LOGIN;
    packet.val["tid"]       = m_tid;
    packet.val["seatid"]    = player->m_seatid;
    packet.val["relogin"]   = relogin;
    packet.val["entrust"]   = m_entrust[player->m_seatid];

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

    vector_to_json_array(m_seatCard[player->m_seatid].m_cards, packet, "card");

    //重登处理
    packet.val["state"]       = m_state;
    //当期操作者
    packet.val["cur_id"]      = getSeat(m_curSeat);

    switch(m_state)
    {
        case STATE_PREPARE:
            {
                for(unsigned int i = 0; i < SEAT_NUM; ++i)
                {
                    packet.val["prepare"].append(m_opState[i] == OP_PREPARE_REDAY); 
                }
            }
            break;
        case STATE_CALL:
            {
                //叫分
                for(unsigned int i = 0; i < SEAT_NUM; ++i)
                {
                    packet.val["callScore"].append(m_callScore[i]);
                }
                //剩余时间
                packet.val["time"] = m_time;
                //当前加倍
                packet.val["count"]         = getGameDouble(false);
                //当前叫分倍数
                packet.val["callcount"]     = getCallDouble();
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
                //当前加倍
                packet.val["count"]         = getGameDouble(false);
                //当前叫分倍数
                packet.val["callcount"]     = getCallDouble();
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
                //当前加倍
                packet.val["count"]         = getGameDouble(false);
                //当前叫分倍数
                packet.val["callcount"]     = getCallDouble();
            }
            break;
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

    //xt_log.debug("loginBC, uid:%d, name:%s, money:%d\n", pl->m_uid, pl->m_name.c_str(), pl->m_money);

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
    if(!m_deck.getHoleCards(m_bottomCard, BOTTON_CARD_NUM))
    {
        xt_log.error("%s:%d, get bottom card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
        return false;
    }

    //xt_log.debug("allocateCard, bottonCard:\n");
    //show(m_bottomCard);

    //手牌
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(!m_deck.getHoleCards(m_seatCard[i].m_cards, HAND_CARD_NUM))
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
    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x00));
    cards1.push_back(XtCard(0x10));
    m_deck.delCard(cards1, m_tid);
    m_bottomCard.assign(cards1.begin(), cards1.end());

    //底牌    
    if(!m_deck.getHoleCards(m_bottomCard, BOTTON_CARD_NUM - cards1.size()))
    {
        xt_log.error("%s:%d, get bottom card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
        return false;
    }

    //xt_log.debug("allocateCard, bottonCard:\n");
    //show(m_bottomCard);

    //手牌
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(!m_deck.getHoleCards(m_seatCard[i].m_cards, HAND_CARD_NUM))
        {
            xt_log.error("%s:%d, get hand card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
            return false;
        }
        //xt_log.debug("uid:%d\n", getSeat(i));
        //show(m_seatCard[i].m_cards);
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
    m_time = hlddz.game->CALLTIME;
    ev_timer_again(hlddz.loop, &m_timerCall);
    //xt_log.debug("m_timerCall first start \n");
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::outProc(void)
{
    m_state = STATE_OUT;
    setAllSeatOp(OP_OUT_WAIT);
    m_curSeat = m_lordSeat;
    m_preSeat = m_curSeat;
    m_time = hlddz.game->OUTTIME;
    ev_timer_again(hlddz.loop, &m_timerOut);
    payTax();
    //xt_log.debug("m_timerOut first start \n");
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::logout(Player* player)
{
    //xt_log.debug("player logout, uid:%d\n", player->m_uid);
    map<int, Player*>::iterator it = m_players.find(player->m_uid);
    if(it != m_players.end())
    {
        m_players.erase(it);
        //清理座位信息
        setSeat(0, player->m_seatid);
        m_opState[player->m_seatid] = OP_PREPARE_WAIT; 
    }
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
        hlddz.game->del_player(player);
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
    //test
    //setScore();
    //修改玩家金币
    payResult();
    //统计局数和胜场
    total();
    //要在sendEnd前面
    winProc();
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
                    ev_timer_stop(hlddz.loop, &m_timerCall);
                }
                onCall();
                sendEntrustCall(getSeatPlayer(entrustSeat), m_callScore[entrustSeat]); 
            }
            break;
        case STATE_OUT:
            {
                if(killtimer)
                {
                    ev_timer_stop(hlddz.loop, &m_timerOut);
                }
                ev_timer_again(hlddz.loop, &m_timerEntrustOut);
            }
            break;
    }
}

void Table::logicCall(void)
{
    //是否已经选出地主
    if(selecLord())
    {
        //底牌给地主
        addBottom2Lord();
        //出牌准备
        outProc();
        //开始出牌
        sendCallResult(); 
        //记牌器信息
        broadcastRecord();

        //地主托管自动处理
        if(m_entrust[m_curSeat])
        {
            entrustProc(true, m_curSeat);
        }
        //xt_log.debug("num: %d, %d, %d\n", m_seatCard[0].m_cards.size(), m_seatCard[1].m_cards.size(), m_seatCard[2].m_cards.size());
    }//设置下一个操作人
    else if(getNext())
    {
        //广播当前叫分和下一个叫分
        //xt_log.debug("m_timerCall again start.\n");
        m_time = hlddz.game->CALLTIME;
        sendCallAgain(); 
        if(m_entrust[m_curSeat])
        {
            entrustProc(false, m_curSeat);
        }
        else
        {
            ev_timer_again(hlddz.loop, &m_timerCall);
        }
    }
    else
    {//重新发牌
        xt_log.debug("nobody call, need send card again.\n");
        gameRestart();
    }
}

void Table::logicOut(Player* player, vector<XtCard>& curCard, bool keep)
{
    if(!curCard.empty())
    {
        //牌型校验
        XtCard::sortByDescending(curCard);
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
        if(!m_deck.compareCard(curCard, m_lastCard))
        {
            xt_log.error("%s:%d, compare fail.", __FILE__, __LINE__); 
            xt_log.error("curCard:\n");
            show(curCard);
            xt_log.error("lastCard:\n");
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
        //发送记牌器
        broadcastRecord();
    }


    //判定结束
    if(m_seatCard[player->m_seatid].m_cards.empty())
    {
        //发送最后一轮出牌
        getNext(); 
        sendOutAgain(true);    
        xt_log.debug("=======================================gameover, winseat:%d\n", player->m_seatid);
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

        m_time = hlddz.game->OUTTIME;
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
                ev_timer_set(&m_timerOut, ev_tstamp(hlddz.game->SECOND_OUTTIME), ev_tstamp(hlddz.game->SECOND_OUTTIME));
            }
            else
            {
                ev_timer_set(&m_timerOut, ev_tstamp(hlddz.game->OUTTIME), ev_tstamp(hlddz.game->OUTTIME));
            }
            //开启出牌定时器
            //xt_log.debug("m_timerOut again start \n");
            ev_timer_again(hlddz.loop, &m_timerOut);
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
        vector_to_json_array(m_seatCard[pl->m_seatid].m_cards, packet, "card");
        packet.val["time"]          = hlddz.game->CALLTIME;
        packet.val["show_time"]     = hlddz.game->SHOWTIME;
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
        packet.val["time"]          = hlddz.game->CALLTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["pre_id"]        = getSeat(m_preSeat);
        packet.val["score"]         = m_callScore[m_preSeat];
        packet.val["count"]         = getGameDouble(false);
        packet.val["callcount"]     = getCallDouble();
        packet.end();
        unicast(pl, packet.tostring());
    }
    //xt_log.debug("sendCallAgain: count:%d\n", getGameDouble(false));
}

void Table::sendCallResult(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_RESULT_CALL;
        packet.val["time"]          = hlddz.game->DOUBLETIME;
        packet.val["score"]         = m_topCall;
        packet.val["lord"]          = getSeat(m_lordSeat);
        packet.val["count"]         = getGameDouble(false);
        packet.val["callcount"]     = getCallDouble();
        vector_to_json_array(m_bottomCard, packet, "card");
        packet.end();
        unicast(pl, packet.tostring());
    }
    //xt_log.debug("sendCallResult: count:%d\n", getGameDouble(false));
}

void Table::sendOutAgain(bool last)
{
    //showHoldcardNum();
    //xt_log.debug("sendOutAgain, outseat:%d, num:%d\n", m_outSeat, m_seatCard[m_outSeat].m_cards.size());
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_AGAIN_OUT;
        packet.val["time"]          = hlddz.game->OUTTIME;
        packet.val["last"]          = last;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["pre_id"]        = getSeat(m_preSeat);
        packet.val["out_id"]        = getSeat(m_outSeat);
        packet.val["keep"]          = (m_preSeat != m_outSeat);
        packet.val["num"]           = static_cast<int>(m_seatCard[m_outSeat].m_cards.size());
        packet.val["count"]         = getGameDouble(false);
        vector_to_json_array(m_lastCard, packet, "card");
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendEnd(void)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_END;
    packet.val["code"]      = CODE_SUCCESS;
    packet.val["double"]    = getResultDoulbe();
    packet.val["bomb"]      = getGameDouble(true);
    packet.val["score"]     = hlddz.game->ROOMSCORE;
    packet.val["spring"]    = getSpringType();

    //xt_log.debug("end info: double:%d, bomb:%d, score:%d\n", doubleNum, getBombNum(), hlddz.game->ROOMSCORE);

    for(map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        Json::Value jval;          
        Player* pl = it->second;
        jval["uid"]          = pl->m_uid;
        jval["name"]         = pl->m_name;
        jval["money"]        = m_money[pl->m_seatid];
        jval["holdmoney"]    = pl->m_money;
        jval["coupon"]       = m_coupon[pl->m_seatid];
        jval["isLord"]       = (pl->m_seatid == m_lordSeat);
        jval["left_num"]     = static_cast<int>(m_seatCard[pl->m_seatid].m_cards.size());
        for (unsigned int i = 0; i < m_seatCard[pl->m_seatid].m_cards.size(); i++) 
        {
            jval["left_cards"].append(m_seatCard[pl->m_seatid].m_cards[i].m_value);
        }

        packet.val["info"].append(jval);
        xt_log.debug("end info: uid:%d, name:%s, money:%d, holdmoney:%d, coupon:%d\n", pl->m_uid, pl->m_name.c_str(), m_money[pl->m_seatid], pl->m_money, m_coupon[pl->m_seatid]);
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

void Table::sendEntrustOut(Player* player, vector<XtCard>& curCard, bool keep)
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

void Table::sendLogout(Player* player)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_LOGOUT;
    packet.val["uid"]       = player->m_uid;
    packet.end();
    broadcast(player, packet.tostring());
}

void Table::sendRecord(Player* player)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_RECORED;

    //初始
    map<int, int> cardinfo;
    for(int i = 3; i <= 17; ++i)
    {
        cardinfo[i] = 0; 
    }

    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(player->m_seatid == i)
        {
            continue;
        }

        vector<XtCard> &cards = m_seatCard[i].m_cards;
        for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
        {
            cardinfo[it->getCardFace()]++;
        }
    }

    for(map<int, int>::const_iterator it = cardinfo.begin(); it != cardinfo.end(); ++it)
    {
        packet.val["info"].append(it->second);
    }
    //showRecord(cardinfo, "record");
    packet.end();

    unicast(player, packet.tostring());
}
        
void Table::broadcastRecord(void)
{
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(m_record[i])
        {
            sendRecord(getSeatPlayer(i));
        }
    }
}

void Table::gameStart(void)
{
    m_curSeat = rand() % SEAT_NUM;
    xt_log.debug("=======================================start send card, cur_id:%d, next_id:%d, next_id:%d, tid:%d\n",
            getSeat(m_curSeat), getSeat((m_curSeat + 1) % SEAT_NUM), getSeat((m_curSeat + 2) % SEAT_NUM), m_tid);

    refreshConfig();
    callProc();
    allocateCard();
    //allocateCardControl();

    sendCard1();

    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_again(hlddz.loop, &m_timerUpdate);

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

    gameStart();
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

    //xt_log.debug("get next finish, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
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

    //从离线player中寻找
    map<int, Player*>::const_iterator ofit = hlddz.game->offline_players.find(uid);
    if(ofit != hlddz.game->offline_players.end())
    {
        return ofit->second; 
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

void Table::showRecord(const map<int, int>& card, char* desc)
{
    char buff[8];
    printStr.clear();
    for(map<int, int>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        sprintf(buff, "%d", it->first);
        printStr.append(buff);
        printStr.append(":");
        sprintf(buff, "%d", it->second);
        printStr.append(buff);
        printStr.append(" ");
    }
    xt_log.debug("%s %s\n", desc, printStr.c_str());
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

int Table::getTableQuota(void)
{
    //台面额度 = 底分 * 底牌加倍（没有为1） * 农民加倍（没有为1） * 叫分 * 2 ^（炸弹个数 + 春天或反春天1）
    //底分
    int score = hlddz.game->ROOMSCORE;
    //底牌加倍
    int bottomDouble = getBottomDouble();
    //农民加倍 范围1, 2, 4
    int famerDouble = getFamerDouble();
    //叫分 范围1, 2, 3
    int callScore = getCallScore();
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

    int ret = static_cast<int>(score * bottomDouble * famerDouble * callScore * tmp); 
    /*
    xt_log.debug("%s:%d, getTableQuota, score:%d, bottomDouble:%d, famerDouble:%d, callScore:%d, bombnum:%d, spring:%d, anti:%d, result:%d\n", __FILE__, __LINE__,
            score, bottomDouble, famerDouble, callScore, bombnum, spring, anti, ret); 
            */
    return ret;
}

int Table::getGameDouble(bool isEnd)
{
    //游戏中显示倍数（结算框炸弹） = 底牌加倍（没有为1） * 2 ^（炸弹个数 + 春天或反春天1）
    //底牌加倍
    int bottomDouble = getBottomDouble();
    //炸弹个数
    int bombnum = getBombNum();
    //春天
    int spring = (isSpring() && isEnd) ? 1 : 0;
    //反春天
    int anti = (isAntiSpring() && isEnd) ? 1 : 0;

    //2 ^（炸弹个数 + 春天或反春天1）
    double baseval = 2;
    double exponetval = bombnum + spring + anti;
    double tmp = pow(baseval, exponetval);

    int ret = static_cast<int>(bottomDouble * tmp); 
    /*
       xt_log.debug("%s:%d, getGameDouble, bottomDouble:%d, bombnum:%d, spring:%d, anti:%d, result:%d\n", __FILE__, __LINE__,
       bottomDouble, bombnum, spring, anti, ret); 
       */
    return ret;
}

int Table::getResultDoulbe(void)
{
    //结算框倍数 = 农民加倍（没有为1）* 叫分
    //农民加倍 范围1, 2, 4
    int famerDouble = getFamerDouble();
    //叫分 范围1, 2, 3
    int callScore = getCallScore();
    int ret = famerDouble * callScore;
    /*
       xt_log.debug("%s:%d, getResultDoulbe, famerDouble:%d, callScore:%d, result:%d\n", __FILE__, __LINE__,
       famerDouble, callScore, ret); 
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

int Table::getCallDouble(void)
{
    return getCallScore();
}

int Table::getCallScore(void)
{
    int ret = 1;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(m_callScore[i] > ret)
        {
            ret = m_callScore[i];
        }
    }

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
    return 1;
}

bool Table::isSpring(void)
{
    //农民没出过牌，且地主出牌(因为开始时候地主也没出牌)
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(i != m_lordSeat && m_outNum[i] != 0)
        {
            return false; 
        }
    }
    if(m_outNum[m_lordSeat] == 0)
    {
        return false;
    }
    return true;
}

bool Table::isAntiSpring(void)
{
    //地主只出过1次牌，农民出过牌（初始时候农民没出牌）
    if(m_outNum[m_lordSeat] != 1)
    {
        return false;
    }
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(i != m_lordSeat && m_outNum[i] <= 0)
        {
            return false; 
        }
    }
    return true;
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
    Jpacket packet;
    packet.val["cmd"]           = SERVER_TAX;
    packet.val["tax"]           = hlddz.game->ROOMTAX;

    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        //xt_log.debug("befor:%d\n", tmpplayer->m_money);
        tmpplayer->changeMoney(-hlddz.game->ROOMTAX);
        //xt_log.debug("after:%d\n", tmpplayer->m_money);

        Json::Value jval;          
        jval["uid"]     = it->first;
        jval["money"]   = tmpplayer->m_money;
        packet.val["info"].append(jval);
    }

    packet.end();
    broadcast(NULL, packet.tostring());
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

    int procType = -1;
    ///////////////////////////////////////////////////////////////////以小博大限制
    //地主赢 地主持有≥2*台面  小农＜台面&大农≥台面 
    if(m_win == m_lordSeat && (lordmoney >= 2 * score) && smallmoney < score && bigmoney >= score)
    {
        m_money[small->m_seatid] = -smallmoney;
        m_money[big->m_seatid] = -score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("%s%d\n", __FILE__, __LINE__);
        procType = 0;
    }
    //地主赢 地主持有≥2*台面 两个农民＜台面
    else if(m_win == m_lordSeat && (lordmoney >= 2 * score) && (smallmoney + bigmoney) < score)
    {
        m_money[small->m_seatid] = -smallmoney;
        m_money[big->m_seatid] = -bigmoney;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("%s%d\n", __FILE__, __LINE__);
        procType = 1;
    }
    //地主赢 地主持有＜2*台面
    else if(m_win == m_lordSeat && lordmoney < 2 * score)
    {
        m_money[small->m_seatid] = -min(smallmoney, min((lordmoney * smallmoney / (smallmoney + bigmoney)), score));
        m_money[big->m_seatid] = -min(bigmoney, min((lordmoney * bigmoney / (smallmoney + bigmoney)), score));
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("%s%d\n", __FILE__, __LINE__);
        procType = 2;
    }
    //农民赢 地主持有≥2*台面      小农＜台面&大农≥台面
    else if(m_win != m_lordSeat && (lordmoney >= 2 * score) && smallmoney < score && bigmoney >= score)
    {
        m_money[small->m_seatid] = smallmoney;
        m_money[big->m_seatid] = score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("%s%d\n", __FILE__, __LINE__);
        procType = 3;
    } 
    //农民赢 地主持有≥2*台面     两个农民＜台面 
    else if(m_win != m_lordSeat && (lordmoney >= 2 * score) && (smallmoney + bigmoney) < score)
    {
        m_money[small->m_seatid] = smallmoney;
        m_money[big->m_seatid] = score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("%s%d\n", __FILE__, __LINE__);
        procType = 4;
    }
    //农民赢 地主持有＜2*台面
    else if(m_win != m_lordSeat && (lordmoney < 2 * score))
    {
        m_money[small->m_seatid] = min(smallmoney, min((lordmoney * smallmoney / (smallmoney + bigmoney)), score));
        m_money[big->m_seatid] = min(bigmoney, min((lordmoney * bigmoney / (smallmoney + bigmoney)), score));
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("%s%d\n", __FILE__, __LINE__);
        procType = 5;
    }
    ///////////////////////////////////////////////////////////////////正常情况
    //地主赢 正常结算
    else if(m_win == m_lordSeat && (lordmoney >= 2 * score) && smallmoney >= score)
    {
        m_money[small->m_seatid] = -score;
        m_money[big->m_seatid] = -score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("%s%d\n", __FILE__, __LINE__);
        procType = 6;
    }
    //农民赢 正常结算
    else if(m_win != m_lordSeat && (lordmoney >= 2 * score) && smallmoney >= score)
    {
        m_money[small->m_seatid] = score;
        m_money[big->m_seatid] = score;
        m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
        //xt_log.debug("%s%d\n", __FILE__, __LINE__);
        procType = 7;
    }


    if(procType == -1 || (m_money[m_lordSeat] + lordmoney < 0) || (m_money[small->m_seatid] + smallmoney < 0) || (m_money[big->m_seatid] + bigmoney < 0))
    {
         xt_log.error("caculate error, lordname:%s, money:%f, big:%s, money:%f, small:%s, money:%f, score:%f\n"
            , lord->m_name.c_str(), lordmoney, big->m_name.c_str(), bigmoney, small->m_name.c_str(), smallmoney, score);
         xt_log.error("result : procType:%d, lordmoney::%d, bigmoney:%d, smallmoney:%d\n", procType, m_money[m_lordSeat], m_money[big->m_seatid], m_money[small->m_seatid]);
    }
}

void Table::setScore(void)
{
    Player* big = getSeatPlayer((m_lordSeat + 1) % 3);  
    Player* small = getSeatPlayer((m_lordSeat + 2) % 3);  

    m_money[small->m_seatid] = -5000;
    m_money[big->m_seatid] = -5000;
    m_money[m_lordSeat] = (-m_money[small->m_seatid]) + (-m_money[big->m_seatid]);
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
        if(pl->m_money < hlddz.game->ROOMTAX || m_entrust[pl->m_seatid])
        {
            Jpacket packet;
            packet.val["cmd"]           = SERVER_KICK;
            packet.val["uid"]           = pl->m_uid;
            packet.end();
            unicast(NULL, packet.tostring());
            xt_log.debug("%s:%d, kick player for not enough money, uid:%d, seatid:%d, money:%d, roomtax:%d\n",__FILE__, __LINE__, pl->m_uid, pl->m_seatid, pl->m_money, hlddz.game->ROOMTAX); 
            m_delPlayer.push_back(pl);
            //不能这里删除，否则logout里有对m_players的删除操作,导致容器错误, 且要保证发送消息完毕
        }
    }

    ev_timer_again(hlddz.loop, &m_timerKick);
}

void Table::addRobotMoney(Player* player)
{
    if(!player->isRobot() || player->m_money > hlddz.game->ROOMLIMIT)
    {
        return;
    }

    int addval = hlddz.game->ROOMLIMIT * (rand() % 9 + 1) + 100000;
    xt_log.debug("%s:%d, addRobotMoney, uid:%d, money:%d \n",__FILE__, __LINE__, player->m_uid, addval); 
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
    //最大倍数
    int maxcount = static_cast<int>(getGameDouble(true));

    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        player = it->second;
        if(m_money[player->m_seatid] > 0)
        {
            //历史记录
            player->updateTopMoney(m_money[player->m_seatid]); 
            //历史记录
            player->updateTopCount(maxcount);
            //兑换券
            m_coupon[player->m_seatid] = player->coupon(m_money[player->m_seatid]);
            //高倍广播
            if(m_money[player->m_seatid] > 0 && maxcount >= 64)
            {
                topCount(player, maxcount); 
            }
            //兑换券广播
            if(m_coupon[player->m_seatid] >= 3)
            {
                topCoupon(player);
            }
        }
    }
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
    if(player == NULL)
    {
        xt_log.error("%s:%d, player null. seatid:%d\n", __FILE__, __LINE__, m_curSeat); 
        return;
    }
    bool keep = false;
    vector<XtCard> curCard;
    vector<XtCard> &myCard = m_seatCard[m_curSeat].m_cards;

    XtCard::sortByDescending(myCard);
    XtCard::sortByDescending(m_lastCard);
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
        m_deck.getOut(myCard, m_lastCard, curCard);
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

bool Table::checkCard(unsigned int seatid, const vector<XtCard>& outcard)  
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
    const vector<XtCard>& holdcard = m_seatCard[seatid].m_cards;
    for(vector<XtCard>::const_iterator it = holdcard.begin(); it != holdcard.end(); ++it)
    {
        tmpset.insert(it->m_value);    
    }

    for(vector<XtCard>::const_iterator it = outcard.begin(); it != outcard.end(); ++it)
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
    for(vector<XtCard>::const_iterator it = m_bottomCard.begin(); it != m_bottomCard.end(); ++it)
    {
        m_seatCard[m_lordSeat].m_cards.push_back(*it);
    }
}

void Table::showHoldcardNum(void)
{
    std::map<int, Player*>::iterator it;
    for (it = m_players.begin(); it != m_players.end(); it++)
    {
        xt_log.debug("uid:%d, seatid:%d, num:%d\n", it->second->m_uid, it->second->m_seatid, m_seatCard[it->second->m_seatid].m_cards.size()); 
    }
}

void Table::refreshConfig(void)
{
    //最低携带
    int ret = 0;
    ret = hlddz.cache_rc->command("hget %s accessStart", hlddz.game->m_venuename.c_str());
    long long accessStart = 0;
    if(ret < 0 || false == hlddz.cache_rc->getSingleInt(accessStart))
    {
        xt_log.error("get accessStart fail. venuename:%s\n", hlddz.game->m_venuename.c_str());
    }
    else
    {
        hlddz.game->ROOMLIMIT = accessStart;
    }

    //房间底分
    ret = hlddz.cache_rc->command("hget %s startPoint", hlddz.game->m_venuename.c_str());
    long long startPoint = 0;
    if(ret < 0 || false == hlddz.cache_rc->getSingleInt(startPoint))
    {
        xt_log.error("get startPoint fail. venuename:%s\n", hlddz.game->m_venuename.c_str());
    }
    else
    {
        hlddz.game->ROOMSCORE = startPoint;
    }

    //台费
    ret = hlddz.cache_rc->command("hget %s accessFee", hlddz.game->m_venuename.c_str());
    long long accessFee = 0;
    if(ret < 0 || false == hlddz.cache_rc->getSingleInt(accessFee))
    {
        xt_log.error("get accessFee fail. venuename:%s\n", hlddz.game->m_venuename.c_str());
    }
    else
    {
        hlddz.game->ROOMTAX = accessFee;
    }

    //xt_log.debug("roomlimit:%d, roomscore:%d \n", hlddz.game->ROOMLIMIT, hlddz.game->ROOMSCORE);
}

void Table::topCount(Player* player, int maxcount)
{
    /*
       char buffer[50];
       sprintf(buffer, "%s在%s中力压群雄,打出了%d倍!真是厉害.", player->m_name.c_str(), hlddz.game->m_title.c_str(), maxcount);
       std::string content = buffer;

       Jpacket packet;
       packet.val["content"]       = content;
       packet.val["color"]         = "FF99FF";
       packet.val["name"]          = "系统";
       packet.val["vlevel"]        = 0;
       packet.val["sex"]           = 3;
       packet.end();

       xt_log.debug("topCount.%s\n", packet.tostring().c_str());

       int ret = hlddz.cache_rc->command("lpush broadcast1 %s", packet.tostring().c_str());
       */

    int ret = hlddz.cache_rc->command("lpush broadcast {\"content\":\"%s在%s中力压群雄,打出了%d倍!真是厉害.\",\"color\":\"#FF99FF\",\"name\":\"系统\",\"vlevel\":\"0\",\"sex\":\"3\"}",
            player->m_name.c_str(), hlddz.game->m_title.c_str(), maxcount);
    if(ret < 0)
    {
        xt_log.error("topCount fail. title:%s, name:%s, maxcount:%d\n", hlddz.game->m_title.c_str(), player->m_name.c_str(), maxcount);
    }
}

void Table::topCoupon(Player* player)
{
    int ret = hlddz.cache_rc->command("lpush broadcast {\"content\":\"天降好彩啦,%s打牌爆出%d兑换券,可以兑换奖励啦!\",\"color\":\"#FF99FF\",\"name\":\"系统\",\"vlevel\":\"0\",\"sex\":\"3\"}",
            player->m_name.c_str(), m_coupon[player->m_seatid]);
    if(ret < 0)
    {
        xt_log.error("topCoupon fail. name:%s, coupon:%d\n", player->m_name.c_str(), m_coupon[player->m_seatid]);
    }
}

int Table::getSpringType(void)
{
    if(isSpring())
    {
        return 1;
    }

    if(isAntiSpring())
    {
        return 2;
    }

    return 0;
}
