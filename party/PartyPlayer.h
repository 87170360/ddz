#ifndef _PARTY_PLAYER_H_
#define _PARTY_PLAYER_H_

#include <string>
#include <map>
#include "PartyMacros.h"

class RedisClient;
class PartyGame;
class PartyServer;



class PartyPlayer 
{

#ifdef  AK_DEBUG
    public:
        static int ms_objectNu;
#endif 

    public:
        PartyPlayer();
        ~PartyPlayer();

    public:
        void setData(int uid,PartyServer* server,PartyGame* game);
        int getUid(){return m_uid;}

        int incMoney(int value);
        int decMoney(int value);

        int setTimeLock(int time);
        int updateMoney();
        int getMoney();
        inline const char* getName() { return m_name.c_str(); }
        inline const char* getAvatar() { return m_avatar.c_str(); } 
        inline int getSex() { return m_sex; }
        inline int getTotalReward(void) const { return m_totalReward; }
        inline int getVlevel(void) { return m_vlevel;}

        bool checkBanned(void);
        //结算
        void procReward(void);
        bool hitSpecial(int bet_aw);
        void reset(void);
        //下注
        int bet(int aw_id, int num);
        //取消下注
        void cancleBet(void); 
        //获取当前下注总金额
        int getCurrentTotalBet(void);
        //获取历史下注总金额
        int getHisoryTotalBet(void);
        //使用上次下注
        void useHistoryBet(void);
        //获取本轮净盈利
        int getPureReward(void);
        //累加当日总奖励
        void addTotalReward(void);
        //玩家每次登录进行初始化的处理
        void initPlayerLogin(void);
        //两个时间戳是否同一天
        bool isSameDay(long int stamp1, long int stamp2);
        //刷新当日奖励到redis
        void setTotalReward2Redis(void);

        const std::map<int, int>& getBet(void) const { return m_bet; }
        const std::map<int, int>& getHis(void) const { return m_his; }

    private:
        /* player info */
        int m_uid;
        std::string m_name;
        std::string m_avatar;
        int m_money;
        int m_sex;
        int m_vlevel;

        RedisClient* m_datarc;
        PartyGame* m_game;
        PartyServer* m_server;

    private:
        //bet info
        std::map<int, int> m_bet;   //key: aw_id, value: num
        std::map<int, int> m_his;   //key: aw_id, value: num
    public:
        //当日总奖励
        int m_totalReward;
        //更新总奖项时间戳
        long int m_rewardStamp;

        //reward
        //单项和特殊奖
        int m_reward;
        int m_rewardLam;
        int m_rewardLig;
        //自动续押
        bool m_auto;
        //重复跟注
        bool m_repeatFollow;
};

#endif /*_PARTY_PLAYER_H_*/

