#ifndef _XT_BEAUTY_GAME_H_
#define _XT_BEAUTY_GAME_H_


#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <map>
#include <ev.h>


#include "XtHoleCards.h"

#include "XtGameLogic.h"
#include "XtBeautyMacros.h"
#include "XtNetMacros.h"
#include "XtTypeDeck.h"


#define XT_BET_FLOW_ID 312 
#define XT_RESULT_FLOW_ID 313


class XtGameServer;
class XtBeautyPlayer;
class XtJsonPacket;

class XtBeautyServer;



class XtBeautyGameResult 
{
	public:
		XtBeautyGameResult(int left,int middle,int right)
		{
			m_left=left;
			m_middle=middle;
			m_right=right;
		}

	public:
		int m_left;
		int m_middle;
		int m_right;
};


class XtBeautyGameConfig 
{
	public:
		int m_baseMoney;
		int m_middleRadio;
		int m_minBetMoney;
		float m_fee;
		int m_maxGameResultHistory;
};



class XtBeautyBetting 
{
	public:
		XtBeautyBetting()
		{
			reset();
		}

	public:
		void reset()
		{
			printf("reset game\n");

			m_deck.fill();
			m_deck.shuffle(2);

			for(int i=0;i<BG_SEAT_ID_NU;i++)
			{
				m_seatBet[i]=0;
			}

			m_betRemainTime=0;
			m_endGameTime=0;
			m_leftWin=0;
			m_middleWin=0;
			m_mostWinPlayer=NULL;
		}

		void deal()
		{

			int card_type1=m_deck.getHoleCards(&m_leftCard);
			int card_type2=m_deck.getHoleCards(&m_rightCard);

			m_leftCard.analysis();
			m_rightCard.analysis();

			assert(card_type1==m_leftCard.getCardType());
			assert(card_type2==m_rightCard.getCardType());

			m_betRemainTime=BG_START_TIME;

		}


	public:
		XtTypeDeck m_deck;
		int m_seatBet[BG_SEAT_ID_NU];

		XtHoleCards m_leftCard;
		XtHoleCards m_rightCard;

		int m_betRemainTime;
		int m_endGameTime;

		int m_leftWin;
		int m_middleWin;

		XtBeautyPlayer* m_mostWinPlayer;

};



class XtBeautyGame :public XtGameLogic
{
	public:
		static void onReadyTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
		static void onStartTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
		static void onEndGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents);
		static void onUpdateGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents);

	public:
		XtBeautyGame();
		virtual ~XtBeautyGame();

	public:
		int configGame(Json::Value* value) XT_OVERRIDE;
		int start(XtGameServer* server,struct ev_loop* loop) XT_OVERRIDE;
		int shutDown() XT_OVERRIDE;

		void onReciveClientCmd(XtGamePlayer* player,XtJsonPacket* packet) XT_OVERRIDE;

		XtGamePlayer* getPlayer(int uid) XT_OVERRIDE;

		int playerLogin(XtGamePlayer* player) XT_OVERRIDE;
		int playerLogout(XtGamePlayer* player) XT_OVERRIDE;


	public:
		void playerBet(XtBeautyPlayer* player,XtJsonPacket* packet);



	public:
		void readyTimer();
		void startTimer();
		void endGameTimer();
		void updateGameTimer();


	protected:
		void handleGameReady();
		void handleGameEnd();
		void handleGameUpdate();
		void handleGameStart();
		void handlePlayerBetResult();

	protected:
		void sendLoginSuccess(XtBeautyPlayer* player);
		void sendLogoutSuccess(XtBeautyPlayer* player);

		void sendGameInfo(XtBeautyPlayer* player);
		void sendBetError(XtBeautyPlayer* player,int code,const char* desc);
		void sendBetSuccess(XtBeautyPlayer* player,int seatid);
		void sendBetPlayerResult();
		void sendMostWinToSpeaker();
		

		void broadcastGameReady(XtBeautyPlayer* player);
		void broadcastGameStart(XtBeautyPlayer* player);
		void broadcastGameEnd(XtBeautyPlayer* player);
		void broadcastGameUpdate(XtBeautyPlayer* player);

	protected:
		void formatGameResult(XtJsonPacket* packet);

	private:
		XtGameServer* m_server;

		int m_status;

		XtBeautyGameConfig m_gameConfig;

		XtBeautyBetting m_gameBetting;



		std::map<int,XtBeautyPlayer*> m_offlinePlayers; 	
		std::map<int,XtBeautyPlayer*> m_loginPlayers; 
		std::map<int,XtBeautyPlayer*> m_betPlayers;

		std::deque<XtBeautyGameResult> m_gameResultList;


		struct ev_loop* m_evLoop;
		ev_timer m_evReadyTimer;
		ev_timer m_evStartTimer;
		ev_timer m_evEndGameTimer;
		ev_timer m_evUpdateTimer;
};





#endif /*_XT_BEAUTY_GAME_H_*/



