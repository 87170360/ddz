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

#include "bull_game_logic.h"
#include "global_define.h"

#include "AllKillMacros.h"

#include "AllKillPlayer.h"

class AllKillServer;

// 下注宝箱
class BetBox
{
public:
	BetBox()
	{
		boxType = -1;
		lotteryMoney = 0;
		getFlag = 0;
	}

public:
	int boxType;
	int lotteryMoney;
	int getFlag;
};

// 游戏输赢结果信息
class GameResultInfo
{
public:
	GameResultInfo(int seat_id0, int seat_id1, int seat_id2, int seat_id3)
	{
		m_seatid[0] = seat_id0;
		m_seatid[1] = seat_id1;
		m_seatid[2] = seat_id2;
		m_seatid[3] = seat_id3;
	}

public:
	int m_seatid[AK_SEAT_ID_NU];   // 4个座位数的赢输类型 （0输 1赢）
};

// 第一名获奖玩家
class LotteryFirstReward
{
public:
	LotteryFirstReward()
	{
		m_uid         = 0;
		m_sex         = 1;
		m_rewardMoney = 0;
	}


public:
    // 设置获奖最多的人
	void setIfMostLotteryReward(int money, int cardType, AllKillPlayer* player)
	{
		if (money > m_rewardMoney)
		{
			m_uid         = player->getUid();			
			m_avatar      = player->getAvatar();
			m_name        = player->getName();
			m_sex         = player->getSex();
			m_rewardMoney = money;
			m_rewardTime  = time(NULL);
			m_cardType    = cardType;
		}
	}

public:
	int m_uid;                 // uid 
	std::string m_avatar;      // 头像
	std::string m_name;        // 名称
	int m_sex;                 // 性别
	int m_rewardMoney;         // 赏金   
	int m_rewardTime;          // 获奖的时间
	int m_cardType;            // 获奖的牌型
};

// 开奖信息
class LotteryResult
{

public:
	LotteryResult()
	{
		reset();
	}

public:
	void reset()
	{
		m_hasOpenLottery=false;
		m_roleLotteryMoney=0;
		for (int i = 0; i < AK_SEAT_ID_NU; i++)
		{
			m_seatLotteryMoney[i]=0;
		}
	}

public:
	bool m_hasOpenLottery;                 // 是否开奖
	int m_roleLotteryMoney;                // 庄家获奖金额
	int m_seatLotteryMoney[AK_SEAT_ID_NU]; // 每个坐位的奖金
	int m_cardType;
};


// seat游戏结果
class SeatGameResult 
{
public:
	SeatGameResult()
	{
		reset();
	}

public:
	void reset()
	{
		m_cardType=CT_NOBULL;
		m_isWin=0;
		m_betTime=0;
		m_totalBetNu=0;
		m_betPlayerList.clear();
	}

public:
	CT_BULL_TYPE m_cardType;                      // 牌型
	int m_isWin;                                  // 0:输 1:赢
	int m_betTime;                                // 赢方牌型倍数
	int m_totalBetNu;                             // 下注数
	std::vector<AllKillPlayer*> m_betPlayerList;  // 该seat的玩家列表
};


// 庄家游戏结果
class RoleGameResult 
{
public:

	RoleGameResult()
	{
		reset();
	}

public:
	void reset()
	{
		m_cardType=CT_NOBULL;
		m_betTime=0;
		m_roleLostMoney=0;
		m_roleRealWinMoney=0;
		m_roleWinMoney=0;
		m_roleResultBetMoney=0;
	}

public:
	CT_BULL_TYPE m_cardType;  // 牌型
	int m_betTime;            // 赢方牌型倍数

	int m_roleLostMoney;      // 庄家输
	int m_roleRealWinMoney;   // 庄家实际赢
	int m_roleWinMoney;       // 庄家赢
	int m_roleResultBetMoney;  
};



// 游戏对局（封装位置的下注和牌，庄家的牌，游戏剩余时间）
class GameBetting
{

public:

	GameBetting()
	{
		m_GameLogic = CBullGameLogic::GetInstance();
		reset();
	}

public:
	//重置
	void reset()
	{
		//for(int i=0;i<AK_SEAT_ID_NU;i++)
		//{
		//	m_seatBet[i]=0;
		//}
		memset(m_seatBet, 0, sizeof(m_seatBet));

		m_betRemainTime=0;
		m_endGameTime=0;


		memset(m_seatCard, 0, sizeof(m_seatCard));
		memset(m_seatGroupCard, 0, sizeof(m_seatGroupCard));
		memset(m_seatCardType, 0, sizeof(m_seatCardType));

		memset(m_roleCard, 0, sizeof(m_roleCard));
		memset(m_roleGroupCard, 0, sizeof(m_roleGroupCard));

		m_roleCardType = CT_NOBULL;  // 没牛

	}

	// 发牌
	void deal(bool sys_role,int radio);


public:

	CBullGameLogic *m_GameLogic;

	int m_seatBet[AK_SEAT_ID_NU];                                //位置的下注数

	_uint8 m_seatCard[AK_SEAT_ID_NU][MAX_HAND_CARD_COUNT];       // 位置牌
	_uint8 m_seatGroupCard[AK_SEAT_ID_NU][MAX_HAND_CARD_COUNT];  // 位置组好的牌
	CT_BULL_TYPE m_seatCardType[AK_SEAT_ID_NU];                  // 位置牌型


	_uint8 m_roleCard[MAX_HAND_CARD_COUNT];                 // 庄家的牌
	_uint8 m_roleGroupCard[MAX_HAND_CARD_COUNT];            // 庄家组好的牌
	CT_BULL_TYPE m_roleCardType;                            // 庄家的牌型

	int m_betRemainTime;    // 下注剩余时间
	int m_endGameTime;      // 游戏结束时间
};




// 游戏配置信息
class GameConfig
{
public:
	int m_baseMoney;         // 基注（投注额以基注为单位）
	int m_askRoleMinMoney;   // 上庄底限
	int m_unRoleLimitMoney;  // 下庄限制（低于这个值，必须下庄）

	/* role another radio */
	int m_roleAnotherCard;    // **换牌概率


	/* sys role info */   
	std::string m_sysRoleName;     // 系统庄家 - 名称
	std::string m_sysRoleAvatar;   // 系统庄家 - 头像
	int m_sysRoleUid;              // 系统庄家 - uid     
	int m_sysRoleMoney;            // 系统庄家 - 金币 
	int m_sysRoleSex;              // 系统庄家 - 性别

	/* sys chat role info */ //系统聊天庄家信息
	std::string m_chatRoleName;
	std::string m_chatRoleAvatar;
	int  m_chatRoleUid;
	int m_chatRoleSex;

	/* enter room chat */ //进入房间的欢迎词
	std::string m_enterRoomChat;

	/*rottle fee */  //奖券抽水比例
	float m_winRottleFee;  

	/* sys fee */ //系统抽水比例
	float m_sysFee;


	/* rottle radio */  //奖金池开奖比例
	float m_rottleBombRadio;
	float m_rottleFiveHBullRadio;
	float m_rottleFiveSBullRadio;

	//入坐金币底限，vlevel等级底限
	int m_sitMinMoney;	
	int m_sitVLevel;

	/* rottle min money */ //开奖底限
	int m_rottleMinOpenMoney;

	/* game result max history */ //展示游戏结果历史最多条数
	int m_gameHistoryNum;

	int m_vid;

};


/////////*****************万人场******************////////////////

class AllKillGame 
{

public:
	AllKillGame();
	~AllKillGame();

public:
	// 读取万人场的配置
	int configGame(Json::Value& value);
	// 启动万人场
	int start(AllKillServer* server,struct ev_loop* loop);
	// 关闭万人场
	int shutDown();

public:
	static void onReadyTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
	static void onStartTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
	static void onEndGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
	static void onUpdateGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
	static void onChangeRoleTimer(struct ev_loop* loop,struct ev_timer* w,int revents);


public:
	AllKillPlayer* getPlayer(int uid);

	void playerLogin(AllKillPlayer* player);
	void playerBet(AllKillPlayer* player,Jpacket& package);      // 玩家下注
	void playerAskRole(AllKillPlayer* player,Jpacket& package);  // 上庄
	void playerUnRole(AllKillPlayer* player,Jpacket& package);   // 下庄
	void playerLogout(AllKillPlayer* player);

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
	// 组包对局玩家信息
	void formatBetPlayerResult(Jpacket* packet);

	void formatGameResult(Jpacket* packet);
	void formatAskRoleList(Jpacket* packet);
	void formatRole(Jpacket* packet);
	void formatLotteryFirstReward(Jpacket* packet);


public:
	void sendLoginSuccess(AllKillPlayer* player);
	void sendGameInfo(AllKillPlayer* player);


	void broadcastGameReady(AllKillPlayer* player);     // 广播游戏准备
	void broadcastGameStart(AllKillPlayer* player);     // 开始
	void broadcastGameEnd(AllKillPlayer* player);       // 结束
	void broadcastGameUpdate(AllKillPlayer* player);    // 更新
	void broadcastAskRoleChange(AllKillPlayer* player); // 改变角色

	//发布消息
	void sendGameInfoToSpeaker();


	void sendBetError(AllKillPlayer* player,int code,const std::string& desc);
	void sendBetSuccess(AllKillPlayer* player,int seat_id,int bet_nu);

	void sendAskRoleSuccess(AllKillPlayer* player);
	void sendAskRoleError(AllKillPlayer* player,int code ,const std::string& desc);

	void sendUnRoleSuccess(AllKillPlayer* player);
	void sendUnRoleErr(AllKillPlayer* player,int code,const std::string& desc);

	void sendPlayerLogoutSuccess(AllKillPlayer* player);
	void sendPlayerChat(AllKillPlayer* player,const std::string& content);
	void sendBetPlayerResult();


protected:
	void handlePlayerBetResult();
	void handleLotteryResult();
	void handleMoneyResult();

	// 此牌型个数
	int getCardTypeNu(int type);

	// 牌型奖池开奖比例
	float getCardTypeLotteryRadio(int type);

	void handleRottleResultByCardType(int card_type,int num);
	int setNextRole();

	int doSitDown(AllKillPlayer* player, int seat_id);
	int doStandUp(AllKillPlayer* player);

	void addLotteryFirstReward(LotteryFirstReward lotteryFirst);

public:
	// 坐下
	void playerSitDown(AllKillPlayer* player, int seat_id);

	// 站起
	void playerStandUp(AllKillPlayer* player,int seat_id);

	// 站起
	void playerStandUp(AllKillPlayer* player);


	void sendLotterFirstPlayers(AllKillPlayer* player);
	
private:

	//连续当庄次数
	int m_role_nu;

	//闲家坐位uin（坐位从0开始）
	int m_playerSeat[AK_SEAT_ID_NU];     


	//游戏结果结果列表
	std::deque<GameResultInfo> m_gameResultList;

	//离线玩家
	std::map<int,AllKillPlayer*> m_offlinePlayers; 	
	//登录玩家
	std::map<int,AllKillPlayer*> m_loginPlayers; 
	//下注玩家
	std::map<int,AllKillPlayer*> m_betPlayers;  
	//上庄玩家
	std::vector<AllKillPlayer*> m_askRoleList;  

	//当前庄家
	AllKillPlayer* m_role;         

	AllKillServer* m_server;

	/* game status*/
	int m_gameStatus;


	/* rottle  money */
	int m_lotteryTotalMoney;  //奖池金额


	/* game config */
	GameConfig m_gameConfig;


	/* game betting 对局信息*/
	GameBetting m_gameBetting;


	/*game result temp info */ 
	SeatGameResult m_seatGameResult[AK_SEAT_ID_NU];  // 坐位游戏结果
	RoleGameResult m_roleGameResult;                 // 庄家游戏结果

	// 开奖结果信息
	LotteryResult m_lotteryResult;         

	// 第一名获奖列表
	std::deque<LotteryFirstReward> m_lotterFirstRewardList;
	
	// 获奖第一名得主
	LotteryFirstReward m_lotteryFirstReward;

	// 赢钱最多玩家
	AllKillPlayer* m_mostRewardPlayer;



	/* timer */
	struct ev_loop* m_evLoop;
	ev_timer m_evReadyTimer;
	ev_timer m_evStartTimer;
	ev_timer m_evEndGameTimer;
	ev_timer m_evUpdateTimer;
	ev_timer m_evChangeRoleTimer;

};

#endif /*_ALL_KILL_GAME_H_*/


