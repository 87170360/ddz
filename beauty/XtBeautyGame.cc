#include <assert.h>

#include "XtBeautyGame.h"
#include "XtJsonPacket.h"
#include "XtBeautyServer.h"
#include "XtBeautyPlayer.h"
#include "XtLog.h"

void XtBeautyGame::onReadyTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	XtBeautyGame* game=(XtBeautyGame*)w->data;
	game->readyTimer();
}

void XtBeautyGame::onStartTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	XtBeautyGame* game=(XtBeautyGame*)w->data;
	game->startTimer();
}


void XtBeautyGame::onEndGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	XtBeautyGame* game=(XtBeautyGame*)w->data;
	game->endGameTimer();
}

void XtBeautyGame::onUpdateGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	XtBeautyGame* game=(XtBeautyGame*)w->data;
	game->updateGameTimer();
}



XtBeautyGame::XtBeautyGame()
{
	m_server=NULL;
	m_status=BG_READY;
	m_evLoop=NULL;
}


XtBeautyGame::~XtBeautyGame()
{

}

int XtBeautyGame::configGame(Json::Value* value)
{
	m_gameConfig.m_baseMoney=(*value)["venue"]["base_money"].asInt();
	m_gameConfig.m_middleRadio=(*value)["venue"]["middle_radio"].asInt();
	m_gameConfig.m_minBetMoney=(*value)["venue"]["min_bet_money"].asInt();
	m_gameConfig.m_fee=(*value)["venue"]["fee"].asDouble();

	m_gameConfig.m_maxGameResultHistory=(*value)["venue"]["game_history_nu"].asInt();

	return 0;
}






int XtBeautyGame::start(XtGameServer* server,struct ev_loop* loop) 
{
	m_status=BG_READY;
	m_server=server;
	m_evLoop=loop;


	m_evReadyTimer.data=this;
	ev_timer_init(&m_evReadyTimer,XtBeautyGame::onReadyTimer,0,BG_READY_TIME);

	m_evStartTimer.data=this;
	ev_timer_init(&m_evStartTimer,XtBeautyGame::onStartTimer,0,BG_START_TIME);

	m_evEndGameTimer.data=this;
	ev_timer_init(&m_evEndGameTimer,XtBeautyGame::onEndGameTimer,0,BG_END_GAME_TIME);

	m_evUpdateTimer.data=this;
	ev_timer_init(&m_evUpdateTimer,XtBeautyGame::onUpdateGameTimer,0,BG_UPDATE_TIME);


	ev_timer_again(m_evLoop,&m_evReadyTimer);
	handleGameReady();

	return 0;

}



int XtBeautyGame::shutDown()
{

	ev_timer_stop(m_evLoop,&m_evReadyTimer);
	ev_timer_stop(m_evLoop,&m_evStartTimer);
	ev_timer_stop(m_evLoop,&m_evEndGameTimer);
	ev_timer_stop(m_evLoop,&m_evUpdateTimer);

	for(std::map<int,XtBeautyPlayer*>::iterator iter=m_offlinePlayers.begin();iter!=m_offlinePlayers.end();++iter)
	{
		XtBeautyPlayer* player=iter->second;
		assert(player);
		delete player;
	}

	for(std::map<int,XtBeautyPlayer*>::iterator iter=m_loginPlayers.begin();iter!=m_loginPlayers.end();++iter)
	{
		XtBeautyPlayer* player=iter->second;
		//	printf("uid=%d,player_addr=%ld\n",iter->first,(long)iter->second);
		assert(player);

		delete player;
	}

	m_loginPlayers.clear();
	m_betPlayers.clear();

	m_server=NULL;
	return 0;

}


void XtBeautyGame::onReciveClientCmd(XtGamePlayer* player,XtJsonPacket* packet)
{
	int cmd=packet->m_val["cmd"].asInt();
	switch(cmd)
	{
		case BG_PLAYER_BET_C:
			playerBet((XtBeautyPlayer*)player,packet);
			break;
		default:
			XT_LOG_ERROR("unkown cmd(%d) from player(%d)\n",cmd,player->getUid());
	}
}



XtGamePlayer* XtBeautyGame::getPlayer(int uid)
{
	std::map<int,XtBeautyPlayer*>::iterator iter= m_offlinePlayers.find(uid);
	if(iter!=m_offlinePlayers.end())
	{
		return iter->second;
	}

	iter=m_loginPlayers.find(uid);


	if(iter!=m_loginPlayers.end())
	{
		XT_LOG_ERROR("player(%d) allready login\n",uid);

		return iter->second;
	}


	XtBeautyPlayer* player=new XtBeautyPlayer;

	player->setData(uid,m_server,this);

	m_offlinePlayers[uid]=player;

	return player;
}


int XtBeautyGame::playerLogin(XtGamePlayer* _player) 
{
	XtBeautyPlayer* player=(XtBeautyPlayer*)_player;

	assert(player!=NULL);
	int uid=player->getUid();

	std::map<int,XtBeautyPlayer*>::iterator iter;
	iter=m_loginPlayers.find(uid);
	if(iter!=m_loginPlayers.end())
	{
		XT_LOG_ERROR("player(%d) all ready login\n",uid);
		return -1;
	}

	iter= m_offlinePlayers.find(uid);


	if(iter!=m_offlinePlayers.end())
	{
		m_loginPlayers[iter->first]=iter->second;
		m_offlinePlayers.erase(iter);
	}
	else 
	{
		XT_LOG_ERROR("player not  find in offline\n");
	}


	sendLoginSuccess(player);
	sendGameInfo(player);

	return 0;
}




int XtBeautyGame::playerLogout(XtGamePlayer* _player)
{
	XtBeautyPlayer* player=(XtBeautyPlayer*)_player;

	int uid=player->getUid();
	if(m_status==BG_END_GAME)
	{
		m_loginPlayers.erase(uid);
		sendLogoutSuccess(player);
		
		if(m_betPlayers.find(uid)!=m_betPlayers.end())
		{
			m_offlinePlayers[uid]=player;
		}
		else 
		{
			delete player;
		}
		return 0;
	}

	if(m_status==BG_READY)
	{
		m_loginPlayers.erase(uid);
		sendLogoutSuccess(player);
		delete player;

		return 0;
	}

	if(m_status==BG_START)
	{
		if(m_betPlayers.find(uid)!=m_betPlayers.end())
		{
			m_loginPlayers.erase(uid);
			m_offlinePlayers[uid]=player;
			sendLogoutSuccess(player);
		}
		else 
		{
			m_loginPlayers.erase(uid);
			sendLogoutSuccess(player);
			delete player;
		}
	}
	return 0;
}

void XtBeautyGame::playerBet(XtBeautyPlayer* player,XtJsonPacket* packet)
{
	int seat_id=packet->m_val["seat_id"].asInt();
	int bet_nu=packet->m_val["bet_nu"].asInt();


	if(m_status!=BG_START)
	{
		sendBetError(player,BG_STATUS_ERR,"");
		return;
	}


	if(bet_nu<=0)
	{
		sendBetError(player,BG_BET_NU_ERR,"bet nu greater than 0");
		return;
	}

	if(seat_id<0&&seat_id>=BG_SEAT_ID_NU)
	{
		sendBetError(player,BG_SEAT_ID_ERR,"");
		return;
	}



	int mlock_time=player->getMoneyLockTime();

	if(mlock_time==-1)
	{
		sendBetError(player,BG_MLOCK_ERR,"");
		return;
	}

	int time_now=time(NULL);

	if(mlock_time>time_now)
	{
		sendBetError(player,BG_MLOCK_ERR,"");
		return;
	}



	int money=player->getMoney();

	if(money<m_gameConfig.m_baseMoney*bet_nu||money<m_gameConfig.m_minBetMoney)
	{
		money=player->updateMoney();
	}

	if(money<m_gameConfig.m_baseMoney*bet_nu || money<m_gameConfig.m_minBetMoney)
	{
		sendBetError(player,BG_MONEY_NOT_ENOUGH,"");
		return;
	}

	m_gameBetting.m_seatBet[seat_id]+=bet_nu;

	player->decMoney(m_gameConfig.m_baseMoney*bet_nu);

	m_server->sendFlow(XT_BET_FLOW_ID,0,player->getUid(),2,-m_gameConfig.m_baseMoney*bet_nu,player->getMoney());


	player->addSeatBet(seat_id,bet_nu);



	int uid=player->getUid();

	m_betPlayers[uid]=player;
	sendBetSuccess(player,seat_id);
}


void XtBeautyGame::readyTimer()
{
	ev_timer_stop(m_evLoop,&m_evReadyTimer);
	m_status=BG_START;

	handleGameStart();

	ev_timer_again(m_evLoop,&m_evStartTimer);
	ev_timer_again(m_evLoop,&m_evUpdateTimer);
}

void XtBeautyGame::startTimer()
{
	ev_timer_stop(m_evLoop,&m_evStartTimer);
	ev_timer_stop(m_evLoop,&m_evUpdateTimer);

	m_status=BG_END_GAME;
	handleGameEnd();

	//ev_timer_set(&m_evEndGameTimer,0,BG_END_GAME_TIME);
	ev_timer_again(m_evLoop,&m_evEndGameTimer);
}


void XtBeautyGame::endGameTimer()
{
	ev_timer_stop(m_evLoop,&m_evEndGameTimer);

	m_status=BG_READY;

	handleGameReady();
	ev_timer_again(m_evLoop,&m_evReadyTimer);
}


void XtBeautyGame::updateGameTimer()
{
	ev_timer_again(m_evLoop,&m_evUpdateTimer);
	handleGameUpdate();
}

void XtBeautyGame::handleGameReady()
{

	m_gameBetting.reset();
	m_gameBetting.deal();

	/* reset bet */
	for(std::map<int,XtBeautyPlayer*>::iterator iter=m_betPlayers.begin();
			iter!=m_betPlayers.end();++iter)
	{
		iter->second->resetSeatBet();
	}
	m_betPlayers.clear();


	/* offline */
	for(std::map<int,XtBeautyPlayer*>::iterator iter=m_offlinePlayers.begin();
			iter!=m_offlinePlayers.end();++iter)
	{
		delete iter->second;
	}

	m_offlinePlayers.clear();

	m_gameBetting.m_mostWinPlayer=NULL;

	broadcastGameReady(NULL);
}


void XtBeautyGame::handleGameStart()
{
	broadcastGameStart(NULL);

	((XtBeautyServer*)m_server)->refreshPlayerNuToRedis();

}


void XtBeautyGame::handlePlayerBetResult()
{
	if(m_gameBetting.m_leftCard.getCardType()==m_gameBetting.m_rightCard.getCardType())
	{
		m_gameBetting.m_middleWin=1;
	}
	else 
	{
		m_gameBetting.m_middleWin=0;
	}

	if(m_gameBetting.m_leftCard.compare(m_gameBetting.m_rightCard)<=0)
	{
		m_gameBetting.m_leftWin=0;
	}
	else 
	{
		m_gameBetting.m_leftWin=1;
	}


	m_gameResultList.push_back(XtBeautyGameResult(m_gameBetting.m_leftWin,m_gameBetting.m_middleWin,!m_gameBetting.m_leftWin));

	if(m_gameResultList.size()>(unsigned int) m_gameConfig.m_maxGameResultHistory)
	{
		m_gameResultList.pop_front();
	}

	for(std::map<int,XtBeautyPlayer*>::iterator iter=m_betPlayers.begin();iter!=m_betPlayers.end();++iter)
	{
		XtBeautyPlayer* player=iter->second;
		player->updateMoney();

		for(int i=0;i<BG_SEAT_ID_NU;i++)
		{
			int bet_nu=player->getSeatBet(i);
			if(bet_nu==0)
			{
				continue;
			}

			switch(i)
			{
				case BG_LEFT_CARD_ID:
					{
						if(m_gameBetting.m_leftWin)
						{
							int total_money= bet_nu*m_gameConfig.m_baseMoney*2;
							int fee=total_money*m_gameConfig.m_fee;
							player->incMoney(total_money-fee);

							player->addBetMoneyResult(bet_nu*m_gameConfig.m_baseMoney);
							m_server->sendFlow(XT_RESULT_FLOW_ID,0,player->getUid(),2,total_money-fee,player->getMoney());
						}
						else 
						{
							player->addBetMoneyResult(-bet_nu*m_gameConfig.m_baseMoney);
						}
					}
					break;

				case BG_RIGHT_CARD_ID:
					{
						if(m_gameBetting.m_leftWin)
						{
							player->addBetMoneyResult(-bet_nu*m_gameConfig.m_baseMoney);
						}
						else 
						{

							int total_money= bet_nu*m_gameConfig.m_baseMoney*2;
							int fee=total_money*m_gameConfig.m_fee;
							player->incMoney(total_money-fee);

							player->addBetMoneyResult(bet_nu*m_gameConfig.m_baseMoney);

							m_server->sendFlow(XT_RESULT_FLOW_ID,0,player->getUid(),2,total_money-fee,player->getMoney());


						}

					}
					break;
				case BG_MIDDLE_CARD_ID:
					{
						if(m_gameBetting.m_middleWin)
						{
							int total_money= bet_nu*m_gameConfig.m_baseMoney*(1+m_gameConfig.m_middleRadio);
							int fee= total_money*m_gameConfig.m_fee;
							player->incMoney(total_money-fee);
							player->addBetMoneyResult(bet_nu*m_gameConfig.m_baseMoney*m_gameConfig.m_middleRadio);

							m_server->sendFlow(XT_RESULT_FLOW_ID,0,player->getUid(),2,total_money-fee,player->getMoney());
						}
						else 
						{
							player->addBetMoneyResult(-bet_nu*m_gameConfig.m_baseMoney);
						}

					}
					break;
			}
		}
	}

	/* get most win bet player */
	for(std::map<int,XtBeautyPlayer*>::iterator iter=m_betPlayers.begin();iter!=m_betPlayers.end();++iter)
	{
		XtBeautyPlayer* player=iter->second;
		if(player->getBetMoneyResult()>0)
		{
			if(m_gameBetting.m_mostWinPlayer)
			{
				if(player->getBetMoneyResult()>m_gameBetting.m_mostWinPlayer->getBetMoneyResult())
				{
					m_gameBetting.m_mostWinPlayer=player;
				}
			}
			else 
			{
				m_gameBetting.m_mostWinPlayer=player;
			}
		}
	}

}




void XtBeautyGame::handleGameEnd()
{
	m_gameBetting.m_endGameTime=time(NULL);

	handlePlayerBetResult();

	broadcastGameUpdate(NULL);
	broadcastGameEnd(NULL);

	sendMostWinToSpeaker();

	sendBetPlayerResult();
}





void XtBeautyGame::handleGameUpdate()
{
	m_gameBetting.m_betRemainTime--;
	if(m_gameBetting.m_betRemainTime<0)
	{
		m_gameBetting.m_betRemainTime=0;
	}
	broadcastGameUpdate(NULL);
}






void XtBeautyGame::sendLoginSuccess(XtBeautyPlayer* player)
{
	XtJsonPacket packet;

	packet.m_val["cmd"]=BG_LOGIN_SUCC_SU;
	packet.m_val["ret"]=0;

	packet.pack();

	m_server->unicast(player,&packet);

}

void XtBeautyGame::sendLogoutSuccess(XtBeautyPlayer* player)
{

	XtJsonPacket packet;

	packet.m_val["cmd"]=BG_LOGOUT_SUCC_SU;
	packet.m_val["ret"]=0;

	packet.pack();

	m_server->unicast(player,&packet);
}



void XtBeautyGame::sendGameInfo(XtBeautyPlayer* player)
{

	XtJsonPacket packet;
	packet.m_val["cmd"]=BG_GAME_INFO_SU ;

	packet.m_val["base_money"]=m_gameConfig.m_baseMoney;
	packet.m_val["middle_radio"]=m_gameConfig.m_middleRadio;
	packet.m_val["min_bet_money"]=m_gameConfig.m_minBetMoney;

	packet.m_val["status"]=m_status;



	if(m_status==BG_READY)
	{

	}
	else if(m_status==BG_START)
	{
		for(int i=0;i<BG_SEAT_ID_NU;i++)
		{
			packet.m_val["bet_info"][i]["seat_id"]=i;
			packet.m_val["bet_info"][i]["seat_bet_nu"]=m_gameBetting.m_seatBet[i];
			packet.m_val["bet_info"][i]["your_bet_nu"]=player->getSeatBet(i);
		}

		packet.m_val["remain_time"]=m_gameBetting.m_betRemainTime;
	}
	else if(m_status==BG_END_GAME)
	{
		formatGameResult(&packet);
	}

	int i=0;
	for(std::deque<XtBeautyGameResult>::iterator iter=m_gameResultList.begin();
			iter!=m_gameResultList.end();
			++iter)
	{
		int j=0;
		packet.m_val["game_history"][i][j]=(*iter).m_left;
		packet.m_val["game_history"][i][1]=(*iter).m_middle;
		packet.m_val["game_history"][i][2]=(*iter).m_right;
		i++;
	}

	packet.pack();
	m_server->unicast(player,&packet);

}





void XtBeautyGame::sendBetError(XtBeautyPlayer* player,int code, const char* desc)
{
	XtJsonPacket packet;
	packet.m_val["cmd"]=BG_PLAYER_BET_RESULT_SU;
	packet.m_val["ret"]=code;
	packet.m_val["desc"]=desc;
	packet.pack();

	m_server->unicast(player,&packet);
}


void XtBeautyGame::sendBetSuccess(XtBeautyPlayer* player,int seat_id)
{
	XtJsonPacket packet;

	packet.m_val["cmd"]=BG_PLAYER_BET_RESULT_SU;
	packet.m_val["ret"]=0;

	packet.m_val["bet_info"]["seat_id"]=seat_id;
	packet.m_val["bet_info"]["seat_bet_nu"]=m_gameBetting.m_seatBet[seat_id];
	packet.m_val["bet_info"]["your_bet_nu"]=player->getSeatBet(seat_id);
	packet.m_val["money"]=player->getMoney();

	packet.pack();

	m_server->unicast(player,&packet);
}


void XtBeautyGame::sendBetPlayerResult()
{
	for(std::map<int,XtBeautyPlayer*>::iterator iter=m_betPlayers.begin();iter!=m_betPlayers.end();++iter)
	{
		XtBeautyPlayer* player=iter->second;

		XtJsonPacket packet;
		packet.m_val["cmd"]=BG_PLAYER_BET_REWARD_SU;
		packet.m_val["reward"]=player->getBetMoneyResult();
		packet.m_val["money"]=player->getMoney();

		packet.pack();

		m_server->unicast(player,&packet);
	}
}


void XtBeautyGame::sendMostWinToSpeaker()
{
	if(m_gameBetting.m_mostWinPlayer)
	{
		if(m_gameBetting.m_mostWinPlayer->getUid()<10000)
		{
			return;
		}

		std::string name=m_gameBetting.m_mostWinPlayer->getName();

		int money=m_gameBetting.m_mostWinPlayer->getBetMoneyResult();
		int m_w=money/10000;
		char buf[2048];
		if(m_w>500)
		{
			sprintf(buf, "%s在主播场中破釜沉舟疯狂下注最终赢得%d万金币满载而回",name.c_str(),m_w);
		}
		else if(m_w>100)
		{
			sprintf(buf,"%s在主播场中虎胆龙威一本万利夺得%d万金币",name.c_str(),m_w);
		}
		else if(m_w>25)
		{
			sprintf(buf,"%s在主播场中勇闯虎穴拼得%d万金币",name.c_str(),m_w);
		}
		else 
		{
			return;
		}
		m_server->sendSpeaker(4006,m_gameBetting.m_mostWinPlayer->getUid(),"系统",buf);
	}
}


void XtBeautyGame::broadcastGameReady(XtBeautyPlayer* player)
{
	XtJsonPacket packet;

	packet.m_val["cmd"]=BG_GAME_READY_SB;

	packet.pack();

	m_server->broadcast(player,&packet);

}


void XtBeautyGame::broadcastGameStart(XtBeautyPlayer* player)
{
	XtJsonPacket packet;
	packet.m_val["cmd"]=BG_GAME_START_SB;
	packet.m_val["remain_time"]=m_gameBetting.m_betRemainTime;
	packet.pack();
	m_server->broadcast(player,&packet);

}


void XtBeautyGame::broadcastGameEnd(XtBeautyPlayer* player)
{
	XtJsonPacket packet;

	packet.m_val["cmd"]=BG_GAME_END_SB;
	formatGameResult(&packet);
	packet.pack();

	m_server->broadcast(player,&packet);

}



void XtBeautyGame::broadcastGameUpdate(XtBeautyPlayer* player)
{

	XtJsonPacket packet;

	packet.m_val["cmd"]=BG_GAME_UPDATE_SB;

	for(int i=0;i<BG_SEAT_ID_NU;i++)
	{
		packet.m_val["seat_bet_info"][i]["seat_id"]=i;
		packet.m_val["seat_bet_info"][i]["bet_nu"]=m_gameBetting.m_seatBet[i];
	}

	packet.m_val["remain_time"]=m_gameBetting.m_betRemainTime;

	packet.pack();

	m_server->broadcast(NULL,&packet);
}




void XtBeautyGame::formatGameResult(XtJsonPacket* packet)
{

	int remain_time=BG_END_GAME_TIME-(time(NULL)-m_gameBetting.m_endGameTime);
	if(remain_time<0)
	{
		remain_time=0;
	}

	packet->m_val["remain_time"]=remain_time;

	int left_win=m_gameBetting.m_leftWin;
	int middle_win=m_gameBetting.m_middleWin;

	if(m_gameBetting.m_mostWinPlayer!=NULL)
	{
		packet->m_val["most_win_player"]["uid"]=m_gameBetting.m_mostWinPlayer->getUid();
		packet->m_val["most_win_player"]["sex"]=m_gameBetting.m_mostWinPlayer->getSex();
		packet->m_val["most_win_player"]["avatar"]=m_gameBetting.m_mostWinPlayer->getAvatar();
		packet->m_val["most_win_player"]["name"]=m_gameBetting.m_mostWinPlayer->getName();
		packet->m_val["most_win_player"]["reward"]=m_gameBetting.m_mostWinPlayer->getBetMoneyResult();
		packet->m_val["has_most_win"]=1;
	}
	else 
	{
		packet->m_val["has_most_win"]=0;
	}


	for(int i=0;i<BG_SEAT_ID_NU;i++)
	{
		switch(i)
		{
			case BG_LEFT_CARD_ID:
				{
					packet->m_val["seat_info"][i]["is_win"]=left_win;
					for(unsigned int j=0;j<m_gameBetting.m_leftCard.m_cards.size();j++)
					{
						packet->m_val["seat_info"][i]["card"].append(m_gameBetting.m_leftCard.m_cards[j].m_value);

						packet->m_val["seat_info"][i]["card_type"]=m_gameBetting.m_leftCard.getCardType();

						packet->m_val["seat_info"][i]["bet_nu"]=m_gameBetting.m_seatBet[i];

					}
				}
				break;

			case BG_RIGHT_CARD_ID:
				{

					packet->m_val["seat_info"][i]["is_win"]=left_win?0:1;
					for(unsigned int j=0;j<m_gameBetting.m_rightCard.m_cards.size();j++)
					{
						packet->m_val["seat_info"][i]["card"].append(m_gameBetting.m_rightCard.m_cards[j].m_value);

						packet->m_val["seat_info"][i]["card_type"]=m_gameBetting.m_rightCard.getCardType();

						packet->m_val["seat_info"][i]["bet_nu"]=m_gameBetting.m_seatBet[i];


					}
				}
				break;

			case BG_MIDDLE_CARD_ID:
				{
					packet->m_val["seat_info"][i]["is_win"]=middle_win;
					packet->m_val["seat_info"][i]["bet_nu"]=m_gameBetting.m_seatBet[i];
				}
				break;
		}
	}
}

