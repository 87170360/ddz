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
#include <math.h>

#include <json/json.h>

#include <map>
#include <set>

#include "XtShuffleDeck.h"
#include "XtHoleCards.h"
#include "jpacket.h"

static const unsigned int SEAT_NUM          = 3;
static const unsigned int HAND_CARD_NUM     = 17;
static const unsigned int BOTTON_CARD_NUM   = 3;
//托管出牌定时
static const unsigned int ENTRUST_OUT_TIME  = 2;

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

        //定时器函数
        /////////////////////////////////////////////////////////////////////////////
        static void callCB(struct ev_loop *loop, struct ev_timer *w, int revents);
        void onCall(void);
        static void OutCB(struct ev_loop *loop, struct ev_timer *w, int revents);
        void onOut(void);
        static void kickCB(struct ev_loop *loop, struct ev_timer *w, int revents);
        void onKick(void);
        static void updateCB(struct ev_loop *loop, struct ev_timer *w, int revents);
        void onUpdate(void);
        static void entrustOutCB(struct ev_loop *loop, struct ev_timer *w, int revents);
        void onEntrustOut(void);
        /////////////////////////////////////////////////////////////////////////////

        //receive msg
        int login(Player* player);
        //断线重连
        bool reLogin(Player* player); 
        void msgPrepare(Player* player);
        void msgCall(Player* player);
        void msgOut(Player* player);
        void msgChange(Player* player);
        void msgView(Player* player);
        void msgEntrust(Player* player);
        void msgChat(Player* player);
        void msgMotion(Player* player);
        void msgIdle(Player* player);

        //坐下
        bool sitdown(Player* player);
        //分牌
        bool allocateCard(void);
        bool allocateCardControl(void);
        //准备处理
        void prepareProc(void);
        //叫分处理
        void callProc(void);
        //出牌处理
        void outProc(void);
        //player已经被释放，不能被托管，彻底结束游戏
        void logout(Player* player);
        //临时断开，进行托管
        void leave(Player* player);
        //结算处理
        void endProc(void);
        //托管处理
        void entrustProc(bool killtimer, int entrustSeat);

        //叫分逻辑
        void logicCall(void);
        //出牌逻辑
        void logicOut(Player* player, vector<XtCard>& curCard, bool keep);

        // send msg
        void loginUC(Player* player, int code, bool relogin);
        void loginBC(Player* player);
        //第一次发牌和叫分
        void sendCard1(void);
        //继续叫分
        void sendCallAgain(void); 
        //叫分结果和加倍
        void sendCallResult(void);
        //继续出牌
        void sendOutAgain(bool last);
        //结束
        void sendEnd(void);
        //定时器时间
        void sendTime(void);
        //准备
        void sendPrepare(Player* player);
        //发送错误反馈
        void sendError(Player* player, int msgid, int errcode);
        //发送托管玩家出牌
        void sendEntrustOut(Player* player, vector<XtCard>& curCard, bool keep);
        //发送托管玩家叫分
        void sendEntrustCall(Player* player, int score);
        //发送托管玩家加倍
        void sendEntrustDouble(Player* player, bool dou);
        //通知托管
        void sendEntrust(int uid, bool active);
        //发送退出
        void sendLogout(Player* player);
    
        //开始发牌
        void gameStart(void);
        //重新发牌
        void gameRestart(void);
        //获取下一个操作用户
        bool getNext(void);
        //获取用户
        Player* getSeatPlayer(unsigned int seatid);
        //设置座位状态
        void setAllSeatOp(int state);
        //座位状态查询
        bool allSeatFit(int state);
        //评选地主
        bool selecLord(void);
        //打印牌组
        void show(const vector<XtCard>& card);
        //打印游戏信息
        void showGame(void);
        //加倍完毕
        bool isDoubleFinish(void);
        //获取台面额度
        int getTableQuota(void);
        //获取游戏中倍数和结算框炸弹数
        int getGameDouble(void);
        //获取结算框倍数(作废)
        int getResultDoulbe(void);
        //获取农民加倍
        int getFamerDouble(void);
        //获取叫分加倍
        int getCallDouble(void);
        //获取叫分
        int getCallScore(void);
        //获取底牌加倍
        int getBottomDouble(void);
        //是否春天
        bool isSpring(void);
        //是否反春天
        bool isAntiSpring(void);
        //炸弹数量
        int getBombNum(void);
        //获取最小本钱
        int getMinMoney(void);

        //扣除玩家牌局的输赢
        void payResult(void);
        //扣除入场费
        void payTax(void);
        //统计比赛次数和胜场次数
        void total(void);
        //计算各座位输赢
        void calculate(void);
        //测试兑换券，设置得分
        void setScore(void);
        //检查入场费, 检查托管并踢出玩家
        void kick(void);
        //增加robot money
        void addRobotMoney(Player* player);
        //累计经验，升级，奖励
        void addPlayersExp(void);
        //胜局处理
        void winProc(void);

        //设置座位的玩家
        void setSeat(int uid, int seatid);
        //获取座位的玩家uid
        int getSeat(int seatid);
        //获取头衔
        string getTitle(int money) {return "title_test";}
        //金币转经验
        int money2exp(int money);
        //托管出牌处理
        void entrustOut(void);
        //手牌是否存在
        bool checkCard(unsigned int seatid, const vector<XtCard>& outcard);  
        //底牌给地主
        void addBottom2Lord(void);
        //显示手牌数量
        void showHoldcardNum(void);
        //从redis刷新配置
        void refreshConfig(void);
        //高倍广播
        void topCount(Player* player, int maxcount);
        //券广播
        void topCoupon(Player* player);

    private:
        void reset(void);

    public:
        int							m_tid;
        int             			m_vid;
        int                         m_state;
        std::map<int, Player*>		m_players;

    private:
        int                         m_count;                        //牌局计数
        int                         m_seats[SEAT_NUM];              //各座位玩家id
        int                         m_opState[SEAT_NUM];            //各座位操作状态            
        int                         m_callScore[SEAT_NUM];          //各座位叫分
        bool                        m_famerDouble[SEAT_NUM];        //农民加倍
        bool                        m_entrust[SEAT_NUM];            //托管状态 true false
        bool                        m_timeout[SEAT_NUM];            //出牌超时 true false
        int                         m_bomb[SEAT_NUM];               //各座位炸弹数量
        int                         m_outNum[SEAT_NUM];             //各座位出牌次数
        int                         m_money[SEAT_NUM];              //各座位输赢
        int                         m_coupon[SEAT_NUM];             //各座位兑换券
        XtHoleCards                 m_seatCard[SEAT_NUM];           //座位手牌
        unsigned int                m_curSeat;                      //当前操作者座位
        unsigned int                m_preSeat;                      //上个操作者座位
        unsigned int                m_lordSeat;                     //地主座位
        unsigned int                m_outSeat;                      //当前出牌者座位
        int                         m_topCall;                      //最高叫分
        unsigned int                m_win;                          //胜利座位
        int                         m_time;                         //剩余倒计时秒

        XtShuffleDeck               m_deck;
        std::vector<XtCard>         m_bottomCard;                   //底牌
        std::vector<XtCard>         m_lastCard;                     //上一轮牌

        ev_timer                    m_timerCall;                    //叫分
        ev_timer                    m_timerOut;                     //出牌
        ev_timer                    m_timerKick;                    //踢人，要保证最后一个消息发送后才断开连接，所以要延时
        ev_timer                    m_timerUpdate;                  //更新倒计时
        ev_timer                    m_timerEntrustOut;              //托管出牌定时器

        vector<Player*>             m_delPlayer;
};

#endif
