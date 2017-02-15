#include <assert.h>
#include <math.h>
#include "AllKillGame.h"
#include "AllKillMacros.h"
#include "AllKillPlayer.h"
#include "AllKillServer.h"

#include "log.h"

extern Log xt_log;



//����
void GameBetting::deal(bool sys_role,int radio)
{
	xt_log.info("Func[%s] deal card ,role another card radio %d.\n",
		__FUNCTION__, radio);

	// 1)ϴ���߼�
	_uint8 GoodCardBuff[AK_HAND_CARD_COUNT][MAX_COUNT];  //5����         
	memset(GoodCardBuff, 0, sizeof(GoodCardBuff));

	//_uint8 GoodChairId[AK_SEAT_ID_NU] = { -1 }; //���Ƶ�����

	_uint8 bRandCard[CARDCOUNT] = { 0 };       //52����                  
	memset(bRandCard, 0, sizeof(bRandCard));

	int bGoodsCardSwtich = 0;   ///��д��

	if (bGoodsCardSwtich == 1)
	{
		//ͳ��ÿ����ҵľ���
		//int count[AK_SEAT_ID_NU] = { -1 };
		//bool is_robot[AK_SEAT_ID_NU] = { false };
		//for (int i = 0; i < AK_SEAT_ID_NU; i++)
		//{
		//	if (seats[i].status < STATUS_IN_GAME)
		//		continue;

		//	count[i] = seats[i].player->total_board; ///�ܵĶԾ���
		//	if (seats[i].player->uid < 10000)
		//	{
		//		is_robot[i] = true;
		//	}
		//}

		////ϴ����
		//m_GameLogic->GoodRandCardList(count, is_robot, bRandCard, CARDCOUNT, AK_SEAT_ID_NU, GoodCardBuff, GoodChairId);
	}
	else
	{
		//���ϴ
		memcpy(bRandCard, m_cbCardData, CARDCOUNT);
		m_GameLogic->RandCardList(bRandCard, CARDCOUNT);
	}

	// 2)����--��ׯ��
	int len = 0;
	memcpy(m_roleCard, bRandCard + len, MAX_HAND_CARD_COUNT);
	len += MAX_HAND_CARD_COUNT;

	// ����
	m_roleCardType = (CT_BULL_TYPE)m_GameLogic->GetCardTypeAndData(
		m_roleCard,  MAX_HAND_CARD_COUNT, m_roleGroupCard);

	//�����ϵͳׯ�ң��õ�"ûţ"�Ļ�����һ���ĸ������·��Ƹ���
	if (CT_NOBULL == m_roleCardType && sys_role)
	{
		if (rand() % 100 < radio)
		{
			memcpy(m_roleCard, bRandCard + len, MAX_HAND_CARD_COUNT);
			len += MAX_HAND_CARD_COUNT;

			// ����
			m_roleCardType = (CT_BULL_TYPE)m_GameLogic->GetCardTypeAndData(
				m_roleCard, MAX_HAND_CARD_COUNT, m_roleGroupCard);
		}
	}

	// 3)����--����λ
	for (int i = 0; i < AK_SEAT_ID_NU; i++) 
	{
		//����
		if (bGoodsCardSwtich == 1 /*&& GoodChairId[i] == i*/)
		{
			memcpy(m_seatCard[i], GoodCardBuff[0], MAX_HAND_CARD_COUNT);
		}
		else
		{
			memcpy(m_seatCard[i], bRandCard + len, MAX_HAND_CARD_COUNT);
			len += MAX_HAND_CARD_COUNT;
		}	

		// ����
		m_seatCardType[i] = (CT_BULL_TYPE)m_GameLogic->GetCardTypeAndData(
			m_seatCard[i], MAX_HAND_CARD_COUNT, m_seatGroupCard[i]);
	}

	// ����ʣ��ʱ��
	m_betRemainTime = AK_START_TIME;

}



// ���ͱ��� 
static int AK_GameGetCardTimes(int card_type)
{
	int card_bet=1;

	if(card_type <= CT_BULL_SIX)        // ţ����ţһ,��ţ
		card_bet = 1;
	else if(card_type <= CT_BULL_NINE)  // ţ�ŵ�ţ��
		card_bet = 2;	
	else if(card_type == CT_BULL_BULL)  // ţţ
		card_bet = 3;
	else if(card_type == CT_BOMB)       // ��ը
		card_bet = 4;
	else if(card_type == CT_FIVE_H_BULL) // �廨ţ
		card_bet = 5;
	else if(card_type == CT_FIVE_S_BULL) // ��Сţ
		card_bet = 8;

	return card_bet;
}


class AK_GamePlayerBetSortFunc
{
	public:
		AK_GamePlayerBetSortFunc(int seat_id)
		{
			m_seatId = seat_id;
		}

	public:
		int operator() (AllKillPlayer* l,AllKillPlayer* r)  //����������
		{
			return l->getSeatBet(m_seatId) > r->getSeatBet(m_seatId);
		}

	public:
		int m_seatId;
};

//=================AllKillGame=====================

AllKillGame::AllKillGame()
{
	m_role=NULL;
	m_server=NULL;
	m_gameStatus=AK_Ready;
	m_evLoop=NULL;
	m_lotteryTotalMoney=0;

	//������ׯ����
	m_role_nu = 0;  

	//�м�λ�����
	memset(m_playerSeat, 0, sizeof(m_playerSeat));  
}



AllKillGame::~AllKillGame()
{


}


// �ر����˳�
int AllKillGame::shutDown()
{
	ev_timer_stop(m_evLoop,&m_evReadyTimer);
	ev_timer_stop(m_evLoop,&m_evStartTimer);
	ev_timer_stop(m_evLoop,&m_evEndGameTimer);
	ev_timer_stop(m_evLoop,&m_evUpdateTimer);
	ev_timer_stop(m_evLoop,&m_evChangeRoleTimer);

	//printf("m_offlinePlayers.size()=%d,m_loginPlayers.size()=%d\n",(int)m_offlinePlayers.size(),(int)m_loginPlayers.size());

	for(std::map<int,AllKillPlayer*>::iterator iter=m_offlinePlayers.begin();iter!=m_offlinePlayers.end();++iter)
	{
		AllKillPlayer* player=iter->second;
		assert(player);
		delete player;
	}

	for(std::map<int,AllKillPlayer*>::iterator iter=m_loginPlayers.begin();iter!=m_loginPlayers.end();++iter)
	{
		AllKillPlayer* player=iter->second;
		//	printf("uid=%d,player_addr=%ld\n",iter->first,(long)iter->second);
		assert(player);

		delete player;
	}

	m_loginPlayers.clear();
	m_betPlayers.clear();

	m_askRoleList.clear();

	if (m_role != NULL)
	{
		m_role = NULL;
	}
	m_server = NULL;

	return 0;
}



// ��(venue)��ص�����
int AllKillGame::configGame(Json::Value& value)
{
	m_gameConfig.m_vid             =value["venue"]["vid"].asInt();
	m_gameConfig.m_baseMoney       = 1; //value["venue"]["base_money"].asInt();  ///��ע����û���ˣ�������ע�����ǻ�ע�ı����ˣ�
	m_gameConfig.m_roleAnotherCard = value["venue"]["role_another_radio"].asInt();	
	m_gameConfig.m_sysFee          = value["venue"]["sys_fee"].asDouble();        // ϵͳ��ˮ	
	m_gameConfig.m_winRottleFee    = value["venue"]["win_rottle_fee"].asDouble(); // ӮǮ��ˮ
	m_gameConfig.m_rottleMinOpenMoney=value["venue"]["rottle_min_open_money"].asInt();

	m_gameConfig.m_askRoleMinMoney = value["venue"]["ask_role_min"].asInt();
	m_gameConfig.m_unRoleLimitMoney= value["venue"]["un_role_limit"].asInt();

	m_gameConfig.m_gameHistoryNum  =value["venue"]["game_history_nu"].asInt();
	m_gameConfig.m_enterRoomChat   = value["venue"]["enter_chat"].asString();	  // ��ӭ��

	// ���Ϳ������ر���
	m_gameConfig.m_rottleBombRadio      =value["venue"]["rottle_radio"]["ct_bomb"].asDouble();
	m_gameConfig.m_rottleFiveHBullRadio =value["venue"]["rottle_radio"]["ct_five_h_bull"].asDouble();
	m_gameConfig.m_rottleFiveSBullRadio =value["venue"]["rottle_radio"]["ct_five_s_bull"].asDouble();

	// ��������
	m_gameConfig.m_sitMinMoney =value["venue"]["sit_down"]["min_money"].asInt();
	m_gameConfig.m_sitVLevel   =value["venue"]["sit_down"]["min_vlevel"].asInt();

	// sys_roleϵͳׯ��
	m_gameConfig.m_sysRoleName     = value["venue"]["sys_role"]["name"].asString();
	m_gameConfig.m_sysRoleAvatar   = value["venue"]["sys_role"]["avatar"].asString();
	m_gameConfig.m_sysRoleUid      = value["venue"]["sys_role"]["uid"].asInt();
	m_gameConfig.m_sysRoleMoney    = value["venue"]["sys_role"]["money"].asInt();
	m_gameConfig.m_sysRoleSex      = value["venue"]["sys_role"]["sex"].asInt();

	// chat_role����ׯ��
	m_gameConfig.m_chatRoleName  = value["venue"]["chat_role"]["name"].asString();
	m_gameConfig.m_chatRoleAvatar= value["venue"]["chat_role"]["avatar"].asString();
	m_gameConfig.m_chatRoleUid   = value["venue"]["chat_role"]["uid"].asInt();
	m_gameConfig.m_chatRoleSex   = value["venue"]["chat_role"]["sex"].asInt();


	//�񽱵�һ�˳�ʼΪϵͳ���
	m_lotteryFirstReward.m_uid    = m_gameConfig.m_sysRoleUid;
	m_lotteryFirstReward.m_name   = m_gameConfig.m_sysRoleName;
	m_lotteryFirstReward.m_avatar = m_gameConfig.m_sysRoleAvatar;
	m_lotteryFirstReward.m_sex    = m_gameConfig.m_sysRoleSex;
	m_lotteryFirstReward.m_rewardMoney = 0;
	m_lotteryFirstReward.m_rewardTime = time(NULL);

	return 0;
}



// ===>>>��Ϸ����
int AllKillGame::start(AllKillServer* server,struct ev_loop* loop)
{
	m_gameStatus = AK_Ready;  //��ʼΪ��׼��״̬��
	m_server = server;
	m_evLoop = loop;

	// ׼��
	m_evReadyTimer.data = this;
	ev_timer_init(&m_evReadyTimer,AllKillGame::onReadyTimer, 0, AK_READY_TIME);

	// ��ʼ
	m_evStartTimer.data = this;
	ev_timer_init(&m_evStartTimer,AllKillGame::onStartTimer, 0, AK_START_TIME);

	// ����
	m_evEndGameTimer.data = this;
	ev_timer_init(&m_evEndGameTimer,AllKillGame::onEndGameTimer, 0, AK_END_GAME_TIME);

	// ����
	m_evUpdateTimer.data = this;
	ev_timer_init(&m_evUpdateTimer,AllKillGame::onUpdateGameTimer, 0, AK_UPDATE_TIME);

	// ��ׯ(��Ϸ����ǰ3����л�ׯ)
	m_evChangeRoleTimer.data = this;
	ev_timer_init(&m_evChangeRoleTimer,AllKillGame::onChangeRoleTimer,0, AK_END_GAME_TIME - AK_SET_NEXT_ROLE_BEFORE_TIME);


	ev_timer_again(m_evLoop,&m_evReadyTimer);   //����ready timer
	handleGameReady();

	return 0;
}


// ��Ϸ׼��
void AllKillGame::handleGameReady()
{
	// ���ÿ������
	m_lotteryResult.reset();          

	 // ����ׯ����Ϸ���
	m_roleGameResult.reset();        

	// ������λ��Ϸ���
	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		m_seatGameResult[i].reset();
	}

	// ���öԾ���Ϣ
	m_gameBetting.reset();        

	// ������ཱ���õ����
	m_mostRewardPlayer = NULL;    

	// ������ҵ�λ����ע��Ϣ
	for (std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		 iter!=m_betPlayers.end(); ++iter)
	{
		iter->second->resetSeatBet();
	}

	// �����ע����б�
	m_betPlayers.clear(); 

	// ���offline�б��е����
	for (std::map<int, AllKillPlayer*>::iterator iter = m_offlinePlayers.begin();
		 iter != m_offlinePlayers.end(); ++iter)
	{
		delete iter->second;
	}
	m_offlinePlayers.clear();


	//ׯ���ٴ����Ƹ���
	int radio = m_gameConfig.m_roleAnotherCard;  
	RedisClient* cache_rc = m_server->getCacheRedis();
	if (cache_rc != NULL)
	{
		int ret = cache_rc->command("hgetall ak_conf");
		if (ret >= 0)
		{
			if (cache_rc->is_array_return_ok() >= 0)
			{
				radio = cache_rc->get_value_as_int("role_another_radio");
			}
		}
	}
	
	// ���ƣ�ϵͳׯ�Ҹ��ݱ������ɶ������ƣ�
	m_gameBetting.deal(NULL == m_role, radio);

	// ����ׯ��
	setNextRole();  

	// �㲥����Ϸ׼����
	broadcastGameReady(NULL);
}


int AllKillGame::setNextRole()
{
	int change_role = false;
	if (m_role != NULL)
	{
		if (m_role->getIsUnRole())  // ��ׯ��
		{
			m_role      = NULL;
			change_role = true;
		}
	}

	if (m_role != NULL)   //���ׯ�ҵĽ�ң������Ļ���ׯ����
	{
		int money = m_role->getMoney();
		if (money < m_gameConfig.m_unRoleLimitMoney)
		{
			xt_log.warn("role(%d) money(%d) not enough, unrole it\n",m_role->getUid(), money);
			m_role = NULL;
			change_role = true;
		}
	}

	if (NULL == m_role)
	{
		while (m_askRoleList.size() >0 )
		{
			AllKillPlayer* player = m_askRoleList[0];
			if(player->updateMoney() < m_gameConfig.m_askRoleMinMoney)
			{
				// �����ׯ�б��У�С����ׯ���޵����
				m_askRoleList.erase(m_askRoleList.begin());
				continue;
			}

			player->setUnRole(false);   //������ׯ���
			m_role = player;
			m_askRoleList.erase(m_askRoleList.begin());
			change_role = true;
			break;
		}
	}

	if (m_role != NULL)
	{
		m_role->setTimeLock(60);   //??
	}

	return change_role;
}


// �㲥����Ϸ׼����
void AllKillGame::broadcastGameReady(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"]          = AK_GAME_READY_SB;
	packet.val["rottle_money"] = m_lotteryTotalMoney;  //���ؽ��

	formatRole(&packet);         //��ǰׯ����Ϣ
	formatAskRoleList(&packet);  //��ǰ������ׯ�б�

	packet.end();
	m_server->broadcast(player,packet.tostring());


	xt_log.debug("Func[%s] �㲥��Ϸ׼�� [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

}

// ��׼��ʱ�䡱��� 
void AllKillGame::onReadyTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->readyTimer();
}


//��׼������������Ϸ���롰��ʼ״̬��
void AllKillGame::readyTimer()
{
	ev_timer_stop(m_evLoop,&m_evReadyTimer);
	m_gameStatus = AK_Start;

	handleGameStart();  //�㲥��Ϸ��ʼ

	ev_timer_again(m_evLoop, &m_evStartTimer);
	ev_timer_again(m_evLoop, &m_evUpdateTimer);

}

// ����ʼ
void AllKillGame::handleGameStart()
{
	m_role_nu++;      //�ۼƵ�ׯ����
	broadcastGameStart(NULL);
}

// �㲥��Ϸ��ʼ
void AllKillGame::broadcastGameStart(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"]          = AK_GAME_START_SB;
	packet.val["remain_time"]  = m_gameBetting.m_betRemainTime;  //�������ʱ��
	packet.val["rottle_money"] = m_lotteryTotalMoney;             //���ؽ��

	// ��ׯ�ҷ�һ����
	packet.val["role_info"]["card"].append(m_gameBetting.m_roleCard[0]);

	// ��4��λ�÷�һ����
	for (int k = 0; k < AK_SEAT_ID_NU; k++)
	{
		packet.val["seat_info"][k]["seat_id"] = k + AK_SEAT_ID_START;  // ��λ��1��ʼ
		packet.val["seat_info"][k]["card"].append(m_gameBetting.m_seatCard[k][0]);
	}

	packet.end();
	m_server->broadcast(player, packet.tostring());

	xt_log.debug("Func[%s] �㲥��Ϸ��ʼ [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
}


//����ʼʱ�䡱��ϣ���Ϸ�������״̬
void AllKillGame::onStartTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game = (AllKillGame*)w->data;
	game->startTimer();
}


// startʱ�����
void AllKillGame::startTimer()
{
	ev_timer_stop(m_evLoop, &m_evStartTimer);
    ev_timer_stop(m_evLoop, &m_evUpdateTimer);  //ֹͣ����timer

	m_gameStatus = AK_EndGame;   //��Ϸ����
	handleGameEnd();

	if(m_lotteryResult.m_hasOpenLottery) 
	{
		// ���п����������ӿ���ʱ��(AK_LOTTERY_OPEN_TIME5��)
		ev_timer_set(&m_evEndGameTimer, 0, AK_END_GAME_TIME + AK_LOTTERY_OPEN_TIME);
		ev_timer_again(m_evLoop ,&m_evEndGameTimer);

		// ��ׯʱ��˳��
		ev_timer_set(&m_evChangeRoleTimer,0,AK_END_GAME_TIME + AK_LOTTERY_OPEN_TIME - AK_SET_NEXT_ROLE_BEFORE_TIME);
		ev_timer_again(m_evLoop, &m_evChangeRoleTimer);
	}
	else 
	{
		ev_timer_set(&m_evEndGameTimer,0, AK_END_GAME_TIME);
		ev_timer_again(m_evLoop,&m_evEndGameTimer);

		ev_timer_set(&m_evChangeRoleTimer, 0, AK_END_GAME_TIME - AK_SET_NEXT_ROLE_BEFORE_TIME);
		ev_timer_again(m_evLoop, &m_evChangeRoleTimer);
	}
}


//��Ϸ����
void AllKillGame::handleGameEnd()
{
	m_gameBetting.m_endGameTime = time(NULL);

	handlePlayerBetResult();   // ��ҶԾֽ��

	handleMoneyResult();       // ������Ӯ

	handleLotteryResult();     // ������

	sendBetPlayerResult();     // *������ҶԾֽ��

	broadcastGameUpdate(NULL); // �㲥��Ϸ����
	broadcastGameEnd(NULL);    // �㲥��Ϸ����

	sendGameInfoToSpeaker();   // ������Ϣ����������߻�����Ϣ��


	saveGameResultToSql();     // �������

}


// ��ҶԾֽ��
void AllKillGame::handlePlayerBetResult()
{
	/*  card info */
	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		//��ׯ�ұ���
		if (m_gameBetting.m_GameLogic->CompareCard(
			m_gameBetting.m_roleGroupCard, 
			m_gameBetting.m_seatGroupCard[i], MAX_HAND_CARD_COUNT, 0, 0))
		{
			// ׯ��Ӯ����λ��
			m_seatGameResult[i].m_isWin = 0;
			// Ӯ�����ͱ���
			m_seatGameResult[i].m_betTime = AK_GameGetCardTimes(m_gameBetting.m_roleCardType);
		}
		else 
		{
			//ׯ���䣬��λӮ
			m_seatGameResult[i].m_isWin = 1;
			// Ӯ�����ͱ���
			m_seatGameResult[i].m_betTime = AK_GameGetCardTimes(m_gameBetting.m_seatCardType[i]);
		}

		// ��λ����
		m_seatGameResult[i].m_cardType = m_gameBetting.m_seatCardType[i];
	
		// ��λ����ע��
		m_seatGameResult[i].m_totalBetNu = m_gameBetting.m_seatBet[i];

	}

	/* player bet list */
	for(std::map<int,AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		iter != m_betPlayers.end(); ++iter)
	{
		AllKillPlayer* player = iter->second;

		for(int i = 0; i < AK_SEAT_ID_NU; i++)
		{
			int player_bet_nu = player->getSeatBet(i + AK_SEAT_ID_START);

			// ��λ���
			if(player_bet_nu > 0)
			{
				m_seatGameResult[i].m_betPlayerList.push_back(player);
			}
		}
	}

	/* sort player bet */
	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		//����ע������ǰ4λ���
		std::sort(m_seatGameResult[i].m_betPlayerList.begin(),
			m_seatGameResult[i].m_betPlayerList.end(),
			AK_GamePlayerBetSortFunc(i + AK_SEAT_ID_START));
	}


	/* handle role result*/
	m_roleGameResult.m_cardType = m_gameBetting.m_roleCardType;                 //ׯ������
	m_roleGameResult.m_betTime  = AK_GameGetCardTimes(m_roleGameResult.m_cardType); //ׯ�����ͱ���


	// ������Ϸ���
	m_gameResultList.push_back(GameResultInfo(
		m_seatGameResult[0].m_isWin,
		m_seatGameResult[1].m_isWin,
		m_seatGameResult[2].m_isWin,
		m_seatGameResult[3].m_isWin));

	if (m_gameResultList.size() > (unsigned int) m_gameConfig.m_gameHistoryNum)  
	{
		m_gameResultList.pop_front();
	}
}

// ������Ӯ
void AllKillGame::handleMoneyResult()
{
	/* handle player bet info */
	for (std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		 iter != m_betPlayers.end(); ++iter)
	{
		AllKillPlayer* player = iter->second;
		player->updateMoney();   // �ȸ����½��

		for (int i = 0; i < AK_SEAT_ID_NU; i++)
		{
			int bet_nu = player->getSeatBet(i + AK_SEAT_ID_START);

			/* add player already bet money �Ȱ���ע��Ǯ�û�ȥ*/
			player->incMoney(bet_nu * m_gameConfig.m_baseMoney);

			if (m_seatGameResult[i].m_isWin)
			{
				int win_money = bet_nu * m_gameConfig.m_baseMoney * m_seatGameResult[i].m_betTime;
				/* add bet result */
				player->addBetMoneyResult(win_money);
			}
			else 
			{
				int lose_money = bet_nu * m_gameConfig.m_baseMoney * m_roleGameResult.m_betTime;

				player->decBetMoneyResult(lose_money);
			}
		}
	}


	/* handle player lose �����м����*/
	for(std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		iter != m_betPlayers.end(); ++iter)
	{
		AllKillPlayer* player = iter->second;
		int bet_money = player->getBetMoneyResult();

		if (bet_money > 0)
		{
			// ��¼ׯ�����
			m_roleGameResult.m_roleLostMoney += bet_money;
			player->m_winCount++;   //�ۼ���Ӯ����
			continue;
		}

		player->m_winCount = 0;     //��Ӯ������0

		// �����ǡ�ׯ��Ӯ��
		m_roleGameResult.m_roleWinMoney += -bet_money;
		int player_money = player->getMoney();
		if (player_money < 0)
		{
			xt_log.error("player(%d) money(%d) is negative\n",player->getUid(),player_money);
			player_money=0;
		}

		//��Ҳ����䣬���
		if(player_money < -bet_money)
		{
			bet_money = -player_money;
		}

		player->setBetMoneyResult(bet_money);  //������ҶԾֽ��

		m_roleGameResult.m_roleRealWinMoney += -bet_money;  //ׯ��ʵ��Ӯ

		player->decMoney(-bet_money);   //�۳�������Ǯ
		m_server->sendBetFlow(m_gameConfig.m_vid, player->getUid(), bet_money,player->getMoney());

		//++�м���
		m_server->savePlayerBet(m_gameConfig.m_vid, 
			player->getUid(), 
			player->getBetMoneyResult(),
			0);
	}


	/* handle role win ׯ��Ӯ*/
	float role_pay_radio = 1.0f;   // ׯ�Ҳ�����ʱ��������������м�
	int role_result_money = m_roleGameResult.m_roleRealWinMoney - m_roleGameResult.m_roleLostMoney;//(ׯ�������Ӯ)

	m_roleGameResult.m_roleResultBetMoney = role_result_money;


	if (m_role != NULL)   //��ׯ��
	{
		m_role->updateMoney();
		int role_money = m_role->getMoney();

		if (role_result_money > 0)
		{
			//ׯ��Ӯ
			m_role->m_winCount++;
			int rottle_money = floor(role_result_money * m_gameConfig.m_winRottleFee); //���س�ˮ
			m_lotteryTotalMoney += rottle_money;

			xt_log.info("Func[%d] role money %d, add role money %d,rottle_money %d,real_money %d \n",
				__FUNCTION__, role_money, role_result_money, rottle_money,role_result_money - rottle_money);

			int sys_fee = floor(role_result_money * m_gameConfig.m_sysFee);  //ϵͳ��ˮ

			m_role->incMoney(role_result_money - rottle_money - sys_fee);  //ׯ��ʵ��Ӯ��

			//ׯ�ҶԾ���ˮ
			m_server->sendBetFlow(m_gameConfig.m_vid, m_role->getUid(),role_result_money-rottle_money-sys_fee,m_role->getMoney());

		}
		else 
		{   //ׯ����
            m_role->m_winCount = 0;
			// 1)����
			if (role_money >= -role_result_money)
			{
				m_role->decMoney(-role_result_money);

				m_server->sendBetFlow(
					m_gameConfig.m_vid, 
					m_role->getUid(), 
					role_result_money, 
					m_role->getMoney()
					);
				xt_log.info("Func[%s] role money %d, dec role money %d,rottle_money %d,real_money %d \n",
					__FUNCTION__, role_money, role_result_money);
			}
			else 
			{   //2)�����䣬���(�мҰ�������)
				m_role->decMoney(role_money);

				m_server->sendBetFlow(m_gameConfig.m_vid, m_role->getUid(), -role_money, m_role->getMoney());
				xt_log.info("role money %d, dec role money %d,rottle_money %d,real_money %d \n",
					role_money,role_result_money);

				role_pay_radio = float(role_money) / float(-role_result_money);  //֧������
			}
		}

		//xt_log.info("role_money is %d \n",m_role->updateMoney());
	}
	else 
	{ //��ׯ��
		/*
		   if(role_result_money>0)
		   {
		   int rottle_money=floor(role_result_money*m_gameConfig.m_winRottleFee);
		   m_lotteryTotalMoney+=rottle_money;
		   }
		   */
	}


	// ���Ӯ��
	m_mostRewardPlayer = NULL;

	/* handle player win �м�Ӯ*/
	for (std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
			iter!=m_betPlayers.end(); ++iter)
	{
		AllKillPlayer* player = iter->second;
		int bet_money = player->getBetMoneyResult();
		if (bet_money < 0)
		{
			continue;
		}

		int result_money = floor(bet_money * role_pay_radio);
		player->setBetMoneyResult(result_money);

		int rottle_money = floor(result_money * m_gameConfig.m_winRottleFee);  // ���س�ˮ����Ӯ�ң�
		m_lotteryTotalMoney += rottle_money;

		int sys_fee = floor(result_money * m_gameConfig.m_sysFee);   //ϵͳ��ˮ����Ӯ�ң�

		player->incMoney(result_money - rottle_money - sys_fee);

		m_server->sendBetFlow(m_gameConfig.m_vid, player->getUid(), result_money-rottle_money-sys_fee, player->getMoney());
		
		//++�м�Ӯ
		m_server->savePlayerBet(m_gameConfig.m_vid, 
			player->getUid(), 
			player->getBetMoneyResult(),
			1);

		if ( NULL == m_mostRewardPlayer)
		{
			m_mostRewardPlayer = player;
		}
		else 
		{
			if (result_money > m_mostRewardPlayer->getBetMoneyResult())
			{
				m_mostRewardPlayer = player;
			}
		}

	}
}


// ���������
void AllKillGame::handleLotteryResult()
{
	m_lotteryResult.m_hasOpenLottery = false;

	if (m_lotteryTotalMoney < m_gameConfig.m_rottleMinOpenMoney)  //���ؿ�������
	{
		return ;
	}

	/*��������*/
	int card_types[] = {CT_BOMB, CT_FIVE_H_BULL, CT_FIVE_S_BULL};

	for(unsigned int i = 0; i < sizeof(card_types) / sizeof(int); i++)
	{
		int num = getCardTypeNu(card_types[i]);
		if (0 == num)
		{
			continue;
		}

		m_lotteryResult.m_hasOpenLottery = true;           // ������ָ�����ͣ�����Կ���
		handleRottleResultByCardType(card_types[i], num); // ���հ�������Ϳ���
	}


	// ���㿪��,������ׯ��
	if (m_lotteryResult.m_hasOpenLottery || m_role != NULL)
	{
		m_lotteryFirstReward.m_rewardMoney = 0;
	}
	else 
	{
		return ;
	}

	/* handle role lottery reward*/
	if (m_role != NULL)
	{
		m_role->incMoney(m_lotteryResult.m_roleLotteryMoney);

		m_server->sendLotteryFlow(m_gameConfig.m_vid,m_role->getUid(), 
			m_lotteryResult.m_roleLotteryMoney, 
			m_role->getMoney());
		
		// ������߽�������
		m_lotteryFirstReward.setIfMostLotteryReward(m_lotteryResult.m_roleLotteryMoney,
			m_lotteryResult.m_cardType,
			m_role);
	}


	/* handle seat lottery reward */
	for (int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		if (0 == m_lotteryResult.m_seatLotteryMoney[i])
		{
			continue;
		}

		for(std::vector<AllKillPlayer*>::iterator iter = m_seatGameResult[i].m_betPlayerList.begin();
			iter!=m_seatGameResult[i].m_betPlayerList.end(); ++iter)
		{
			AllKillPlayer* player = *iter;

			int bet_nu = player->getSeatBet(i+AK_SEAT_ID_START);
			assert(bet_nu>0);

			// �мҽ��𣬰���ע������
			int rottle_money = floor(m_lotteryResult.m_seatLotteryMoney[i] * (float)bet_nu / (float)m_gameBetting.m_seatBet[i]);

			player->incMoney(rottle_money);
			m_lotteryFirstReward.setIfMostLotteryReward(rottle_money, m_lotteryResult.m_cardType, player);
			m_server->sendLotteryFlow(m_gameConfig.m_vid,player->getUid(), rottle_money, player->getMoney());
		}
	}

	//�����һ��
    addLotteryFirstReward(m_lotteryFirstReward);
}

// ������ҶԾֽ��
void AllKillGame::sendBetPlayerResult()
{
	//�мҵ���Ӯ
	for (std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		iter!=m_betPlayers.end(); ++iter)
	{		
		Jpacket p_packet;
		p_packet.val["cmd"] = AK_PLAYER_BET_REWARD_SU;
		p_packet.val["reward"]=iter->second->getBetMoneyResult();  // Ӯ����Ľ�Ǯ
		p_packet.val["money"]=iter->second->getMoney();            // ��ҵĽ��
		p_packet.end();
		m_server->unicast(iter->second, p_packet.tostring());
	}

	//ׯ�ҵ���Ӯ
	if (m_role)
	{
		Jpacket p_packet;
		p_packet.val["cmd"] = AK_PLAYER_BET_REWARD_SU;

		p_packet.val["reward"]=m_roleGameResult.m_roleResultBetMoney;
		p_packet.val["money"]=m_role->getMoney();

		p_packet.end();

		m_server->unicast(m_role,p_packet.tostring());
	}

}


// �㲥��Ϸ����
void AllKillGame::broadcastGameEnd(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"] = AK_GAME_END_SB;  // ��Ϸ�����㲥

	formatLotteryFirstReward(&packet);   // �н���һ�����
	formatGameResult(&packet);

	// ++���¶Ծ������Ϣ
	formatBetPlayerResult(&packet);

	packet.val["rottle_money"] = m_lotteryTotalMoney;   //���ؽ��
	packet.end();


	m_server->broadcast(player, packet.tostring());

	xt_log.debug("Func[%s] �㲥��Ϸ���� [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

}


//������Ϣ����������߻�����Ϣ��
void AllKillGame::sendGameInfoToSpeaker()
{
	if (m_lotteryResult.m_hasOpenLottery)
	{
		char buf[2048];
		int money_w = m_lotteryFirstReward.m_rewardMoney/10000;
		if (money_w >10)
		{
			sprintf(buf,"���˳����ؿ�������ϲ%s���ͷ����%d�򽱽�",m_lotteryFirstReward.m_name.c_str(),money_w);
			m_server->sendSpeaker(4006,0,"ϵͳ",buf);
		}
	}


	if (m_mostRewardPlayer != NULL)
	{
		char buf[2048];
		int result_money=m_mostRewardPlayer->getBetMoneyResult();
		int money_w=result_money/10000;
		if(money_w>500)
		{
			sprintf(buf,"%s�����˳��е�ʶ��Ⱥ��������עӮ��%d���ҳ��ض������ض���",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"ϵͳ",buf);
		}
		else if(money_w>100)
		{
			sprintf(buf,"%s�����˳����治��ɫ�������עӮ��%d�����ս�һ���ٻ�",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"ϵͳ",buf);

		}
		else if(money_w>50)
		{
			sprintf(buf,"%s�����˳��е�����ϸ��Ӯ��%d�����ȳ���ע",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"ϵͳ",buf);
		}
	}
}

//��Ϸ��־��ˮ���
void AllKillGame::saveGameResultToSql()
{
	return;   //û�б�allkill_log

	if (m_gameStatus != AK_EndGame)
	{
		xt_log.error("status error for saveGameResultToSql\n");
		return;
	}

	// ��������
	int online_people = m_loginPlayers.size();

	// ���ֶ�
	std::string table_info="( create_time, vid, online_people, bet_people,rottle_money,rottle_open_money,role_id, role_tcard, role_rcard, role_money, id1_people, id1_betnu, id1_tcard, id1_rcard, id1_iswin,id1_money, id2_people, id2_betnu, id2_tcard, id2_rcard, id2_iswin,id2_money, id3_people, id3_betnu, id3_tcard, id3_rcard,id3_iswin, id3_money, id4_people, id4_betnu, id4_tcard, id4_rcard,id4_iswin, id4_money)" ;

	// �������
	int rottle_open=0;
	if (m_lotteryResult.m_hasOpenLottery)
	{
		//�������
		rottle_open += m_lotteryResult.m_roleLotteryMoney;

		for(int i=0;i<AK_SEAT_ID_NU;i++)
		{
			rottle_open += m_lotteryResult.m_seatLotteryMoney[i];
		}
	}


	// ��ע����
	int bet_people = m_betPlayers.size();

	// ��ǰʱ��
	time_t time_now=time(NULL);

	std::string value = "";
	char buf[1024] = {0};

	sprintf(buf,"%ld,%d,%d,%d,%d,%d",
		time_now,
		m_gameConfig.m_vid,
		online_people,
		bet_people,
		m_lotteryTotalMoney,
		rottle_open
		);
	value+=buf;

	// ׯ��uid
	int role_id = m_gameConfig.m_sysRoleUid;
	if(m_role)
	{
		role_id = m_role->getUid();
	}

	sprintf(buf,"%d,%d,%d,%d",
		role_id,
		m_roleGameResult.m_cardType,
		m_roleGameResult.m_betTime,
		m_roleGameResult.m_roleResultBetMoney);

	value += std::string(",") + buf;

	for (int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		int bet_money = m_seatGameResult[i].m_totalBetNu * m_gameConfig.m_baseMoney;
		if (m_seatGameResult[i].m_isWin)
		{
			bet_money = bet_money * m_seatGameResult[i].m_betTime;
		}
		else 
		{
			bet_money = -bet_money * m_roleGameResult.m_betTime;
		}

		sprintf(buf,",%ld,%d,%d,%d,%d,%d",
			m_seatGameResult[i].m_betPlayerList.size(),
			m_seatGameResult[i].m_totalBetNu,
			m_seatGameResult[i].m_cardType,
			m_seatGameResult[i].m_betTime,
			m_seatGameResult[i].m_isWin,
			bet_money);

		value+=buf;
	}
	value = " value("+value+")";

	std::string sql=std::string("insert allkill_log") + table_info + value;

	//printf("sql:%s\n",sql.c_str());

	int ret=m_server->getSqlClient()->query(sql.c_str());

	if(ret!=0)
	{
		xt_log.error("Func[%s] sql=%s.\n", __FUNCTION__, sql);
	}
}


//������ʱ�䡱���
void AllKillGame::onEndGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->endGameTimer();
}

// ���ֽ����������¾֡���Ϸ׼����
void AllKillGame::endGameTimer()
{
	ev_timer_stop(m_evLoop, &m_evChangeRoleTimer);
	ev_timer_stop(m_evLoop, &m_evEndGameTimer);

	m_gameStatus = AK_Ready;

	handleGameReady();
	ev_timer_again(m_evLoop, &m_evReadyTimer);


	// ˢ�����������
	m_server->refreshPlayerNuToRedis();

}

//������ʱ�䡱���
void AllKillGame::onUpdateGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->updateGameTimer();
}

// ����ׯʱ�䡱���
void AllKillGame::onChangeRoleTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*) w->data;
	game->changeRoleTimer();
}


void AllKillGame::updateGameTimer()
{
	ev_timer_again(m_evLoop, &m_evUpdateTimer);
	handleGameUpdate();
}


void AllKillGame::handleGameUpdate()
{
	m_gameBetting.m_betRemainTime--;
	if (m_gameBetting.m_betRemainTime < 0)
	{
		m_gameBetting.m_betRemainTime = 0;
	}

	broadcastGameUpdate(NULL);
}

// �㲥��Ϸ����
void AllKillGame::broadcastGameUpdate(AllKillPlayer* player)
{
	bool showUpdateLog = false;
	Jpacket packet;
	packet.val["cmd"] = AK_GAME_UPDATE_SB;  //��Ϸ���¹㲥

	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		packet.val["seat_bet_info"][i]["seat_id"] = i + AK_SEAT_ID_START;      //λ����Ϣ
		packet.val["seat_bet_info"][i]["bet_nu"]  = m_gameBetting.m_seatBet[i];//ѹ��ע��

		// �˶�ʱ�䣬��λ����ע������б����º�����
		int j = 0;
		for(std::map<int,AllKillPlayer*>::iterator iter = m_betPlayers.begin();
			iter != m_betPlayers.end(); ++iter)
		{
			AllKillPlayer* tmpPlayer = iter->second;
			if (tmpPlayer != NULL)
			{
				int player_bet_nu = tmpPlayer->getTmpSeatBet(i + AK_SEAT_ID_START);

				if (player_bet_nu > 0)
				{
					showUpdateLog = true;
					packet.val["seat_bet_info"][i]["player_list"][j]["name"]   = tmpPlayer->getName();
					packet.val["seat_bet_info"][i]["player_list"][j]["uid"]    = tmpPlayer->getUid();
					packet.val["seat_bet_info"][i]["player_list"][j]["avatar"] = tmpPlayer->getAvatar();
					packet.val["seat_bet_info"][i]["player_list"][j]["sex"]    = tmpPlayer->getSex();
					packet.val["seat_bet_info"][i]["player_list"][j]["money"]  = tmpPlayer->getMoney();
					packet.val["seat_bet_info"][i]["player_list"][j]["bet_nu"] = player_bet_nu;
				}			
			}
		}
	}

	packet.val["remain_time"] = m_gameBetting.m_betRemainTime;  //�����ʱ��
	packet.end();
	m_server->broadcast(NULL, packet.tostring());


	if (showUpdateLog)
	{
		xt_log.debug("Func[%s] �㲥���� [%s]\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
	

}


// ������Ҷ���
AllKillPlayer* AllKillGame::getPlayer(int uid)
{
	std::map<int,AllKillPlayer*>::iterator iter= m_offlinePlayers.find(uid);
	if(iter!=m_offlinePlayers.end())
	{
		return iter->second;
	}

	iter=m_loginPlayers.find(uid);

	if(iter!=m_loginPlayers.end())
	{
		xt_log.error("player(%d) allready login\n",uid);
		return iter->second;
	}

	AllKillPlayer* player=new AllKillPlayer;
	player->setData(uid,m_server,this);

	m_offlinePlayers[uid]=player;

	return player;

}

// ��ҵ�¼��Ϸ
void AllKillGame::playerLogin(AllKillPlayer* player)
{
	assert(player != NULL);
	int uid = player->getUid();

	std::map<int,AllKillPlayer*>::iterator iter;

	iter = m_loginPlayers.find(uid);
	// �Ѿ���¼��ֱ�ӷ���
	if (iter != m_loginPlayers.end())
	{
		xt_log.error("player(%d) all ready login\n",uid);
		return;
	}
	
	iter= m_offlinePlayers.find(uid); 
	//���������ҵ���������������
	if(iter!=m_offlinePlayers.end())
	{
		m_loginPlayers[iter->first]=iter->second;
		m_offlinePlayers.erase(iter);
	}
	else 
	{
		xt_log.error("player find in offline\n");  //?
	}

	sendLoginSuccess(player);  //���е�¼�ɹ�
	sendGameInfo(player);      //������Ϸ��Ϣ

	//���л�ӭ��
	sendPlayerChat(player, m_gameConfig.m_enterRoomChat); 
}

// �м�ѹע
void AllKillGame::playerBet(AllKillPlayer* player,Jpacket& package)
{
	int seat_id = package.val["seat_id"].asInt();   // ѹעλ��
	int bet_nu  = package.val["bet_nu"].asInt();    // ѹעע��

	if (m_role == player)
	{
		xt_log.error("Fun[%s]ׯ�Ҳ�������ע",__FUNCTION__);
		sendBetError(player, AK_ROLE_BET_ERR, "error:role can not bet");
		return;
	}

	// ��Ϸ��δ��ʼ����עʧ��
	if (m_gameStatus != AK_Start)
	{
		sendBetError(player, AK_STATUS_ERR, "game status error");
		return;
	}

    // ѹעע��Ϊ0
	if (bet_nu <= 0)
	{
		sendBetError(player, AK_BET_NU_ERR, "bet_nu greater than 0");
		return;
	}

	// λ�÷Ƿ�
	if(seat_id < AK_SEAT_ID_START && seat_id > AK_SEAT_ID_END)
	{
		sendBetError(player, AK_SEAT_ID_ERR, "seat_id error should[1,4]");
		return;
	}

	int money = player->getMoney();

	if (money < m_gameConfig.m_baseMoney * bet_nu)
	{
		money = player->updateMoney();
	}

	// ��Ҳ���
	if (money < m_gameConfig.m_baseMoney * bet_nu)
	{
		sendBetError(player, AK_MONEY_NOT_ENOUGH, "money not enough");
		return ;
	}

	m_gameBetting.m_seatBet[seat_id - AK_SEAT_ID_START] += bet_nu; //�ۼ�λ�õ���ע��

	// ��ע��Ǯ
	player->decMoney(m_gameConfig.m_baseMoney * bet_nu);  

	// ��¼��ҵ�λ��ѹע
	player->addSeatBet(seat_id, bet_nu);   

	int uid = player->getUid();

	m_betPlayers[uid] = player;     // ���뵽�Ծ�����б�
	sendBetSuccess(player, seat_id, bet_nu); // ��ע�ɹ�Ӧ��

}

//������ׯ
void AllKillGame::playerAskRole(AllKillPlayer* player,Jpacket& package)
{
	xt_log.info("Func[%s] player(%d) ask_role\n",
		__FUNCTION__, player->getUid());

	int role_nu = m_askRoleList.size();

	for(int i = 0;i < role_nu; i++)
	{
		if(m_askRoleList[i] == player)
		{
			xt_log.warn("player(%d) �Ѿ�����ׯ�б���\n",player->getUid());
			sendAskRoleSuccess(player);
			return;
		}
	}

	if (m_role == player)
	{
		xt_log.warn("player(%d) alread role. �Ѿ���ׯ��,����������ׯ.\n",player->getUid());
		sendAskRoleSuccess(player);
		return;
	}

	int money = player->getMoney(); 
	if (money < m_gameConfig.m_askRoleMinMoney)  ///��ׯ����
	{
		money = player->updateMoney();
	}

	if (money < m_gameConfig.m_askRoleMinMoney)
	{
		xt_log.error("Func[%s] player(%d) ask role, money(%d) not fit\n",
			__FUNCTION__, player->getUid(),money);

		sendAskRoleError(player, AK_MONEY_NOT_ENOUGH, "ask role money not enough");
		return;
	}

	m_askRoleList.push_back(player);  //������ׯ�б���
	sendAskRoleSuccess(player);

}

// �����ׯ
void AllKillGame::playerUnRole(AllKillPlayer* player,Jpacket& package)
{
	if (m_role == player)  //ׯ������
	{
		if (AK_EndGame == m_gameStatus)
		{
			m_role = NULL;

			if (m_askRoleList.size() > 0)
			{
				AllKillPlayer* player = m_askRoleList[0]; // �ֵ���ׯ�б��һ����Ϊ��ǰׯ��
				player->setUnRole(false);                 // ��ׯ���

				m_askRoleList.erase(m_askRoleList.begin());
				m_role = player;
			}
			sendUnRoleSuccess(player);
			return;
		}

		m_role->setUnRole(true);
		sendUnRoleSuccess(player);

		return;
	}

	int role_nu=m_askRoleList.size();

	for(int i=0;i<role_nu;i++)
	{
		if(m_askRoleList[i]==player)
		{
			m_askRoleList.erase(m_askRoleList.begin()+i);
			sendUnRoleSuccess(player);
			return;
		}
	}
	sendUnRoleErr(player,AK_NOT_IN_ROLE_LIST,"");
}

// ��ҵǳ�
void AllKillGame::playerLogout(AllKillPlayer* player)
{
	if (NULL == player)
	{
		return;
	}

	//վ��
	playerStandUp(player);

	int uid=player->getUid();

	// ��Ϸ����
	if (AK_EndGame == m_gameStatus)
	{
		m_loginPlayers.erase(uid);

		//�������ׯ��,���ÿ�ׯ��
		if (player == m_role)
		{
			m_role=NULL;
		}
		sendPlayerLogoutSuccess(player);

		int role_nu = m_askRoleList.size();
		for (int i = 0; i < role_nu; i++)
		{
			//������ڡ���ׯ�б��С�
			if (m_askRoleList[i]==player)
			{
				m_askRoleList.erase(m_askRoleList.begin() + i);
				broadcastAskRoleChange(player);
				break;
			}
		}

		// ����Ծ�����б��У���������ߣ��´������
		if (m_betPlayers.find(uid) != m_betPlayers.end())
		{
			m_offlinePlayers[uid] = player;
		}
		else 
		{
			delete player;
		}

		return;
	}


	if (AK_Ready == m_gameStatus)
	{
		if (player == m_role)
		{
			//������ׯ���
			player->setUnRole(true);
			m_loginPlayers.erase(uid);
			m_offlinePlayers[uid]=player;
			sendPlayerLogoutSuccess(player);
		}
		else 
		{
			m_loginPlayers.erase(uid);
			sendPlayerLogoutSuccess(player);

			int role_nu = m_askRoleList.size();
			for( int i = 0; i < role_nu; i++)
			{
				if (m_askRoleList[i] == player)
				{
					m_askRoleList.erase(m_askRoleList.begin() + i);
					broadcastAskRoleChange(player);
					break;
				}
			}
			delete player;
		}
		return;
	}

	if (AK_Start == m_gameStatus)
	{
		if (m_betPlayers.find(uid) != m_betPlayers.end() 
			|| m_role == player)
		{
			m_loginPlayers.erase(uid);
			m_offlinePlayers[uid] = player;

			sendPlayerLogoutSuccess(player);

			if (m_role == player)
			{
				m_role->setUnRole(true);
			}
			else 
			{
				int role_nu = m_askRoleList.size();
				for(int i = 0; i < role_nu; i++)
				{
					if (m_askRoleList[i] == player)
					{
						m_askRoleList.erase(m_askRoleList.begin() + i);
						broadcastAskRoleChange(player);
						break;
					}
				}
			}
		}
		else 
		{
			m_loginPlayers.erase(uid);
			sendPlayerLogoutSuccess(player);

			int role_nu = m_askRoleList.size();
			for (int i = 0; i < role_nu; i++)
			{
				if (m_askRoleList[i] == player)
				{
					m_askRoleList.erase(m_askRoleList.begin() + i);
					broadcastAskRoleChange(player);
					break;
				}
			}
			delete player;
		}
	}
}

// ��ÿ����Ϸ����ǰ3��,������ׯ
void AllKillGame::changeRoleTimer()
{
	if(m_gameStatus != AK_EndGame)
	{
		xt_log.error("change role timer error not EndGameStatus.\n");
		return;
	}

	ev_timer_stop(m_evLoop, &m_evChangeRoleTimer);
	handleChangeRole();
}



// ����ׯ
void AllKillGame::handleChangeRole()
{
	int ret = setNextRole();

    // �л�ׯ 
	if (ret)
	{
		broadcastAskRoleChange(NULL);

		// ����������ׯ����
		m_role_nu = 0;

		// �мҳ�Ϊׯ��,���������λ������վ��	
		playerStandUp(m_role);
	}
}




// �����͸���
int AllKillGame::getCardTypeNu(int card_type)
{
	int ret = 0;
	if (m_roleGameResult.m_cardType == card_type)   // ׯ������
	{
		ret++;
	}

	for (int i = 0; i < AK_SEAT_ID_NU; i++)    // λ������
	{
		if (m_seatGameResult[i].m_cardType == card_type
			&& m_seatGameResult[i].m_totalBetNu > 0)
		{
			ret++;
		}
	}
	return ret;
}

// ���Ϳ�������
float AllKillGame::getCardTypeLotteryRadio(int card_type)
{
	float rottle_radio=0.0;

	if(CT_BULL_BULL == card_type)  //��ը
	{
		rottle_radio = m_gameConfig.m_rottleBombRadio;
	}
	else if (CT_FIVE_H_BULL == card_type)  //�廨ţ
	{
		rottle_radio = m_gameConfig.m_rottleFiveHBullRadio;
	}
	else if(CT_FIVE_S_BULL == card_type)  //��Сţ
	{
		rottle_radio = m_gameConfig.m_rottleFiveSBullRadio;
	}
	return rottle_radio;
}


// �����Ϳ�������ׯ�ң�����λ���п������ͣ����ܵý���
void AllKillGame::handleRottleResultByCardType(int card_type, int num)
{
	assert(num > 0);

	float rottle_radio  = getCardTypeLotteryRadio(card_type);
	int rottle_money    = m_lotteryTotalMoney * rottle_radio;
	m_lotteryTotalMoney -= rottle_money;

	rottle_money = rottle_money / num;      // ���λ�ó�����ͬ����

	m_lotteryResult.m_cardType = card_type; // ��ǰ�񽱵�����

	if (m_roleGameResult.m_cardType == card_type)
	{
		m_lotteryResult.m_roleLotteryMoney = rottle_money;
	}

	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		if(m_seatGameResult[i].m_cardType == card_type)
		{
			m_lotteryResult.m_seatLotteryMoney[i] = rottle_money;
		}
	}
}



// ��¼�ɹ�
void AllKillGame::sendLoginSuccess(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"] = AK_LOGIN_SU;
	packet.val["ret"] = 0;

	packet.end();

	m_server->unicast(player,packet.tostring());

	xt_log.debug("Func[%s] ��¼�ɹ�Ӧ�� [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

	/* broadcast  �㲥��¼�ɹ�*/
	packet.val["cmd"]    = AK_LOGIN_SUCC_SB;
	packet.val["uid"]    = player->getUid();
	packet.val["seatid"] = player->m_seatId;
	packet.val["name"]   = player->getName();
	packet.val["sex"]    = player->getSex();
	packet.val["avatar"] = player->getAvatar();
	packet.val["money"]  = player->getMoney();
	packet.end();

	m_server->broadcast(player, packet.tostring());
	xt_log.debug("Func[%s] �㲥��¼ [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());


}

// ������Ϸ�����䣩��Ϣ
void AllKillGame::sendGameInfo(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_GAME_INFO_SU;

	// ��Ʊ�н���һ�����
	formatLotteryFirstReward(&packet);

	// ��Ϸ��������Ϣ
	packet.val["config"]["bet_base"] = m_gameConfig.m_baseMoney; //��ע���
	packet.val["config"]["ask_role_min"] = m_gameConfig.m_askRoleMinMoney;//������ׯ����С���
	packet.val["config"]["un_role_limit"] = m_gameConfig.m_unRoleLimitMoney;  //��ׯ����С���

	packet.val["config"]["rottle_ct_bomb"] = m_gameConfig.m_rottleBombRadio;              //��ը�����н�����
	packet.val["config"]["rottle_ct_five_h_bull"] = m_gameConfig.m_rottleFiveHBullRadio;  //���廨ţ���н�����
	packet.val["config"]["rottle_ct_five_s_bull"] = m_gameConfig.m_rottleFiveSBullRadio;  //����Сţ���н�����

	packet.val["config"]["win_rottle_fee"] = m_gameConfig.m_winRottleFee;         //�����Ӯ��Ǯ��ˮ����������
	packet.val["config"]["rottle_min_open_money"] = m_gameConfig.m_rottleMinOpenMoney; //��С�������

	packet.val["config"]["sit_min_money"] = m_gameConfig.m_sitMinMoney;  //������ҵ���
	packet.val["config"]["sit_min_vlevel"] = m_gameConfig.m_sitVLevel;   //����vlevel�ȼ�����

	// ��ע��������
	int box_size = m_server->m_conf["bet_box_cfg"].size();
	for (int i = 0; i < box_size; i++)
	{
		packet.val["bet_box_cfg"][i]["box_type"]   = m_server->m_conf["bet_box_cfg"][i]["box_type"].asInt();
		packet.val["bet_box_cfg"][i]["bet_need"]   = m_server->m_conf["bet_box_cfg"][i]["bet_need"].asInt();
	}

	//�����ע�����б�
	std::vector<BetBox> vecBetBox = m_server->queryBetLottery(player->getVid(), player->getUid());
	for (unsigned int k = 0; k < vecBetBox.size(); k++)
	{
		packet.val["bet_lottery_daily"][k]["box_type"]      = vecBetBox[k].boxType;
		packet.val["bet_lottery_daily"][k]["lottery_money"] = vecBetBox[k].lotteryMoney;
		packet.val["bet_lottery_daily"][k]["get_flag"]      = vecBetBox[k].getFlag;
	}

	// ���ص�ǰ�Ľ��
	packet.val["rottle_money"] = m_lotteryTotalMoney;  
    
	//��ǰ�ȴ���ׯ�б�
	formatAskRoleList(&packet); 

	//��ǰׯ����Ϣ
	formatRole(&packet);         

	/* λ��id����עλ�ô�1��ʼ��*/ 
	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		packet.val["seats"][i]["seat_id"] = AK_SEAT_ID_START + i;  //ÿ��λ�ö�Ӧ��seat_id
	}

	// �������������Ϣ
	int kk = 0;
	for (std::map<int, AllKillPlayer*>::iterator iter = m_loginPlayers.begin();
		iter != m_loginPlayers.end(); ++iter)
	{
		AllKillPlayer* player = iter->second;
		packet.val["players"][kk]["uid"]    = player->getUid();  
		packet.val["players"][kk]["seatid"] = player->m_seatId;  
		packet.val["players"][kk]["name"]   = player->getName();  
		packet.val["players"][kk]["sex"]    = player->getSex();  
		packet.val["players"][kk]["avatar"] = player->getAvatar();  
		packet.val["players"][kk]["money"]  = player->getMoney(); 
		packet.val["players"][kk]["win_count"] = player->m_winCount; 

		kk++;
	}

	//��ʽͼ
	kk=0;
	for (std::deque<GameResultInfo>::iterator iter = m_gameResultList.begin();
		iter != m_gameResultList.end(); ++iter)
	{
		for (int j = 0; j < AK_SEAT_ID_NU; j++)
		{
			packet.val["game_history"][kk][j] = (*iter).m_seatid[j];
		}
		kk++;
	}


	// ��Ϸ��ǰ״̬(AK_Ready, AK_Start, AK_EndGame)
	packet.val["status"] = m_gameStatus;  

	if (AK_Ready == m_gameStatus)
	{
		//===׼��״̬(AK_Ready)===
		/*do nothing */
	}
	else if (AK_Start == m_gameStatus)   
	{
		//===��ʼ״̬(AK_Start)===
		for(int i = 0; i < AK_SEAT_ID_NU; i++)
		{
			packet.val["bet_info"][i]["seat_id"]     = i + AK_SEAT_ID_START;  //λ����Ϣ
			packet.val["bet_info"][i]["seat_bet_nu"] = m_gameBetting.m_seatBet[i]; //λ����עע��
			packet.val["bet_info"][i]["your_bet_nu"] = player->getSeatBet(i + AK_SEAT_ID_START); //����ע��ע��
		}

		packet.val["remain_time"] = m_gameBetting.m_betRemainTime;   //�������ʣ��ʱ��


		// ��ׯ�ҷ�һ����
		packet.val["role_info"]["card"].append(m_gameBetting.m_roleCard[0]);

		// ��4��λ�÷�һ����
		for (int k = 0; k < AK_SEAT_ID_NU; k++)
		{
			packet.val["seat_info"][k]["seat_id"] = k + AK_SEAT_ID_START;  // ��λ��1��ʼ
			packet.val["seat_info"][k]["card"].append(m_gameBetting.m_seatCard[k][0]);
		}

	}
	else if (AK_EndGame == m_gameStatus)   
	{
		//===����״̬(AK_EndGame)===
		formatGameResult(&packet);
	}



	packet.end();

	m_server->unicast(player,packet.tostring());
}


//��ׯ�б�ı�Ĺ㲥
void AllKillGame::broadcastAskRoleChange(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_ASK_ROLE_LIST_CHANGE_SB;
	formatAskRoleList(&packet); // ��ǰ��ׯ�б�
	formatRole(&packet);        // ��ǰׯ����Ϣ
	packet.end();
	m_server->broadcast(player, packet.tostring());

	
	xt_log.debug("Func[%s] �㲥��ׯ�б� [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
}










void AllKillGame::sendBetError(AllKillPlayer* player,int code,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]  = AK_PLAYER_BET_RESULT_SU;   // ���гɹ�ѹע���ݰ�
	packet.val["ret"]  = code;                      // 0:Ϊ�ɹ� ����:�������
	packet.val["desc"] = desc;                      // ��������
	packet.end();

	m_server->unicast(player, packet.tostring());
}


void AllKillGame::sendBetSuccess(AllKillPlayer* player,int seat_id, int bet_nu)
{
	Jpacket packet;

	packet.val["cmd"] = AK_PLAYER_BET_RESULT_SU;
	packet.val["ret"] = 0;

	packet.val["bet_info"]["seat_id"]     = seat_id; //λ����Ϣ
	packet.val["bet_info"]["seat_bet_nu"] = m_gameBetting.m_seatBet[seat_id-AK_SEAT_ID_START]; //��ע��
	packet.val["bet_info"]["your_bet_nu"] = bet_nu;  //�����ע��ע��
	packet.val["money"] = player->getMoney(); //��ҵĽ��

	packet.end();

	m_server->unicast(player,packet.tostring());


	xt_log.debug("Func[%s] ѹע�ɹ����� [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

	//++�㲥��ע
	//{
	//	Jpacket packet;

	//	packet.val["cmd"] = AK_PLAYER_BET_SB;

	//	packet.val["bet_info"]["seat_id"]     = seat_id; //λ����Ϣ
	//	packet.val["bet_info"]["seat_bet_nu"] = m_gameBetting.m_seatBet[seat_id - AK_SEAT_ID_START]; //��ע��
	//	packet.val["bet_info"]["uid"]         = player->getUid();
	//	packet.val["bet_info"]["bet_nu"]      = bet_nu;  //�����ע��ע��
	//	packet.val["money"]                   = player->getMoney(); //��ҵĽ��

	//	packet.end();

	//	m_server->broadcast(player,packet.tostring());
	//}
}



// ��ׯ�ɹ�
void AllKillGame::sendAskRoleSuccess(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_ASK_ROLE_RESULT_SU; // ��������ɹ����ݰ�
	packet.val["ret"] = 0;

	formatAskRoleList(&packet);
	formatRole(&packet);

	packet.end();

	/* unicast */
	m_server->unicast(player,packet.tostring());


	/* broadcast  */
	packet.val["cmd"] = AK_ASK_ROLE_LIST_CHANGE_SB;  //��ׯ�б�ı�Ĺ㲥
	packet.end();

	m_server->broadcast(player,packet.tostring());
}


// ��ׯʧ��
void AllKillGame::sendAskRoleError(AllKillPlayer* player,int code ,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]  = AK_ASK_ROLE_RESULT_SU;
	packet.val["ret"]  = code;
	packet.val["desc"] = desc;

	packet.end();
	m_server->unicast(player,packet.tostring());
}

// ��ׯ�ɹ�
void AllKillGame::sendUnRoleSuccess(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"]  = AK_ASK_UN_ROLE_RESULT_SU; 
	packet.val["ret"]  = 0;
	packet.val["desc"] = "";

	formatAskRoleList(&packet);
	formatRole(&packet);

	packet.end();
	/* unicast */
	m_server->unicast(player,packet.tostring());

	/* broadcast  */
	packet.val["cmd"] = AK_ASK_ROLE_LIST_CHANGE_SB; //��ׯ�б�ı�Ĺ㲥
	packet.end();

	m_server->broadcast(player, packet.tostring());
}


//��ׯ����
void AllKillGame::sendUnRoleErr(AllKillPlayer* player,int code,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]  = AK_ASK_UN_ROLE_RESULT_SU;
	packet.val["ret"]  = code;
	packet.val["desc"] = desc;
	packet.end();

	m_server->unicast(player,packet.tostring());
}

// �ǳ��ɹ�
void AllKillGame::sendPlayerLogoutSuccess(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_LOGOUT_SU;
	packet.val["ret"] = 0;
	packet.end();
	m_server->unicast(player, packet.tostring());

	/* broadcast �㲥�ǳ� */
	{
		Jpacket packetBroadcast;
		packetBroadcast.val["cmd"] = AK_LOGOUT_SB;   
		packetBroadcast.val["uid"] = player->getUid();
		packetBroadcast.val["type"] = 0;
		packetBroadcast.end();

		m_server->broadcast(player, packetBroadcast.tostring());
	}

}

// �㲥��ӭ�ʣ�������У�
void AllKillGame::sendPlayerChat(AllKillPlayer* player,const std::string& chat) 
{
	Jpacket packet;
	packet.val["cmd"]    = AK_CHAT_SB;
	packet.val["uid"]    = m_gameConfig.m_chatRoleUid;
	packet.val["name"]   = m_gameConfig.m_chatRoleName;
	packet.val["avatar"] = m_gameConfig.m_chatRoleAvatar;
	packet.val["sex"]    = m_gameConfig.m_chatRoleSex;
	packet.val["content"]= chat;
	packet.end();

	m_server->unicast(player,packet.tostring());
}



//��ǰׯ����Ϣ
void AllKillGame::formatRole(Jpacket* packet)
{
	if (m_role != NULL)
	{
		packet->val["role"]["name"]        = m_role->getName();
		packet->val["role"]["money"]       = m_role->getMoney();
		packet->val["role"]["avatar"]      = m_role->getAvatar();
		packet->val["role"]["uid"]         = m_role->getUid();
		packet->val["role"]["sex"]         = m_role->getSex();
		packet->val["role"]["next_unrole"] = m_role->getIsUnRole()? 1:0;   //�Ƿ�����ׯ״̬
		packet->val["role"]["role_nu"]     = m_role_nu;  //������ׯ����
		
	}
	else 
	{
		packet->val["role"]["name"]        = m_gameConfig.m_sysRoleName;
		packet->val["role"]["money"]       = m_gameConfig.m_sysRoleMoney;
		packet->val["role"]["avatar"]      = m_gameConfig.m_sysRoleAvatar;
		packet->val["role"]["uid"]         = m_gameConfig.m_sysRoleUid;
		packet->val["role"]["sex"]         = m_gameConfig.m_sysRoleSex;
		packet->val["role"]["next_unrole"] = 0;
		packet->val["role"]["role_nu"]     = m_role_nu;  //������ׯ����
	}

}

// ��ׯ�б�
void AllKillGame::formatAskRoleList(Jpacket* packet)
{
	int ask_role_nu = m_askRoleList.size();
	for(int i = 0; i < ask_role_nu; i++)
	{
		AllKillPlayer* player = m_askRoleList[i];  // ��ׯ�б�
		if (player != NULL)
		{
			packet->val["ask_roles"][i]["name"]   = player->getName();
			packet->val["ask_roles"][i]["uid"]    = player->getUid();
			packet->val["ask_roles"][i]["avatar"] = player->getAvatar();
			packet->val["ask_roles"][i]["money"]  = player->getMoney();
			packet->val["ask_roles"][i]["sex"]    = player->getSex();
		}
	}
}



// �н���һ�����
void AllKillGame::formatLotteryFirstReward(Jpacket* packet)
{
	packet->val["rottle_first_reward"]["name"]         = m_lotteryFirstReward.m_name;
	packet->val["rottle_first_reward"]["avatar"]       = m_lotteryFirstReward.m_avatar;
	packet->val["rottle_first_reward"]["sex"]          = m_lotteryFirstReward.m_sex;
	packet->val["rottle_first_reward"]["uid"]          = m_lotteryFirstReward.m_uid;
	packet->val["rottle_first_reward"]["reward_money"] = m_lotteryFirstReward.m_rewardMoney;  //��õĽ��
	packet->val["rottle_first_reward"]["reward_time"]  = m_lotteryFirstReward.m_rewardTime;   //��ʱ��
}

// �Ծ������Ϣ
void AllKillGame::formatBetPlayerResult(Jpacket* packet)
{
	if (NULL == packet)
	{
		return;
	}

	int i = 0;
	for(std::map<int,AllKillPlayer*>::iterator iter=m_betPlayers.begin();
		iter!=m_betPlayers.end(); ++iter)
	{

		AllKillPlayer* player = iter->second;
		if (player != NULL)
		{
			packet->val["bet_players"][i]["uid"]    = player->getUid();
			packet->val["bet_players"][i]["name"]   = player->getName();
			packet->val["bet_players"][i]["avatar"] = player->getAvatar();
			packet->val["bet_players"][i]["sex"]    = player->getSex();
			packet->val["bet_players"][i]["money"]  = player->getMoney();	
			packet->val["bet_players"][i]["win_count"]  = player->m_winCount;				

			//�����ע�����б�
			std::vector<BetBox> vecBetBox = m_server->queryBetLottery(player->getVid(), player->getUid());
			for (unsigned int k = 0; k < vecBetBox.size(); k++)
			{
				packet->val["bet_players"][i]["bet_lottery_daily"][k]["box_type"]      = vecBetBox[k].boxType;
				packet->val["bet_players"][i]["bet_lottery_daily"][k]["lottery_money"] = vecBetBox[k].lotteryMoney;
				packet->val["bet_players"][i]["bet_lottery_daily"][k]["get_flag"]      = vecBetBox[k].getFlag;
			}
		}
	}
}

// ��Ϸ���
void AllKillGame::formatGameResult(Jpacket* packet)
{
	int remain_time=0;

	// ���п������ӳ�5��
	if(m_lotteryResult.m_hasOpenLottery)
	{		
		remain_time = AK_END_GAME_TIME - (time(NULL) - m_gameBetting.m_endGameTime) + AK_LOTTERY_OPEN_TIME;
	}
	else 
	{
		remain_time = AK_END_GAME_TIME - (time(NULL) - m_gameBetting.m_endGameTime);
	}

	if (remain_time < 0)
	{
		remain_time = 0;
	}

	packet->val["remain_time"]     = remain_time;                               // ʣ��ʱ��(�뿪�ֵ�ʱ��)
	packet->val["has_open_rottle"] = m_lotteryResult.m_hasOpenLottery ? 1 : 0;  // �������

	int rottle_total_money = m_lotteryResult.m_roleLotteryMoney;

	for (int i = 0;i < AK_SEAT_ID_NU; i++)
	{
		rottle_total_money += m_lotteryResult.m_seatLotteryMoney[i];
	}

	packet->val["rottle_total_money"] = rottle_total_money;  // �����ܽ��


	/* format role info */
	for(unsigned int i = 1;i < MAX_HAND_CARD_COUNT; i++)
	{
		//ׯ�Һ�4����
		packet->val["role_info"]["card"].append(m_gameBetting.m_roleCard[i]);  //ׯ�Һ�4����
	}

	packet->val["role_info"]["rottle_money"] = m_lotteryResult.m_roleLotteryMoney;    // ׯ�һ�õĽ��ؽ��
	packet->val["role_info"]["card_type"]    = m_roleGameResult.m_cardType;           // ׯ�ҵ�����            
	packet->val["role_info"]["reward_money"] = m_roleGameResult.m_roleResultBetMoney; // ��õĽ��
	packet->val["role_info"]["bet_times"]    = m_roleGameResult.m_betTime;            // ����

	if (m_role != NULL)
	{
		packet->val["role_info"]["money"] = m_role->getMoney();    //ׯ�ҵĽ��
		packet->val["role_info"]["win_count"] = m_role->m_winCount;
	}
	else 
	{
		packet->val["role_info"]["money"] = m_gameConfig.m_sysRoleMoney;
		packet->val["role_info"]["win_count"] = 0;     //ϵͳׯ����Ӯ����Ϊ0
	}
	

	/* seat info */
	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		packet->val["seat_info"][i]["seat_id"]      = i + AK_SEAT_ID_START;
		packet->val["seat_info"][i]["rottle_money"] = m_lotteryResult.m_seatLotteryMoney[i];
		packet->val["seat_info"][i]["is_win"]       = m_seatGameResult[i].m_isWin;
		packet->val["seat_info"][i]["card_type"]    = m_seatGameResult[i].m_cardType;
		packet->val["seat_info"][i]["total_bet_nu"] = m_seatGameResult[i].m_totalBetNu;


		for(int j = 1; j < MAX_HAND_CARD_COUNT; j++)   
		{
			//��λ��4����
			packet->val["seat_info"][i]["card"].append(m_gameBetting.m_seatCard[i][j]);
		}

		//Ӯ������
		int betTime = m_roleGameResult.m_betTime;
		if (m_seatGameResult[i].m_isWin > 0)
		{			 
			betTime = m_seatGameResult[i].m_betTime;
		}
		packet->val["seat_info"][i]["bet_times"] = betTime;

		int size = m_seatGameResult[i].m_betPlayerList.size();

		for(int j = 0;j < size; j++)
		{
			AllKillPlayer* player = m_seatGameResult[i].m_betPlayerList[j];
			packet->val["seat_info"][i]["player_list"][j]["name"]   = player->getName();
			packet->val["seat_info"][i]["player_list"][j]["uid"]    = player->getUid();
			packet->val["seat_info"][i]["player_list"][j]["avatar"] = player->getAvatar();
			packet->val["seat_info"][i]["player_list"][j]["sex"]    = player->getSex();
			packet->val["seat_info"][i]["player_list"][j]["bet_nu"] = player->getSeatBet(i+AK_SEAT_ID_START);

			int seatWinMoney = player->getSeatBet(i + AK_SEAT_ID_START) * betTime;  //��λ������Ӯ��Ǯ��˰ǰ��
			if(m_seatGameResult[i].m_isWin <= 0)
			{	
				//��ʱתΪ����
				seatWinMoney = -seatWinMoney;
			}
			packet->val["seat_info"][i]["player_list"][j]["seat_win_money"] = seatWinMoney;			
		}
	}
}



//m_playerSeat[AK_SEAT_ID_NU]
int AllKillGame::doSitDown(AllKillPlayer* player, int seat_id)
{
	// �������ָ������λ�Ƿ�Ϊ����
	if (seat_id >= 0 && seat_id < AK_SEAT_ID_NU )
	{
		if (m_playerSeat[seat_id] == 0)
		{
			m_playerSeat[seat_id] = player->getUid();
			return seat_id;
		}
		else
		{
			xt_log.warn("Func[%s], seat_id[%d] not empty", __FUNCTION__, seat_id);
		}
	} 
	else 
	{
		xt_log.error("Func[%s], seat_id[%d] error", __FUNCTION__, seat_id);
	}
	
	return -1;
}

// ������£�ע�������λ�Ǵ�0��ʼ�ģ�
void AllKillGame::playerSitDown(AllKillPlayer* player, int seat_id)
{	
	int ret = -2;
	std::string desc="";

	//�ж�λ���Ƿ�Ϸ�
	if (seat_id >= 0 && seat_id < AK_SEAT_ID_NU)
	{
		// ׯ�Ҳ�������
		if (player != m_role)
		{
			//���֮ǰ�����µĻ�����ִ��վ��֧�ֻ�λ��	
			playerStandUp(player);

			//ִ������
			ret = doSitDown(player, seat_id);
		}
		else
		{
			ret = -3;
			desc="role can not sit down";
		}

	}
	else
	{
		ret = -2;
		desc="seat_id error";
	}

	
	if (ret < 0)
	{
		player->m_seatId = -1;  // ����ʧ�ܣ����ǿ�λ��
		desc="not empty seat";
	}
	else
	{
		player->m_seatId = ret;  // ����λ��
	}

	Jpacket packet;
	packet.val["cmd"]  = AK_SIT_DOWN_SU;
	if (ret >= 0)
	{
		packet.val["ret"]  = 0;
	}
	else 
	{
		packet.val["ret"]  = ret;
	}	

	packet.val["desc"]    = desc.c_str();
    packet.val["seat_id"] = player->m_seatId;
	packet.end();

	m_server->unicast(player,packet.tostring());

	xt_log.debug("Func[%s] ����Ӧ�� [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());


	// �ɹ����¹㲥
	if (ret >= 0)
	{
		Jpacket packet;

		packet.val["cmd"]     = AK_SIT_DOWN_SB;
		packet.val["uid"]     = player->getUid();
		packet.val["seat_id"] = player->m_seatId;
		packet.end();

		m_server->broadcast(player, packet.tostring());

		xt_log.debug("Func[%s] �㲥���� [%s]\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
}

// վ��
int AllKillGame::doStandUp(AllKillPlayer* player)
{
	int ret = -1;

	if (NULL == player)
	{
		return ret;
	}

	
	for (int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		if (m_playerSeat[i] == player->getUid())
		{
			m_playerSeat[i] = 0;
			ret = i;
		}
	}
	return ret;
}

//���������վ��Ӧ��
void AllKillGame::playerStandUp(AllKillPlayer* player, int seat_id)
{
	if (NULL == player)
	{
		return;
	}

	std::string desc = "";

	int ret = doStandUp(player);	
	if (ret < 0)
	{
		//վ��ʧ��(��û����)
		desc="you do not have to sit down";
	}

	Jpacket packet;
	packet.val["cmd"]  = AK_STAND_UP_SU;
	if (ret >= 0)
	{
		packet.val["ret"]  = 0;
	}
	else
	{
		packet.val["ret"]  = ret;
	}
	
	packet.val["desc"] = desc.c_str();
	packet.end();

	m_server->unicast(player,packet.tostring());

	xt_log.debug("Func[%s] վ��Ӧ�� [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
	

	// վ��ɹ��㲥
	if (ret >= 0)
	{
		Jpacket packet;
		packet.val["cmd"]     = AK_STAND_UP_SB;
		packet.val["uid"]     = player->getUid();
		packet.val["seat_id"] = ret;
		packet.end();

		m_server->broadcast(NULL, packet.tostring());

		xt_log.debug("Func[%s] �㲥վ�� [%s]\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
}


void AllKillGame::playerStandUp(AllKillPlayer* player)
{
	if (NULL == player)
	{
		return;
	}

	int seat_id = doStandUp(player);

	// վ��ɹ��㲥
	if (seat_id >= 0)
	{
		Jpacket packet;
		packet.val["cmd"]     = AK_STAND_UP_SB;
		packet.val["uid"]     = player->getUid();
		packet.val["seat_id"] = seat_id;
		packet.end();

		m_server->broadcast(NULL, packet.tostring());

		xt_log.debug("Func[%s] �㲥վ�� [%s]\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
}

// �����н���һ��
void AllKillGame::addLotteryFirstReward(LotteryFirstReward lotteryFirst)
{
	xt_log.info("Func[%s], addLotteryFirstReward uin[%d] size[%d] ", 
		__FUNCTION__, lotteryFirst.m_uid, m_lotterFirstRewardList.size());

	m_lotterFirstRewardList.push_back(lotteryFirst);
	

	if (m_lotterFirstRewardList.size() > 30/*(unsigned int) m_gameConfig.m_gameHistoryNum*/)  
	{
		m_lotterFirstRewardList.pop_front();
	}
}


// һ�Ƚ������Ӧ��
void AllKillGame::sendLotterFirstPlayers(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_LOTTERY_FIRST_SU;

	for (unsigned int i = 0; i < m_lotterFirstRewardList.size(); i++)
	{
		LotteryFirstReward lotterFirst = m_lotterFirstRewardList[i];

		packet.val["player_list"][i]["uid"]          = lotterFirst.m_uid; 
		packet.val["player_list"][i]["name"]         = lotterFirst.m_name;
		packet.val["player_list"][i]["sex"]          = lotterFirst.m_sex; 
		packet.val["player_list"][i]["avatar"]       = lotterFirst.m_avatar;
		packet.val["player_list"][i]["reward_money"] = lotterFirst.m_rewardMoney;
		packet.val["player_list"][i]["reward_time"]  = lotterFirst.m_rewardTime;
		packet.val["player_list"][i]["card_type"]    = lotterFirst.m_rewardTime;
		
	}
	packet.end();

	m_server->unicast(player, packet.tostring());

	xt_log.debug("Func[%s] һ�Ƚ�������б� [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
}

