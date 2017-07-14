#ifndef _PARTY_GAME_H_
#define _PARTY_GAME_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ev++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <algorithm>

#include "jpacket.h"
#include "PartyMacros.h"
#include "PartyPlayer.h"

class PartyServer;
class historyData
{
    public:
        historyData(void)
            : aw_id(0), lightning(false), fresh(false)
        {}
        historyData(int _aw_id, bool _lightning, bool _fresh)
            : aw_id(_aw_id), lightning(_lightning), fresh(_fresh)
        {}
    public:
        int     aw_id;
        bool    lightning;
        bool    fresh; 
};

class PartyGame 
{
    public:
        static void onTimerStart(struct ev_loop* loop,struct ev_timer* w,int revents);
        void timerStart(void);
        static void onTimerEnd(struct ev_loop* loop,struct ev_timer* w,int revents);
        void timerEnd(void);
        static void onTimerUpdate(struct ev_loop* loop,struct ev_timer* w,int revents);
        void timerUpdate(void);
        static void onTimerPrepare(struct ev_loop* loop,struct ev_timer* w,int revents);
        void timerPrepare(void);

        PartyGame();
        ~PartyGame();

        int configGame(Json::Value& value);
        int start(PartyServer* server,struct ev_loop* loop);
        int shutDown();
        PartyPlayer* getPlayer(int uid);

        ///////////////////////////////////////////////////
        //msg
        void playerLogin(PartyPlayer* player);
        void playerLoginRsp(PartyPlayer* player);

        void playerBet(PartyPlayer* player, Jpacket& package);
        void playerBetRsp(int aw_pos, int total_bet, PartyPlayer* player, PARTY_RETURN ret_code);

        void playerCancle(PartyPlayer* player, Jpacket& package);
        void playerCancleRsp(PartyPlayer* player, PARTY_RETURN ret_code);

        void playerFollow(PartyPlayer* player, Jpacket& package);
        void playerFollowRsp(PartyPlayer* player, PARTY_RETURN ret_code);
        
        void playerLogout(PartyPlayer* player);
        void playerLogoutRsp(PartyPlayer* player);

        void playerAuto(PartyPlayer* player, Jpacket& package);
        void playerAutoRsp(PartyPlayer* player, PARTY_RETURN ret_code);

        void bcUpdate(void);
        void bcStart(void);
        void bcEnd(void);
        void bcPrepare(void);

    private:
        void startTimeup(void);
        void endTimeup(void);
        void prepareTimeup(void);
        void updateTimeup(void);

        //特殊奖
        bool isSpecial(void);
        double getSpecialProba(int aw_id, int mi_lv, double b) const;
        void specialAward(void);
        void testGetSpecialProba(void);
        void testSpecialAward(void);
        //闪电
        bool isLightning(void);
        void lightningAward(void);
        void testLightningProba(void);
        //单项奖
        void singleAward(void);
        double getSingleProba(int aw_id, int mi_lv, double b) const;
        void testGetSingleProba(void);
        void testSingleAward(void);
        //送灯
        bool isLamp(void);
        void lampAward(void);
        double getLampProba(void) const;
        double getLoseRate(void) const;
        double getLoseLimit(void) const;
        double getAwardLose(int aw_id) const;//某奖项赔付的总数
        void testGetLampProba(void);
        void testGetLoseRate(void);
        void testLampAward(void);
        //结算
        void finalMission(void);
        void testFinalMission(void);
        //增加下注玩家
        void addBetPlayer(PartyPlayer* player);

    public:
        //明池等级
        int getMingLevel(void) const;
    private:
        double getMingFactor(void) const;
        double getAnFactor(void) const;
        double MathLog(double x, double y) const;

    public:
        //获取奖项赔率
        double getProba(int aw_id, int mi_lv) const;
        //获取奖项权重
        double getWeight(int aw_id, int mi_lv) const;

    private:
        void reset(void);
    public:
        bool isSpecialAward(int awid) const;
        inline bool getLightning(void) const { return m_light; }
        inline const std::set<int>& getLamp(void) { return m_aw_set; }
        inline int getAwid(void) const { return m_aw_id; }
    private:
        //池子处理
        void procPool(void);
        void changeGameBet(PartyPlayer* player, int aw_id, int bet_nu);
        //显示
        void randomShow(void);
        int getAnimal(int cur);
        void testGetAnimal(void);

        void addHistory(void);
        void addTop(void);

        //结算转动动物
        void rollAnimal(void);

        //打包奖项位置
        void packAwardPosInfo(Jpacket& packet);
        //打包下注信息
        void packBetInfo(Jpacket& packet);
        //打包下注额度
        void packBetType(Jpacket& packet);
        //打包个人下注信息
        void packPersonBet(Jpacket& packet, PartyPlayer* player);
        //打包历史记录
        void packHistory(Jpacket& packet);
        //打包奖项赔率
        void packProba(Jpacket& packet, int level);
        //打包排行榜
        void packTop(Jpacket& packet);
        //打包灯 
        void packLamp(Jpacket& packet);
        //打包本次开的全部奖项
        void packAllAward(Jpacket& packet);
        Json::Value packAwardProba(int aw_id, int prob_aw);
        //打包机器人配置
        void packRobotConf(Jpacket& packet);
        //获取奖项位置
        int getAwardPos(int pai) const;
        //获取大盘位置
        int getBigPos(void);
        //获取动物类型
        int getAnimalType(int aw_id) const;
        //获取颜色类型
        int getColorType(int aw_id) const;
        //获取奖项信息
        int getAwardId(int animal, int color) const;
        //获取本次奖项类型
        int getAwardType(void) const;

        //自动下注
        void procPlayerAuto(void);
        //更新配置数据
        void updateConfig(void);
        //从cache reids取数据
        double getValue(const char* str, double defval) const; 
        char* getStringValue(const char* str, char* defval) const; 
        //单项奖赔率转为特殊奖赔率
        void changeToSpecial(Jpacket& packet);
        //保存开奖数据
        void saveResult(void);
        //保存每个玩家得失
        void savePlayerResult(void);
        //保存排行榜
        void saveTop(void);
        //获取奖项名字
        const char* getAwardName(int aw_id) const;
        //送灯格式化
        const char* getLampFormat(void) const;
        //下注信息格式化
        const char* getBetFormat(void) const;
        //排行榜格式化
        const char* getTopFormat(void);
        void split(const std::string& s, char c, std::vector<int>& v);
        void splitString(const std::string& s, char c, std::vector<std::string>& v);
        void split2(const std::string& s, char c1, char c2, std::vector< std::pair<int, int> >& v);
        //特殊奖初始化处理
        void procSpecialInit(void); 
        //开特殊奖的条件
        bool checkSpecialLimit(void);
        //是否干预结果
        bool isControl(void) const;
        //干预处理
        void procCtrl(void);
        //概率处理
        void procNormal(void);
        //玩家总下注
        double getTotalBet(void) const;

    private:
        std::map<int,PartyPlayer*> m_offlinePlayers; 	
        std::map<int,PartyPlayer*> m_loginPlayers; 
        std::map<int,PartyPlayer*> m_betPlayers; 
        PartyServer* m_server;

        struct ev_loop* m_evLoop;

        ev_timer m_timer_start;
        ev_timer m_timer_end;
        ev_timer m_timer_update;
        ev_timer m_timer_prepare;

        PARTY_STATE m_status;
        //明池+抽水
        double m_ming;
        double m_an;
        //抽水
        double m_tax;
        int m_timeCount;

        std::map<int, double> m_special;
        double m_spec_total;
        std::map<int, double> m_single;

        //客户端奖项排列 key =  pos, value = award_id
        std::map<int, int> m_show;
        //开奖记录
        std::deque<historyData> m_history;
        //排行榜
        std::set<int> m_top;
        std::vector<int> m_showTop;

        //大盘转动位置
        int m_pos;
        int m_aw_id;
        bool m_light;
        std::set<int> m_aw_set;
        //全部下注情况
        std::map<int, double> m_game_bet;
        //真人下注情况
        std::map<int, double> m_people_bet;
        //开奖计算时候的明池等级
        int m_openLevel;
        
        //单局下注上限
        int m_betLimit;
        //reids表名
        std::string m_tbname;
        //场馆id
        int m_vid;
        //机器人下注类型权重
        std::vector<int> m_btw; 
        //机器人下注范围
        std::vector<int> m_br; 
        //下注额度类型
        std::vector<int> m_bt; 

        //本轮系统赢的钱
        int m_winMoney;

        //一次性标志
        bool m_first;
        //总押注
        int m_totalIn;
        //总奖励
        int m_totalOut;
        //中奖人数
        int m_awardPeople;
        //比较系数
        double m_party_compare;
        //开奖间隔区间
        int m_party_range1;
        int m_party_range2;
        //间歇抽水值 
        double m_party_water;
        //检查间隔
        int m_party_check;
        //特殊奖时间戳
        time_t m_award_stamp; 
        //特殊奖检查时间戳
        time_t m_check_stamp; 
        //上次开特殊奖时候的暗池值
        double m_last_an;
        //干预区间
        int m_ctrl_1;
        int m_ctrl_2;
        //干预概率
        int m_ctrl_pro;
};

#endif /*_PARTY_GAME_H_*/
