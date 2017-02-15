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

// ��ע����
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

// ��Ϸ��Ӯ�����Ϣ
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
	int m_seatid[AK_SEAT_ID_NU];   // 4����λ����Ӯ������ ��0�� 1Ӯ��
};

// ��һ�������
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
    // ���û�������
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
	std::string m_avatar;      // ͷ��
	std::string m_name;        // ����
	int m_sex;                 // �Ա�
	int m_rewardMoney;         // �ͽ�   
	int m_rewardTime;          // �񽱵�ʱ��
	int m_cardType;            // �񽱵�����
};

// ������Ϣ
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
	bool m_hasOpenLottery;                 // �Ƿ񿪽�
	int m_roleLotteryMoney;                // ׯ�һ񽱽��
	int m_seatLotteryMoney[AK_SEAT_ID_NU]; // ÿ����λ�Ľ���
	int m_cardType;
};


// seat��Ϸ���
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
	CT_BULL_TYPE m_cardType;                      // ����
	int m_isWin;                                  // 0:�� 1:Ӯ
	int m_betTime;                                // Ӯ�����ͱ���
	int m_totalBetNu;                             // ��ע��
	std::vector<AllKillPlayer*> m_betPlayerList;  // ��seat������б�
};


// ׯ����Ϸ���
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
	CT_BULL_TYPE m_cardType;  // ����
	int m_betTime;            // Ӯ�����ͱ���

	int m_roleLostMoney;      // ׯ����
	int m_roleRealWinMoney;   // ׯ��ʵ��Ӯ
	int m_roleWinMoney;       // ׯ��Ӯ
	int m_roleResultBetMoney;  
};



// ��Ϸ�Ծ֣���װλ�õ���ע���ƣ�ׯ�ҵ��ƣ���Ϸʣ��ʱ�䣩
class GameBetting
{

public:

	GameBetting()
	{
		m_GameLogic = CBullGameLogic::GetInstance();
		reset();
	}

public:
	//����
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

		m_roleCardType = CT_NOBULL;  // ûţ

	}

	// ����
	void deal(bool sys_role,int radio);


public:

	CBullGameLogic *m_GameLogic;

	int m_seatBet[AK_SEAT_ID_NU];                                //λ�õ���ע��

	_uint8 m_seatCard[AK_SEAT_ID_NU][MAX_HAND_CARD_COUNT];       // λ����
	_uint8 m_seatGroupCard[AK_SEAT_ID_NU][MAX_HAND_CARD_COUNT];  // λ����õ���
	CT_BULL_TYPE m_seatCardType[AK_SEAT_ID_NU];                  // λ������


	_uint8 m_roleCard[MAX_HAND_CARD_COUNT];                 // ׯ�ҵ���
	_uint8 m_roleGroupCard[MAX_HAND_CARD_COUNT];            // ׯ����õ���
	CT_BULL_TYPE m_roleCardType;                            // ׯ�ҵ�����

	int m_betRemainTime;    // ��עʣ��ʱ��
	int m_endGameTime;      // ��Ϸ����ʱ��
};




// ��Ϸ������Ϣ
class GameConfig
{
public:
	int m_baseMoney;         // ��ע��Ͷע���Ի�עΪ��λ��
	int m_askRoleMinMoney;   // ��ׯ����
	int m_unRoleLimitMoney;  // ��ׯ���ƣ��������ֵ��������ׯ��

	/* role another radio */
	int m_roleAnotherCard;    // **���Ƹ���


	/* sys role info */   
	std::string m_sysRoleName;     // ϵͳׯ�� - ����
	std::string m_sysRoleAvatar;   // ϵͳׯ�� - ͷ��
	int m_sysRoleUid;              // ϵͳׯ�� - uid     
	int m_sysRoleMoney;            // ϵͳׯ�� - ��� 
	int m_sysRoleSex;              // ϵͳׯ�� - �Ա�

	/* sys chat role info */ //ϵͳ����ׯ����Ϣ
	std::string m_chatRoleName;
	std::string m_chatRoleAvatar;
	int  m_chatRoleUid;
	int m_chatRoleSex;

	/* enter room chat */ //���뷿��Ļ�ӭ��
	std::string m_enterRoomChat;

	/*rottle fee */  //��ȯ��ˮ����
	float m_winRottleFee;  

	/* sys fee */ //ϵͳ��ˮ����
	float m_sysFee;


	/* rottle radio */  //����ؿ�������
	float m_rottleBombRadio;
	float m_rottleFiveHBullRadio;
	float m_rottleFiveSBullRadio;

	//������ҵ��ޣ�vlevel�ȼ�����
	int m_sitMinMoney;	
	int m_sitVLevel;

	/* rottle min money */ //��������
	int m_rottleMinOpenMoney;

	/* game result max history */ //չʾ��Ϸ�����ʷ�������
	int m_gameHistoryNum;

	int m_vid;

};


/////////*****************���˳�******************////////////////

class AllKillGame 
{

public:
	AllKillGame();
	~AllKillGame();

public:
	// ��ȡ���˳�������
	int configGame(Json::Value& value);
	// �������˳�
	int start(AllKillServer* server,struct ev_loop* loop);
	// �ر����˳�
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
	void playerBet(AllKillPlayer* player,Jpacket& package);      // �����ע
	void playerAskRole(AllKillPlayer* player,Jpacket& package);  // ��ׯ
	void playerUnRole(AllKillPlayer* player,Jpacket& package);   // ��ׯ
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
	// ����Ծ������Ϣ
	void formatBetPlayerResult(Jpacket* packet);

	void formatGameResult(Jpacket* packet);
	void formatAskRoleList(Jpacket* packet);
	void formatRole(Jpacket* packet);
	void formatLotteryFirstReward(Jpacket* packet);


public:
	void sendLoginSuccess(AllKillPlayer* player);
	void sendGameInfo(AllKillPlayer* player);


	void broadcastGameReady(AllKillPlayer* player);     // �㲥��Ϸ׼��
	void broadcastGameStart(AllKillPlayer* player);     // ��ʼ
	void broadcastGameEnd(AllKillPlayer* player);       // ����
	void broadcastGameUpdate(AllKillPlayer* player);    // ����
	void broadcastAskRoleChange(AllKillPlayer* player); // �ı��ɫ

	//������Ϣ
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

	// �����͸���
	int getCardTypeNu(int type);

	// ���ͽ��ؿ�������
	float getCardTypeLotteryRadio(int type);

	void handleRottleResultByCardType(int card_type,int num);
	int setNextRole();

	int doSitDown(AllKillPlayer* player, int seat_id);
	int doStandUp(AllKillPlayer* player);

	void addLotteryFirstReward(LotteryFirstReward lotteryFirst);

public:
	// ����
	void playerSitDown(AllKillPlayer* player, int seat_id);

	// վ��
	void playerStandUp(AllKillPlayer* player,int seat_id);

	// վ��
	void playerStandUp(AllKillPlayer* player);


	void sendLotterFirstPlayers(AllKillPlayer* player);
	
private:

	//������ׯ����
	int m_role_nu;

	//�м���λuin����λ��0��ʼ��
	int m_playerSeat[AK_SEAT_ID_NU];     


	//��Ϸ�������б�
	std::deque<GameResultInfo> m_gameResultList;

	//�������
	std::map<int,AllKillPlayer*> m_offlinePlayers; 	
	//��¼���
	std::map<int,AllKillPlayer*> m_loginPlayers; 
	//��ע���
	std::map<int,AllKillPlayer*> m_betPlayers;  
	//��ׯ���
	std::vector<AllKillPlayer*> m_askRoleList;  

	//��ǰׯ��
	AllKillPlayer* m_role;         

	AllKillServer* m_server;

	/* game status*/
	int m_gameStatus;


	/* rottle  money */
	int m_lotteryTotalMoney;  //���ؽ��


	/* game config */
	GameConfig m_gameConfig;


	/* game betting �Ծ���Ϣ*/
	GameBetting m_gameBetting;


	/*game result temp info */ 
	SeatGameResult m_seatGameResult[AK_SEAT_ID_NU];  // ��λ��Ϸ���
	RoleGameResult m_roleGameResult;                 // ׯ����Ϸ���

	// ���������Ϣ
	LotteryResult m_lotteryResult;         

	// ��һ�����б�
	std::deque<LotteryFirstReward> m_lotterFirstRewardList;
	
	// �񽱵�һ������
	LotteryFirstReward m_lotteryFirstReward;

	// ӮǮ������
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


