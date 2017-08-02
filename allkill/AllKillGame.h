#ifndef _ALL_KILL_GAME_H_
#define _ALL_KILL_GAME_H_

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
#include <queue>

#include "jpacket.h"

//#include "XtShuffleDeck.h"
#include "XtTypeDeck.h"
#include "XtHoleCards.h"

#include "AllKillMacros.h"

#include "AllKillPlayer.h"

class AllKillServer;

enum BetType
{
    BetType_1           = 1, //A > B C D
    BetType_2           = 2, //A = B = C = D
    BetType_3           = 3, //A B > C D
    BetType_4           = 4, //other
};


enum ResultType
{
    ResultType_0w		= 0,
    ResultType_1w		= 1,
    ResultType_2w		= 2,
    ResultType_3w		= 3,
    ResultType_4w		= 4,
};

enum SysResult
{
    SysResult_lose      = -1,
    SysResult_random    = 0,
    SysResult_win       = 1,
};

class DataBetPos
{
    public:
        DataBetPos(void)
        {
            m_pos = 0;		
            m_bet = 0;		
        }
    public:
        int m_pos;
        int m_bet;
};

class GameResultInfo
{
    public:
        GameResultInfo(int seat_id0,int seat_id1,int seat_id2,int seat_id3)
        {
            m_seatid[0]=seat_id0;
            m_seatid[1]=seat_id1;
            m_seatid[2]=seat_id2;
            m_seatid[3]=seat_id3;
        }

    public:
        int m_seatid[AK_SEAT_ID_NU];
};


class AllKillRottleFirstReward
{
    public:
        AllKillRottleFirstReward()
        {
            m_uid=0;
            m_sex=1;
            m_rewardMoney=0;
        }


    public:
        void setIfMostReward(int money,AllKillPlayer* player)
        {
            if(money>m_rewardMoney)
            {
                m_uid=player->getUid();
                m_rewardMoney=money;
                m_avatar=player->getAvatar();
                m_name=player->getName();
                m_sex=player->getSex();
            }
        }

    public:
        int m_uid;
        std::string m_avatar;
        std::string m_name;
        int m_sex;
        int m_rewardMoney;
};


class AllKillRottleResult
{
    public:
        AllKillRottleResult()
        {
            reset();
        }

    public:
        void reset()
        {
            m_hasOpenRottle=false;
            m_roleRottleMoney=0;
            for(int i=0;i<AK_SEAT_ID_NU;i++)
            {
                m_seatRottleMoney[i]=0;
            }
        }

    public:
        bool m_hasOpenRottle;

        int m_roleRottleMoney;
        int m_seatRottleMoney[AK_SEAT_ID_NU];
};



class AllKillGameSeatResult 
{
    public:
        AllKillGameSeatResult()
        {
            reset();
        }

    public:
        int m_cardType;
        int m_isWin;
        int m_betTime;
        int m_totalBetNu;
        std::vector<AllKillPlayer*> m_betPlayerList;

    public:
        void reset()
        {
            m_cardType=0;
            m_isWin=0;
            m_betTime=0;
            m_totalBetNu=0;
            m_betPlayerList.clear();
        }

        int getHumanBet(int seatid)
        {
            int totalBet = 0;
            for(std::vector<AllKillPlayer*>::iterator it = m_betPlayerList.begin(); it != m_betPlayerList.end(); ++it) 
            {
                if((*it)->getUid() > VALID_CLIENT_UID_MIN)   
                {
                    totalBet += (*it)->getSeatBet(seatid + AK_SEAT_ID_START); 
                }
            }
            return totalBet;
        }

        int getHumanSize(void)
        {
            int num = 0;
            for(std::vector<AllKillPlayer*>::iterator it = m_betPlayerList.begin(); it != m_betPlayerList.end(); ++it) 
            {
                if((*it)->getUid() > VALID_CLIENT_UID_MIN)   
                {
                    num++;
                }
            }
            return num;
        }
};


class AllKillGameRoleResult 
{
    public:
        AllKillGameRoleResult()
        {
            reset();
        }

    public:
        void reset()
        {
            m_cardType=0;
            m_betTime=0;
            m_roleLostMoney=0;
            m_roleRealWinMoney=0;
            m_roleWinMoney=0;
            m_roleResultBetMoney=0;
            m_humanResult = 0;
        }

    public:
        int m_cardType;
        int m_betTime;

        int m_roleLostMoney;
        int m_roleRealWinMoney;
        int m_roleWinMoney;
        int m_roleResultBetMoney;

        //庄家对真人的结果
        int m_humanResult;
};




class AllKillGameBetting
{
    public:
        AllKillGameBetting()
        {
            //int weight[6]={(int)(52*1.5), (int)(48*1.5), (int)(1096*1.5), (int)(720*1.5), (int)(3744*1.5), (int)(16440/2)};
            int weight[6]={(int)(8), (int)(6.5), (int)(8.5), (int)(720*1.5), (int)(3744*1.5), (int)(16440/2)};
            m_deck.setTypeWeight(weight);
            reset();
        }

    public:
        void reset()
        {
            m_deck.fill();
            m_deck.shuffle(0);
            vector<int> forbitface;
            forbitface.push_back(2);
            forbitface.push_back(3);
            forbitface.push_back(4);
            forbitface.push_back(5);
            forbitface.push_back(6);
            m_deck.forbitFace(forbitface);
            for(int i=0;i<AK_SEAT_ID_NU;i++)
            {
                m_seatBet[i]=0;
                m_playerSeatBet[i]=0;
            }
            m_betRemainTime=0;
            m_endGameTime=0;
        }

        void deal(bool sys_role,int radio);
        void dealEx(SysResult type);
        void deal3(SysResult type);

        //private:
    public:
        void createHolecards(bool sysWin);
        ResultType getResultType(bool sysWin) const;
        void create0w(bool sysWin);
        void create1w(bool sysWin);
        void create2w(bool sysWin);
        void create3w(bool sysWin);
        void create4w(bool sysWin);

        void func1(void);
        void func2(void);
        void func3(void);
        void func4(void);
        void func5(void);
        void func6(void);
        void func7(void);
        void func8(void);
        void func9(void);
        void func10(void);
        void func11(void);
        void func12(void);
        void funcRand(void);
        BetType getBetType(void) const;
        ResultType getResultTypeWin4(void) const;
        ResultType getResultTypeWin3(void) const;
        ResultType getResultTypeWin2(void) const;
        ResultType getResultTypeLost4(void) const;
        ResultType getResultTypeLost3(void) const;
        ResultType getResultTypeLost2(void) const;
        bool procCard(ResultType rt, BetType bt, bool SysWin);
        bool isBetZero(void) const;
        int totalBet(void) const;

        void addPlayerSeatBet(int seat_id, int benu);

    public:
        //XtShuffleDeck m_deck;
        XtTypeDeck m_deck;
        int m_seatBet[AK_SEAT_ID_NU];
        XtHoleCards m_seatCard[AK_SEAT_ID_NU];
        XtHoleCards m_roleCard;

        int m_betRemainTime;
        int m_endGameTime;
    private:
        int m_playerSeatBet[AK_SEAT_ID_NU];
};

class AllKillGameConfig
{
    public:
        int m_baseMoney;
        int m_askRoleMinMoney;
        int m_roleMinMoney;

        /* role another radio */
        int m_roleAnotherCard;


        /* sys role info */
        std::string m_sysRoleName;
        std::string m_sysRoleAvatar;
        int m_sysRoleUid;
        int m_sysRoleMoney;
        int m_sysRoleSex;

        /* sys chat role info */
        std::string m_chatRoleName;
        std::string m_chatRoleAvatar;	
        int  m_chatRoleUid;
        int m_chatRoleSex;

        /* enter room chat */
        std::string m_enterRoomChat;

        /*rottle fee */
        float m_winRottleFee;

        /* sys fee */
        float m_sysFee;


        /* rottle radio */
        float m_rottleBaoZhiRadio;
        float m_rottleShunJinRadio;
        float m_rottleJinHuaRadio;

        /* rottle min money */
        int m_rottleMinOpenMoney;

        /* game result max history */
        int m_maxGameResultHistory;

        int m_vid;

		int m_deskMinMoney;
};

//values: m_init_value m_change_value m_proba m_win_act_lower m_win_act_upper m_lose_act_lower m_lose_act_upper
class WinControl
{
    public:
        WinControl(void) : 
            m_pServer(NULL)
    {}

        void            init(AllKillServer* pServer);
        void            updateToRedis(const std::string &name, double value);
        void            setValueToRedis(const std::string &name, double value);
        double          getValueFromRedis(const std::string &str) const;
        SysResult       getPlayResult(bool sysRole, bool betZero) const;
        void            resetValue(void);
    private:
        AllKillServer*  m_pServer;
};

class AllKillGame 
{
    public:
        static void onReadyTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
        static void onStartTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
        static void onEndGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
        static void onUpdateGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
        static void onChangeRoleTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
    public:
        AllKillGame();
        ~AllKillGame();

    public:
        int configGame(Json::Value& value);
        int start(AllKillServer* server,struct ev_loop* loop);
        int shutDown();
    public:
        AllKillPlayer* getPlayer(int uid);
        AllKillPlayer* getPlayerNoAdd(int uid);

        void playerLogin(AllKillPlayer* player);
        void playerBet(AllKillPlayer* player,Jpacket& package);
        void playerAskRole(AllKillPlayer* player,Jpacket& package);
        void playerUnRole(AllKillPlayer* player,Jpacket& package);
        void playerLogout(AllKillPlayer* player);
        void playerDesk(AllKillPlayer* player,Jpacket& package);

    public:
        void readyTimer();
        void startTimer();
        void endGameTimer();
        void updateGameTimer();
        void changeRoleTimer();

    public:
        void handleGameReady();
        void handleGameEnd();
        void handleGameUpdate();
        void handleGameStart();
        void handleChangeRole();

        void saveGameResultToSql();

    public:
        void formatGameResult(Jpacket* packet);
        void formatAskRoleList(Jpacket* packet);
        void formatRole(Jpacket* packet);
        void formatRottleFirstReward(Jpacket* packet);
        void formatDeskPlayer(Jpacket& packet);

    public:
        void sendLoginSuccess(AllKillPlayer* player);

        void sendGameInfo(AllKillPlayer* player);

        void broadcastGameReady(AllKillPlayer* player, , bool formatDeskInfo = false);
        void broadcastGameStart(AllKillPlayer* player);
        void broadcastGameEnd(AllKillPlayer* player);
        void broadcastGameUpdate(AllKillPlayer* player);
        void broadcastAskRoleChange(AllKillPlayer* player);

        void sendGameInfoToSpeaker();

        void sendBetError(AllKillPlayer* player,int code,const std::string& desc);
        void sendBetSuccess(AllKillPlayer* player,int seat_id);

		void CheckBroadcastDeskBetInfo(AllKillPlayer* player, int seat_id);

        void sendAskRoleSuccess(AllKillPlayer* player);
        void sendAskRoleError(AllKillPlayer* player,int code ,const std::string& desc);

        void sendUnRoleSuccess(AllKillPlayer* player);
        void sendUnRoleErr(AllKillPlayer* player,int code,const std::string& desc);

        void sendPlayerLogoutSuccess(AllKillPlayer* player);
        void sendPlayerChat(AllKillPlayer* player,const std::string& content);
        void sendBetPlayerResult();
    protected:
        void handlePlayerBetResult();
        void handleRottleResult();
        void handleMoneyResult();
		void handleDeskChange();

        int getCardTypeNu(int type);
        float getCardTypeRottleRadio(int type);
        void handleRottleResultByCardType(int card_type,int num);
        int setNextRole();

    private:
        void setCardForTest(void);
        bool isBetLimit(void);

    private:
        std::deque<GameResultInfo> m_gameResultInfo;
        std::map<int,AllKillPlayer*> m_offlinePlayers; 	
        std::map<int,AllKillPlayer*> m_loginPlayers; 
        std::map<int,AllKillPlayer*> m_betPlayers;
        std::vector<AllKillPlayer*> m_askRoleList;
        //上桌玩家列表
        int m_descPlayers[AK_DECKPLAYER_NU]; //value is uid, index is seatid
        AllKillPlayer* m_role;
        AllKillServer* m_server;

		bool m_bDeskPlayerChange;

        /* game status */
        int m_status;

        /* rottle  money */
        int m_rottleTotalMoney;

        /* game config */
        AllKillGameConfig m_gameConfig;

        /* game betting */
        AllKillGameBetting m_gameBetting;


        /*game result temp info */
        AllKillGameSeatResult m_seatResult[AK_SEAT_ID_NU];
        AllKillGameRoleResult m_roleResult;
        AllKillRottleResult m_rottleResult;
        AllKillRottleFirstReward m_rottleFirstReward;
        AllKillPlayer* m_mostRewardPlayer;

        /* timer */
        struct ev_loop* m_evLoop;
        ev_timer m_evReadyTimer;
        ev_timer m_evStartTimer;
        ev_timer m_evEndGameTimer;
        ev_timer m_evUpdateTimer;
        ev_timer m_evChangeRoleTimer;
        WinControl m_win_control;
};

#endif /*_ALL_KILL_GAME_H_*/


