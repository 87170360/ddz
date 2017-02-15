#include <assert.h>
#include <math.h>
#include "AllKillGame.h"
#include "AllKillMacros.h"
#include "AllKillPlayer.h"
#include "AllKillServer.h"

#include "log.h"

extern Log xt_log;



//发牌
void GameBetting::deal(bool sys_role,int radio)
{
	xt_log.info("Func[%s] deal card ,role another card radio %d.\n",
		__FUNCTION__, radio);

	// 1)洗牌逻辑
	_uint8 GoodCardBuff[AK_HAND_CARD_COUNT][MAX_COUNT];  //5手牌         
	memset(GoodCardBuff, 0, sizeof(GoodCardBuff));

	//_uint8 GoodChairId[AK_SEAT_ID_NU] = { -1 }; //好牌的椅子

	_uint8 bRandCard[CARDCOUNT] = { 0 };       //52张牌                  
	memset(bRandCard, 0, sizeof(bRandCard));

	int bGoodsCardSwtich = 0;   ///先写死

	if (bGoodsCardSwtich == 1)
	{
		//统计每个玩家的局数
		//int count[AK_SEAT_ID_NU] = { -1 };
		//bool is_robot[AK_SEAT_ID_NU] = { false };
		//for (int i = 0; i < AK_SEAT_ID_NU; i++)
		//{
		//	if (seats[i].status < STATUS_IN_GAME)
		//		continue;

		//	count[i] = seats[i].player->total_board; ///总的对局数
		//	if (seats[i].player->uid < 10000)
		//	{
		//		is_robot[i] = true;
		//	}
		//}

		////洗好牌
		//m_GameLogic->GoodRandCardList(count, is_robot, bRandCard, CARDCOUNT, AK_SEAT_ID_NU, GoodCardBuff, GoodChairId);
	}
	else
	{
		//随机洗
		memcpy(bRandCard, m_cbCardData, CARDCOUNT);
		m_GameLogic->RandCardList(bRandCard, CARDCOUNT);
	}

	// 2)发牌--对庄家
	int len = 0;
	memcpy(m_roleCard, bRandCard + len, MAX_HAND_CARD_COUNT);
	len += MAX_HAND_CARD_COUNT;

	// 组牌
	m_roleCardType = (CT_BULL_TYPE)m_GameLogic->GetCardTypeAndData(
		m_roleCard,  MAX_HAND_CARD_COUNT, m_roleGroupCard);

	//如果是系统庄家，得到"没牛"的话，以一定的概率重新发牌给他
	if (CT_NOBULL == m_roleCardType && sys_role)
	{
		if (rand() % 100 < radio)
		{
			memcpy(m_roleCard, bRandCard + len, MAX_HAND_CARD_COUNT);
			len += MAX_HAND_CARD_COUNT;

			// 组牌
			m_roleCardType = (CT_BULL_TYPE)m_GameLogic->GetCardTypeAndData(
				m_roleCard, MAX_HAND_CARD_COUNT, m_roleGroupCard);
		}
	}

	// 3)发牌--对坐位
	for (int i = 0; i < AK_SEAT_ID_NU; i++) 
	{
		//好牌
		if (bGoodsCardSwtich == 1 /*&& GoodChairId[i] == i*/)
		{
			memcpy(m_seatCard[i], GoodCardBuff[0], MAX_HAND_CARD_COUNT);
		}
		else
		{
			memcpy(m_seatCard[i], bRandCard + len, MAX_HAND_CARD_COUNT);
			len += MAX_HAND_CARD_COUNT;
		}	

		// 组牌
		m_seatCardType[i] = (CT_BULL_TYPE)m_GameLogic->GetCardTypeAndData(
			m_seatCard[i], MAX_HAND_CARD_COUNT, m_seatGroupCard[i]);
	}

	// 本局剩余时间
	m_betRemainTime = AK_START_TIME;

}



// 牌型倍数 
static int AK_GameGetCardTimes(int card_type)
{
	int card_bet=1;

	if(card_type <= CT_BULL_SIX)        // 牛六到牛一,无牛
		card_bet = 1;
	else if(card_type <= CT_BULL_NINE)  // 牛九到牛七
		card_bet = 2;	
	else if(card_type == CT_BULL_BULL)  // 牛牛
		card_bet = 3;
	else if(card_type == CT_BOMB)       // 四炸
		card_bet = 4;
	else if(card_type == CT_FIVE_H_BULL) // 五花牛
		card_bet = 5;
	else if(card_type == CT_FIVE_S_BULL) // 五小牛
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
		int operator() (AllKillPlayer* l,AllKillPlayer* r)  //函数操作符
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

	//连续当庄次数
	m_role_nu = 0;  

	//闲家位置清空
	memset(m_playerSeat, 0, sizeof(m_playerSeat));  
}



AllKillGame::~AllKillGame()
{


}


// 关闭万人场
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



// 场(venue)相关的配置
int AllKillGame::configGame(Json::Value& value)
{
	m_gameConfig.m_vid             =value["venue"]["vid"].asInt();
	m_gameConfig.m_baseMoney       = 1; //value["venue"]["base_money"].asInt();  ///基注，（没用了，现在下注不再是基注的倍数了）
	m_gameConfig.m_roleAnotherCard = value["venue"]["role_another_radio"].asInt();	
	m_gameConfig.m_sysFee          = value["venue"]["sys_fee"].asDouble();        // 系统抽水	
	m_gameConfig.m_winRottleFee    = value["venue"]["win_rottle_fee"].asDouble(); // 赢钱抽水
	m_gameConfig.m_rottleMinOpenMoney=value["venue"]["rottle_min_open_money"].asInt();

	m_gameConfig.m_askRoleMinMoney = value["venue"]["ask_role_min"].asInt();
	m_gameConfig.m_unRoleLimitMoney= value["venue"]["un_role_limit"].asInt();

	m_gameConfig.m_gameHistoryNum  =value["venue"]["game_history_nu"].asInt();
	m_gameConfig.m_enterRoomChat   = value["venue"]["enter_chat"].asString();	  // 欢迎词

	// 牌型开奖奖池比例
	m_gameConfig.m_rottleBombRadio      =value["venue"]["rottle_radio"]["ct_bomb"].asDouble();
	m_gameConfig.m_rottleFiveHBullRadio =value["venue"]["rottle_radio"]["ct_five_h_bull"].asDouble();
	m_gameConfig.m_rottleFiveSBullRadio =value["venue"]["rottle_radio"]["ct_five_s_bull"].asDouble();

	// 入座条件
	m_gameConfig.m_sitMinMoney =value["venue"]["sit_down"]["min_money"].asInt();
	m_gameConfig.m_sitVLevel   =value["venue"]["sit_down"]["min_vlevel"].asInt();

	// sys_role系统庄家
	m_gameConfig.m_sysRoleName     = value["venue"]["sys_role"]["name"].asString();
	m_gameConfig.m_sysRoleAvatar   = value["venue"]["sys_role"]["avatar"].asString();
	m_gameConfig.m_sysRoleUid      = value["venue"]["sys_role"]["uid"].asInt();
	m_gameConfig.m_sysRoleMoney    = value["venue"]["sys_role"]["money"].asInt();
	m_gameConfig.m_sysRoleSex      = value["venue"]["sys_role"]["sex"].asInt();

	// chat_role聊天庄家
	m_gameConfig.m_chatRoleName  = value["venue"]["chat_role"]["name"].asString();
	m_gameConfig.m_chatRoleAvatar= value["venue"]["chat_role"]["avatar"].asString();
	m_gameConfig.m_chatRoleUid   = value["venue"]["chat_role"]["uid"].asInt();
	m_gameConfig.m_chatRoleSex   = value["venue"]["chat_role"]["sex"].asInt();


	//获奖第一人初始为系统玩家
	m_lotteryFirstReward.m_uid    = m_gameConfig.m_sysRoleUid;
	m_lotteryFirstReward.m_name   = m_gameConfig.m_sysRoleName;
	m_lotteryFirstReward.m_avatar = m_gameConfig.m_sysRoleAvatar;
	m_lotteryFirstReward.m_sex    = m_gameConfig.m_sysRoleSex;
	m_lotteryFirstReward.m_rewardMoney = 0;
	m_lotteryFirstReward.m_rewardTime = time(NULL);

	return 0;
}



// ===>>>游戏启动
int AllKillGame::start(AllKillServer* server,struct ev_loop* loop)
{
	m_gameStatus = AK_Ready;  //初始为“准备状态”
	m_server = server;
	m_evLoop = loop;

	// 准备
	m_evReadyTimer.data = this;
	ev_timer_init(&m_evReadyTimer,AllKillGame::onReadyTimer, 0, AK_READY_TIME);

	// 开始
	m_evStartTimer.data = this;
	ev_timer_init(&m_evStartTimer,AllKillGame::onStartTimer, 0, AK_START_TIME);

	// 结束
	m_evEndGameTimer.data = this;
	ev_timer_init(&m_evEndGameTimer,AllKillGame::onEndGameTimer, 0, AK_END_GAME_TIME);

	// 更新
	m_evUpdateTimer.data = this;
	ev_timer_init(&m_evUpdateTimer,AllKillGame::onUpdateGameTimer, 0, AK_UPDATE_TIME);

	// 换庄(游戏结束前3秒进行换庄)
	m_evChangeRoleTimer.data = this;
	ev_timer_init(&m_evChangeRoleTimer,AllKillGame::onChangeRoleTimer,0, AK_END_GAME_TIME - AK_SET_NEXT_ROLE_BEFORE_TIME);


	ev_timer_again(m_evLoop,&m_evReadyTimer);   //启动ready timer
	handleGameReady();

	return 0;
}


// 游戏准备
void AllKillGame::handleGameReady()
{
	// 重置开奖结果
	m_lotteryResult.reset();          

	 // 重置庄家游戏结果
	m_roleGameResult.reset();        

	// 重置坐位游戏结果
	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		m_seatGameResult[i].reset();
	}

	// 重置对局信息
	m_gameBetting.reset();        

	// 重置最多奖金获得的玩家
	m_mostRewardPlayer = NULL;    

	// 重置玩家的位置下注信息
	for (std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		 iter!=m_betPlayers.end(); ++iter)
	{
		iter->second->resetSeatBet();
	}

	// 清空下注玩家列表
	m_betPlayers.clear(); 

	// 清除offline列表中的玩家
	for (std::map<int, AllKillPlayer*>::iterator iter = m_offlinePlayers.begin();
		 iter != m_offlinePlayers.end(); ++iter)
	{
		delete iter->second;
	}
	m_offlinePlayers.clear();


	//庄家再次拿牌概率
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
	
	// 发牌（系统庄家根据比例，可二次拿牌）
	m_gameBetting.deal(NULL == m_role, radio);

	// 设置庄家
	setNextRole();  

	// 广播“游戏准备”
	broadcastGameReady(NULL);
}


int AllKillGame::setNextRole()
{
	int change_role = false;
	if (m_role != NULL)
	{
		if (m_role->getIsUnRole())  // 下庄吗
		{
			m_role      = NULL;
			change_role = true;
		}
	}

	if (m_role != NULL)   //检查庄家的金币，不够的话下庄处理
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
				// 清除上庄列表中，小于上庄底限的玩家
				m_askRoleList.erase(m_askRoleList.begin());
				continue;
			}

			player->setUnRole(false);   //设置下庄标记
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


// 广播“游戏准备”
void AllKillGame::broadcastGameReady(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"]          = AK_GAME_READY_SB;
	packet.val["rottle_money"] = m_lotteryTotalMoney;  //奖池金额

	formatRole(&packet);         //当前庄家信息
	formatAskRoleList(&packet);  //当前等特上庄列表

	packet.end();
	m_server->broadcast(player,packet.tostring());


	xt_log.debug("Func[%s] 广播游戏准备 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

}

// “准备时间”完毕 
void AllKillGame::onReadyTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->readyTimer();
}


//“准备结束”，游戏进入“开始状态”
void AllKillGame::readyTimer()
{
	ev_timer_stop(m_evLoop,&m_evReadyTimer);
	m_gameStatus = AK_Start;

	handleGameStart();  //广播游戏开始

	ev_timer_again(m_evLoop, &m_evStartTimer);
	ev_timer_again(m_evLoop, &m_evUpdateTimer);

}

// 处理开始
void AllKillGame::handleGameStart()
{
	m_role_nu++;      //累计当庄次数
	broadcastGameStart(NULL);
}

// 广播游戏开始
void AllKillGame::broadcastGameStart(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"]          = AK_GAME_START_SB;
	packet.val["remain_time"]  = m_gameBetting.m_betRemainTime;  //离结束的时间
	packet.val["rottle_money"] = m_lotteryTotalMoney;             //奖池金额

	// 给庄家发一张牌
	packet.val["role_info"]["card"].append(m_gameBetting.m_roleCard[0]);

	// 给4个位置发一张牌
	for (int k = 0; k < AK_SEAT_ID_NU; k++)
	{
		packet.val["seat_info"][k]["seat_id"] = k + AK_SEAT_ID_START;  // 坐位从1开始
		packet.val["seat_info"][k]["card"].append(m_gameBetting.m_seatCard[k][0]);
	}

	packet.end();
	m_server->broadcast(player, packet.tostring());

	xt_log.debug("Func[%s] 广播游戏开始 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
}


//“开始时间”完毕，游戏进入结束状态
void AllKillGame::onStartTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game = (AllKillGame*)w->data;
	game->startTimer();
}


// start时间完毕
void AllKillGame::startTimer()
{
	ev_timer_stop(m_evLoop, &m_evStartTimer);
    ev_timer_stop(m_evLoop, &m_evUpdateTimer);  //停止更新timer

	m_gameStatus = AK_EndGame;   //游戏结束
	handleGameEnd();

	if(m_lotteryResult.m_hasOpenLottery) 
	{
		// 若有开奖，则增加开奖时间(AK_LOTTERY_OPEN_TIME5秒)
		ev_timer_set(&m_evEndGameTimer, 0, AK_END_GAME_TIME + AK_LOTTERY_OPEN_TIME);
		ev_timer_again(m_evLoop ,&m_evEndGameTimer);

		// 换庄时间顺延
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


//游戏结束
void AllKillGame::handleGameEnd()
{
	m_gameBetting.m_endGameTime = time(NULL);

	handlePlayerBetResult();   // 玩家对局结果

	handleMoneyResult();       // 处理输赢

	handleLotteryResult();     // 处理开奖

	sendBetPlayerResult();     // *发送玩家对局结果

	broadcastGameUpdate(NULL); // 广播游戏更新
	broadcastGameEnd(NULL);    // 广播游戏结束

	sendGameInfoToSpeaker();   // 发布消息（开奖和最高获奖者信息）


	saveGameResultToSql();     // 保存入库

}


// 玩家对局结果
void AllKillGame::handlePlayerBetResult()
{
	/*  card info */
	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		//与庄家比牌
		if (m_gameBetting.m_GameLogic->CompareCard(
			m_gameBetting.m_roleGroupCard, 
			m_gameBetting.m_seatGroupCard[i], MAX_HAND_CARD_COUNT, 0, 0))
		{
			// 庄家赢，坐位输
			m_seatGameResult[i].m_isWin = 0;
			// 赢方牌型倍数
			m_seatGameResult[i].m_betTime = AK_GameGetCardTimes(m_gameBetting.m_roleCardType);
		}
		else 
		{
			//庄家输，坐位赢
			m_seatGameResult[i].m_isWin = 1;
			// 赢方牌型倍数
			m_seatGameResult[i].m_betTime = AK_GameGetCardTimes(m_gameBetting.m_seatCardType[i]);
		}

		// 坐位牌型
		m_seatGameResult[i].m_cardType = m_gameBetting.m_seatCardType[i];
	
		// 坐位的下注数
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

			// 坐位玩家
			if(player_bet_nu > 0)
			{
				m_seatGameResult[i].m_betPlayerList.push_back(player);
			}
		}
	}

	/* sort player bet */
	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		//按下注金额，排序前4位玩家
		std::sort(m_seatGameResult[i].m_betPlayerList.begin(),
			m_seatGameResult[i].m_betPlayerList.end(),
			AK_GamePlayerBetSortFunc(i + AK_SEAT_ID_START));
	}


	/* handle role result*/
	m_roleGameResult.m_cardType = m_gameBetting.m_roleCardType;                 //庄家牌型
	m_roleGameResult.m_betTime  = AK_GameGetCardTimes(m_roleGameResult.m_cardType); //庄家牌型倍数


	// 保存游戏结果
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

// 处理输赢
void AllKillGame::handleMoneyResult()
{
	/* handle player bet info */
	for (std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		 iter != m_betPlayers.end(); ++iter)
	{
		AllKillPlayer* player = iter->second;
		player->updateMoney();   // 先更新下金币

		for (int i = 0; i < AK_SEAT_ID_NU; i++)
		{
			int bet_nu = player->getSeatBet(i + AK_SEAT_ID_START);

			/* add player already bet money 先把下注的钱拿回去*/
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


	/* handle player lose 处理闲家输的*/
	for(std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		iter != m_betPlayers.end(); ++iter)
	{
		AllKillPlayer* player = iter->second;
		int bet_money = player->getBetMoneyResult();

		if (bet_money > 0)
		{
			// 记录庄家输的
			m_roleGameResult.m_roleLostMoney += bet_money;
			player->m_winCount++;   //累加连赢次数
			continue;
		}

		player->m_winCount = 0;     //连赢次数置0

		// 以下是“庄家赢”
		m_roleGameResult.m_roleWinMoney += -bet_money;
		int player_money = player->getMoney();
		if (player_money < 0)
		{
			xt_log.error("player(%d) money(%d) is negative\n",player->getUid(),player_money);
			player_money=0;
		}

		//玩家不够输，输光
		if(player_money < -bet_money)
		{
			bet_money = -player_money;
		}

		player->setBetMoneyResult(bet_money);  //设置玩家对局金额

		m_roleGameResult.m_roleRealWinMoney += -bet_money;  //庄家实际赢

		player->decMoney(-bet_money);   //扣除玩家输的钱
		m_server->sendBetFlow(m_gameConfig.m_vid, player->getUid(), bet_money,player->getMoney());

		//++闲家输
		m_server->savePlayerBet(m_gameConfig.m_vid, 
			player->getUid(), 
			player->getBetMoneyResult(),
			0);
	}


	/* handle role win 庄家赢*/
	float role_pay_radio = 1.0f;   // 庄家不够输时，按比例分配给闲家
	int role_result_money = m_roleGameResult.m_roleRealWinMoney - m_roleGameResult.m_roleLostMoney;//(庄家最后输赢)

	m_roleGameResult.m_roleResultBetMoney = role_result_money;


	if (m_role != NULL)   //有庄家
	{
		m_role->updateMoney();
		int role_money = m_role->getMoney();

		if (role_result_money > 0)
		{
			//庄家赢
			m_role->m_winCount++;
			int rottle_money = floor(role_result_money * m_gameConfig.m_winRottleFee); //奖池抽水
			m_lotteryTotalMoney += rottle_money;

			xt_log.info("Func[%d] role money %d, add role money %d,rottle_money %d,real_money %d \n",
				__FUNCTION__, role_money, role_result_money, rottle_money,role_result_money - rottle_money);

			int sys_fee = floor(role_result_money * m_gameConfig.m_sysFee);  //系统抽水

			m_role->incMoney(role_result_money - rottle_money - sys_fee);  //庄家实际赢的

			//庄家对局流水
			m_server->sendBetFlow(m_gameConfig.m_vid, m_role->getUid(),role_result_money-rottle_money-sys_fee,m_role->getMoney());

		}
		else 
		{   //庄家输
            m_role->m_winCount = 0;
			// 1)够输
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
			{   //2)不够输，输光(闲家按比例分)
				m_role->decMoney(role_money);

				m_server->sendBetFlow(m_gameConfig.m_vid, m_role->getUid(), -role_money, m_role->getMoney());
				xt_log.info("role money %d, dec role money %d,rottle_money %d,real_money %d \n",
					role_money,role_result_money);

				role_pay_radio = float(role_money) / float(-role_result_money);  //支付比率
			}
		}

		//xt_log.info("role_money is %d \n",m_role->updateMoney());
	}
	else 
	{ //无庄家
		/*
		   if(role_result_money>0)
		   {
		   int rottle_money=floor(role_result_money*m_gameConfig.m_winRottleFee);
		   m_lotteryTotalMoney+=rottle_money;
		   }
		   */
	}


	// 最大赢家
	m_mostRewardPlayer = NULL;

	/* handle player win 闲家赢*/
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

		int rottle_money = floor(result_money * m_gameConfig.m_winRottleFee);  // 奖池抽水（对赢家）
		m_lotteryTotalMoney += rottle_money;

		int sys_fee = floor(result_money * m_gameConfig.m_sysFee);   //系统抽水（对赢家）

		player->incMoney(result_money - rottle_money - sys_fee);

		m_server->sendBetFlow(m_gameConfig.m_vid, player->getUid(), result_money-rottle_money-sys_fee, player->getMoney());
		
		//++闲家赢
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


// 处理开奖情况
void AllKillGame::handleLotteryResult()
{
	m_lotteryResult.m_hasOpenLottery = false;

	if (m_lotteryTotalMoney < m_gameConfig.m_rottleMinOpenMoney)  //奖池开奖底限
	{
		return ;
	}

	/*开奖牌型*/
	int card_types[] = {CT_BOMB, CT_FIVE_H_BULL, CT_FIVE_S_BULL};

	for(unsigned int i = 0; i < sizeof(card_types) / sizeof(int); i++)
	{
		int num = getCardTypeNu(card_types[i]);
		if (0 == num)
		{
			continue;
		}

		m_lotteryResult.m_hasOpenLottery = true;           // 满足有指定牌型，则可以开奖
		handleRottleResultByCardType(card_types[i], num); // 最终按最大牌型开奖
	}


	// 满足开奖,并且有庄家
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
		
		// 设置最高奖金获得者
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

			// 闲家奖金，按下注比例份
			int rottle_money = floor(m_lotteryResult.m_seatLotteryMoney[i] * (float)bet_nu / (float)m_gameBetting.m_seatBet[i]);

			player->incMoney(rottle_money);
			m_lotteryFirstReward.setIfMostLotteryReward(rottle_money, m_lotteryResult.m_cardType, player);
			m_server->sendLotteryFlow(m_gameConfig.m_vid,player->getUid(), rottle_money, player->getMoney());
		}
	}

	//奖金第一名
    addLotteryFirstReward(m_lotteryFirstReward);
}

// 发送玩家对局结果
void AllKillGame::sendBetPlayerResult()
{
	//闲家的输赢
	for (std::map<int, AllKillPlayer*>::iterator iter = m_betPlayers.begin();
		iter!=m_betPlayers.end(); ++iter)
	{		
		Jpacket p_packet;
		p_packet.val["cmd"] = AK_PLAYER_BET_REWARD_SU;
		p_packet.val["reward"]=iter->second->getBetMoneyResult();  // 赢或输的金钱
		p_packet.val["money"]=iter->second->getMoney();            // 玩家的金币
		p_packet.end();
		m_server->unicast(iter->second, p_packet.tostring());
	}

	//庄家的输赢
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


// 广播游戏结束
void AllKillGame::broadcastGameEnd(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"] = AK_GAME_END_SB;  // 游戏结束广播

	formatLotteryFirstReward(&packet);   // 中奖第一的玩家
	formatGameResult(&packet);

	// ++更新对局玩家信息
	formatBetPlayerResult(&packet);

	packet.val["rottle_money"] = m_lotteryTotalMoney;   //奖池金额
	packet.end();


	m_server->broadcast(player, packet.tostring());

	xt_log.debug("Func[%s] 广播游戏结束 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

}


//发布消息（开奖和最高获奖者信息）
void AllKillGame::sendGameInfoToSpeaker()
{
	if (m_lotteryResult.m_hasOpenLottery)
	{
		char buf[2048];
		int money_w = m_lotteryFirstReward.m_rewardMoney/10000;
		if (money_w >10)
		{
			sprintf(buf,"万人场奖池开奖，恭喜%s获得头奖，%d万奖金",m_lotteryFirstReward.m_name.c_str(),money_w);
			m_server->sendSpeaker(4006,0,"系统",buf);
		}
	}


	if (m_mostRewardPlayer != NULL)
	{
		char buf[2048];
		int result_money=m_mostRewardPlayer->getBetMoneyResult();
		int money_w=result_money/10000;
		if(money_w>500)
		{
			sprintf(buf,"%s在万人场中胆识超群，天量下注赢得%d万金币车载斗量满载而归",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"系统",buf);
		}
		else if(money_w>100)
		{
			sprintf(buf,"%s在万人场中面不改色，疯狂下注赢得%d万金币终将一树百获",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"系统",buf);

		}
		else if(money_w>50)
		{
			sprintf(buf,"%s在万人场中胆大心细，赢得%d万金币稳吃三注",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"系统",buf);
		}
	}
}

//游戏日志流水入库
void AllKillGame::saveGameResultToSql()
{
	return;   //没有表allkill_log

	if (m_gameStatus != AK_EndGame)
	{
		xt_log.error("status error for saveGameResultToSql\n");
		return;
	}

	// 在线人数
	int online_people = m_loginPlayers.size();

	// 表字段
	std::string table_info="( create_time, vid, online_people, bet_people,rottle_money,rottle_open_money,role_id, role_tcard, role_rcard, role_money, id1_people, id1_betnu, id1_tcard, id1_rcard, id1_iswin,id1_money, id2_people, id2_betnu, id2_tcard, id2_rcard, id2_iswin,id2_money, id3_people, id3_betnu, id3_tcard, id3_rcard,id3_iswin, id3_money, id4_people, id4_betnu, id4_tcard, id4_rcard,id4_iswin, id4_money)" ;

	// 开奖金额
	int rottle_open=0;
	if (m_lotteryResult.m_hasOpenLottery)
	{
		//开奖金额
		rottle_open += m_lotteryResult.m_roleLotteryMoney;

		for(int i=0;i<AK_SEAT_ID_NU;i++)
		{
			rottle_open += m_lotteryResult.m_seatLotteryMoney[i];
		}
	}


	// 下注人数
	int bet_people = m_betPlayers.size();

	// 当前时间
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

	// 庄家uid
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


//“结束时间”完毕
void AllKillGame::onEndGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->endGameTimer();
}

// 本局结束，进入下局“游戏准备”
void AllKillGame::endGameTimer()
{
	ev_timer_stop(m_evLoop, &m_evChangeRoleTimer);
	ev_timer_stop(m_evLoop, &m_evEndGameTimer);

	m_gameStatus = AK_Ready;

	handleGameReady();
	ev_timer_again(m_evLoop, &m_evReadyTimer);


	// 刷新在线玩家数
	m_server->refreshPlayerNuToRedis();

}

//“更新时间”完毕
void AllKillGame::onUpdateGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->updateGameTimer();
}

// “换庄时间”完毕
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

// 广播游戏更新
void AllKillGame::broadcastGameUpdate(AllKillPlayer* player)
{
	bool showUpdateLog = false;
	Jpacket packet;
	packet.val["cmd"] = AK_GAME_UPDATE_SB;  //游戏更新广播

	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		packet.val["seat_bet_info"][i]["seat_id"] = i + AK_SEAT_ID_START;      //位置信息
		packet.val["seat_bet_info"][i]["bet_nu"]  = m_gameBetting.m_seatBet[i];//压数注数

		// 此段时间，该位置下注的玩家列表，更新后清零
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

	packet.val["remain_time"] = m_gameBetting.m_betRemainTime;  //离结束时间
	packet.end();
	m_server->broadcast(NULL, packet.tostring());


	if (showUpdateLog)
	{
		xt_log.debug("Func[%s] 广播更新 [%s]\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
	

}


// 创建玩家对象
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

// 玩家登录游戏
void AllKillGame::playerLogin(AllKillPlayer* player)
{
	assert(player != NULL);
	int uid = player->getUid();

	std::map<int,AllKillPlayer*>::iterator iter;

	iter = m_loginPlayers.find(uid);
	// 已经登录，直接返回
	if (iter != m_loginPlayers.end())
	{
		xt_log.error("player(%d) all ready login\n",uid);
		return;
	}
	
	iter= m_offlinePlayers.find(uid); 
	//在离线中找到，则放入在线玩家
	if(iter!=m_offlinePlayers.end())
	{
		m_loginPlayers[iter->first]=iter->second;
		m_offlinePlayers.erase(iter);
	}
	else 
	{
		xt_log.error("player find in offline\n");  //?
	}

	sendLoginSuccess(player);  //下行登录成功
	sendGameInfo(player);      //下行游戏信息

	//下行欢迎词
	sendPlayerChat(player, m_gameConfig.m_enterRoomChat); 
}

// 闲家压注
void AllKillGame::playerBet(AllKillPlayer* player,Jpacket& package)
{
	int seat_id = package.val["seat_id"].asInt();   // 压注位置
	int bet_nu  = package.val["bet_nu"].asInt();    // 压注注数

	if (m_role == player)
	{
		xt_log.error("Fun[%s]庄家不可以下注",__FUNCTION__);
		sendBetError(player, AK_ROLE_BET_ERR, "error:role can not bet");
		return;
	}

	// 游戏还未开始，下注失败
	if (m_gameStatus != AK_Start)
	{
		sendBetError(player, AK_STATUS_ERR, "game status error");
		return;
	}

    // 压注注数为0
	if (bet_nu <= 0)
	{
		sendBetError(player, AK_BET_NU_ERR, "bet_nu greater than 0");
		return;
	}

	// 位置非法
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

	// 金币不够
	if (money < m_gameConfig.m_baseMoney * bet_nu)
	{
		sendBetError(player, AK_MONEY_NOT_ENOUGH, "money not enough");
		return ;
	}

	m_gameBetting.m_seatBet[seat_id - AK_SEAT_ID_START] += bet_nu; //累计位置的下注数

	// 下注扣钱
	player->decMoney(m_gameConfig.m_baseMoney * bet_nu);  

	// 记录玩家的位置压注
	player->addSeatBet(seat_id, bet_nu);   

	int uid = player->getUid();

	m_betPlayers[uid] = player;     // 加入到对局玩家列表
	sendBetSuccess(player, seat_id, bet_nu); // 下注成功应答

}

//申请上庄
void AllKillGame::playerAskRole(AllKillPlayer* player,Jpacket& package)
{
	xt_log.info("Func[%s] player(%d) ask_role\n",
		__FUNCTION__, player->getUid());

	int role_nu = m_askRoleList.size();

	for(int i = 0;i < role_nu; i++)
	{
		if(m_askRoleList[i] == player)
		{
			xt_log.warn("player(%d) 已经在上庄列表中\n",player->getUid());
			sendAskRoleSuccess(player);
			return;
		}
	}

	if (m_role == player)
	{
		xt_log.warn("player(%d) alread role. 已经是庄家,无需申请上庄.\n",player->getUid());
		sendAskRoleSuccess(player);
		return;
	}

	int money = player->getMoney(); 
	if (money < m_gameConfig.m_askRoleMinMoney)  ///上庄底限
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

	m_askRoleList.push_back(player);  //放入上庄列表中
	sendAskRoleSuccess(player);

}

// 玩家下庄
void AllKillGame::playerUnRole(AllKillPlayer* player,Jpacket& package)
{
	if (m_role == player)  //庄家是我
	{
		if (AK_EndGame == m_gameStatus)
		{
			m_role = NULL;

			if (m_askRoleList.size() > 0)
			{
				AllKillPlayer* player = m_askRoleList[0]; // 轮到上庄列表第一个人为当前庄家
				player->setUnRole(false);                 // 下庄标记

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

// 玩家登出
void AllKillGame::playerLogout(AllKillPlayer* player)
{
	if (NULL == player)
	{
		return;
	}

	//站起
	playerStandUp(player);

	int uid=player->getUid();

	// 游戏结束
	if (AK_EndGame == m_gameStatus)
	{
		m_loginPlayers.erase(uid);

		//如果我是庄家,则置空庄家
		if (player == m_role)
		{
			m_role=NULL;
		}
		sendPlayerLogoutSuccess(player);

		int role_nu = m_askRoleList.size();
		for (int i = 0; i < role_nu; i++)
		{
			//如果我在“上庄列表中”
			if (m_askRoleList[i]==player)
			{
				m_askRoleList.erase(m_askRoleList.begin() + i);
				broadcastAskRoleChange(player);
				break;
			}
		}

		// 如果对局玩家列表中，则放入离线，下次清除用
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
			//设置下庄标记
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

// 在每局游戏结束前3秒,触发换庄
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



// 处理换庄
void AllKillGame::handleChangeRole()
{
	int ret = setNextRole();

    // 有换庄 
	if (ret)
	{
		broadcastAskRoleChange(NULL);

		// 重置连续坐庄次数
		m_role_nu = 0;

		// 闲家成为庄家,如果其有坐位，则需站起	
		playerStandUp(m_role);
	}
}




// 此牌型个数
int AllKillGame::getCardTypeNu(int card_type)
{
	int ret = 0;
	if (m_roleGameResult.m_cardType == card_type)   // 庄家牌型
	{
		ret++;
	}

	for (int i = 0; i < AK_SEAT_ID_NU; i++)    // 位置牌型
	{
		if (m_seatGameResult[i].m_cardType == card_type
			&& m_seatGameResult[i].m_totalBetNu > 0)
		{
			ret++;
		}
	}
	return ret;
}

// 牌型开奖比例
float AllKillGame::getCardTypeLotteryRadio(int card_type)
{
	float rottle_radio=0.0;

	if(CT_BULL_BULL == card_type)  //四炸
	{
		rottle_radio = m_gameConfig.m_rottleBombRadio;
	}
	else if (CT_FIVE_H_BULL == card_type)  //五花牛
	{
		rottle_radio = m_gameConfig.m_rottleFiveHBullRadio;
	}
	else if(CT_FIVE_S_BULL == card_type)  //五小牛
	{
		rottle_radio = m_gameConfig.m_rottleFiveSBullRadio;
	}
	return rottle_radio;
}


// 按牌型开奖（当庄家，或者位置有开奖牌型，才能得奖）
void AllKillGame::handleRottleResultByCardType(int card_type, int num)
{
	assert(num > 0);

	float rottle_radio  = getCardTypeLotteryRadio(card_type);
	int rottle_money    = m_lotteryTotalMoney * rottle_radio;
	m_lotteryTotalMoney -= rottle_money;

	rottle_money = rottle_money / num;      // 多个位置出现相同牌型

	m_lotteryResult.m_cardType = card_type; // 当前获奖的牌型

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



// 登录成功
void AllKillGame::sendLoginSuccess(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"] = AK_LOGIN_SU;
	packet.val["ret"] = 0;

	packet.end();

	m_server->unicast(player,packet.tostring());

	xt_log.debug("Func[%s] 登录成功应答 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

	/* broadcast  广播登录成功*/
	packet.val["cmd"]    = AK_LOGIN_SUCC_SB;
	packet.val["uid"]    = player->getUid();
	packet.val["seatid"] = player->m_seatId;
	packet.val["name"]   = player->getName();
	packet.val["sex"]    = player->getSex();
	packet.val["avatar"] = player->getAvatar();
	packet.val["money"]  = player->getMoney();
	packet.end();

	m_server->broadcast(player, packet.tostring());
	xt_log.debug("Func[%s] 广播登录 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());


}

// 发送游戏（房间）信息
void AllKillGame::sendGameInfo(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_GAME_INFO_SU;

	// 彩票中奖第一的玩家
	formatLotteryFirstReward(&packet);

	// 游戏的配置信息
	packet.val["config"]["bet_base"] = m_gameConfig.m_baseMoney; //基注金额
	packet.val["config"]["ask_role_min"] = m_gameConfig.m_askRoleMinMoney;//申请上庄的最小金额
	packet.val["config"]["un_role_limit"] = m_gameConfig.m_unRoleLimitMoney;  //下庄的最小金额

	packet.val["config"]["rottle_ct_bomb"] = m_gameConfig.m_rottleBombRadio;              //开炸弹的中奖金额比
	packet.val["config"]["rottle_ct_five_h_bull"] = m_gameConfig.m_rottleFiveHBullRadio;  //开五花牛的中奖金额比
	packet.val["config"]["rottle_ct_five_s_bull"] = m_gameConfig.m_rottleFiveSBullRadio;  //开五小牛的中奖金额比

	packet.val["config"]["win_rottle_fee"] = m_gameConfig.m_winRottleFee;         //从玩家赢的钱抽水比例到奖池
	packet.val["config"]["rottle_min_open_money"] = m_gameConfig.m_rottleMinOpenMoney; //最小开奖金额

	packet.val["config"]["sit_min_money"] = m_gameConfig.m_sitMinMoney;  //入坐金币底限
	packet.val["config"]["sit_min_vlevel"] = m_gameConfig.m_sitVLevel;   //入坐vlevel等级底限

	// 下注宝箱配置
	int box_size = m_server->m_conf["bet_box_cfg"].size();
	for (int i = 0; i < box_size; i++)
	{
		packet.val["bet_box_cfg"][i]["box_type"]   = m_server->m_conf["bet_box_cfg"][i]["box_type"].asInt();
		packet.val["bet_box_cfg"][i]["bet_need"]   = m_server->m_conf["bet_box_cfg"][i]["bet_need"].asInt();
	}

	//玩家下注宝箱列表
	std::vector<BetBox> vecBetBox = m_server->queryBetLottery(player->getVid(), player->getUid());
	for (unsigned int k = 0; k < vecBetBox.size(); k++)
	{
		packet.val["bet_lottery_daily"][k]["box_type"]      = vecBetBox[k].boxType;
		packet.val["bet_lottery_daily"][k]["lottery_money"] = vecBetBox[k].lotteryMoney;
		packet.val["bet_lottery_daily"][k]["get_flag"]      = vecBetBox[k].getFlag;
	}

	// 奖池当前的金额
	packet.val["rottle_money"] = m_lotteryTotalMoney;  
    
	//当前等待上庄列表
	formatAskRoleList(&packet); 

	//当前庄家信息
	formatRole(&packet);         

	/* 位置id（下注位置从1开始）*/ 
	for(int i = 0; i < AK_SEAT_ID_NU; i++)
	{
		packet.val["seats"][i]["seat_id"] = AK_SEAT_ID_START + i;  //每个位置对应的seat_id
	}

	// 房间所有玩家信息
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

	//走式图
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


	// 游戏当前状态(AK_Ready, AK_Start, AK_EndGame)
	packet.val["status"] = m_gameStatus;  

	if (AK_Ready == m_gameStatus)
	{
		//===准备状态(AK_Ready)===
		/*do nothing */
	}
	else if (AK_Start == m_gameStatus)   
	{
		//===开始状态(AK_Start)===
		for(int i = 0; i < AK_SEAT_ID_NU; i++)
		{
			packet.val["bet_info"][i]["seat_id"]     = i + AK_SEAT_ID_START;  //位置信息
			packet.val["bet_info"][i]["seat_bet_nu"] = m_gameBetting.m_seatBet[i]; //位置下注注数
			packet.val["bet_info"][i]["your_bet_nu"] = player->getSeatBet(i + AK_SEAT_ID_START); //你下注的注数
		}

		packet.val["remain_time"] = m_gameBetting.m_betRemainTime;   //离结束的剩余时间


		// 给庄家发一张牌
		packet.val["role_info"]["card"].append(m_gameBetting.m_roleCard[0]);

		// 给4个位置发一张牌
		for (int k = 0; k < AK_SEAT_ID_NU; k++)
		{
			packet.val["seat_info"][k]["seat_id"] = k + AK_SEAT_ID_START;  // 坐位从1开始
			packet.val["seat_info"][k]["card"].append(m_gameBetting.m_seatCard[k][0]);
		}

	}
	else if (AK_EndGame == m_gameStatus)   
	{
		//===结束状态(AK_EndGame)===
		formatGameResult(&packet);
	}



	packet.end();

	m_server->unicast(player,packet.tostring());
}


//上庄列表改变的广播
void AllKillGame::broadcastAskRoleChange(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_ASK_ROLE_LIST_CHANGE_SB;
	formatAskRoleList(&packet); // 当前上庄列表
	formatRole(&packet);        // 当前庄家信息
	packet.end();
	m_server->broadcast(player, packet.tostring());

	
	xt_log.debug("Func[%s] 广播上庄列表 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
}










void AllKillGame::sendBetError(AllKillPlayer* player,int code,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]  = AK_PLAYER_BET_RESULT_SU;   // 下行成功压注数据包
	packet.val["ret"]  = code;                      // 0:为成功 其它:错误代码
	packet.val["desc"] = desc;                      // 错误描述
	packet.end();

	m_server->unicast(player, packet.tostring());
}


void AllKillGame::sendBetSuccess(AllKillPlayer* player,int seat_id, int bet_nu)
{
	Jpacket packet;

	packet.val["cmd"] = AK_PLAYER_BET_RESULT_SU;
	packet.val["ret"] = 0;

	packet.val["bet_info"]["seat_id"]     = seat_id; //位置信息
	packet.val["bet_info"]["seat_bet_nu"] = m_gameBetting.m_seatBet[seat_id-AK_SEAT_ID_START]; //总注数
	packet.val["bet_info"]["your_bet_nu"] = bet_nu;  //玩家下注的注数
	packet.val["money"] = player->getMoney(); //玩家的金币

	packet.end();

	m_server->unicast(player,packet.tostring());


	xt_log.debug("Func[%s] 压注成功返回 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

	//++广播下注
	//{
	//	Jpacket packet;

	//	packet.val["cmd"] = AK_PLAYER_BET_SB;

	//	packet.val["bet_info"]["seat_id"]     = seat_id; //位置信息
	//	packet.val["bet_info"]["seat_bet_nu"] = m_gameBetting.m_seatBet[seat_id - AK_SEAT_ID_START]; //总注数
	//	packet.val["bet_info"]["uid"]         = player->getUid();
	//	packet.val["bet_info"]["bet_nu"]      = bet_nu;  //玩家下注的注数
	//	packet.val["money"]                   = player->getMoney(); //玩家的金币

	//	packet.end();

	//	m_server->broadcast(player,packet.tostring());
	//}
}



// 上庄成功
void AllKillGame::sendAskRoleSuccess(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_ASK_ROLE_RESULT_SU; // 下行申请成功数据包
	packet.val["ret"] = 0;

	formatAskRoleList(&packet);
	formatRole(&packet);

	packet.end();

	/* unicast */
	m_server->unicast(player,packet.tostring());


	/* broadcast  */
	packet.val["cmd"] = AK_ASK_ROLE_LIST_CHANGE_SB;  //上庄列表改变的广播
	packet.end();

	m_server->broadcast(player,packet.tostring());
}


// 上庄失败
void AllKillGame::sendAskRoleError(AllKillPlayer* player,int code ,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]  = AK_ASK_ROLE_RESULT_SU;
	packet.val["ret"]  = code;
	packet.val["desc"] = desc;

	packet.end();
	m_server->unicast(player,packet.tostring());
}

// 下庄成功
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
	packet.val["cmd"] = AK_ASK_ROLE_LIST_CHANGE_SB; //上庄列表改变的广播
	packet.end();

	m_server->broadcast(player, packet.tostring());
}


//下庄错误
void AllKillGame::sendUnRoleErr(AllKillPlayer* player,int code,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]  = AK_ASK_UN_ROLE_RESULT_SU;
	packet.val["ret"]  = code;
	packet.val["desc"] = desc;
	packet.end();

	m_server->unicast(player,packet.tostring());
}

// 登出成功
void AllKillGame::sendPlayerLogoutSuccess(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"] = AK_LOGOUT_SU;
	packet.val["ret"] = 0;
	packet.end();
	m_server->unicast(player, packet.tostring());

	/* broadcast 广播登出 */
	{
		Jpacket packetBroadcast;
		packetBroadcast.val["cmd"] = AK_LOGOUT_SB;   
		packetBroadcast.val["uid"] = player->getUid();
		packetBroadcast.val["type"] = 0;
		packetBroadcast.end();

		m_server->broadcast(player, packetBroadcast.tostring());
	}

}

// 广播欢迎词（聊天框中）
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



//当前庄家信息
void AllKillGame::formatRole(Jpacket* packet)
{
	if (m_role != NULL)
	{
		packet->val["role"]["name"]        = m_role->getName();
		packet->val["role"]["money"]       = m_role->getMoney();
		packet->val["role"]["avatar"]      = m_role->getAvatar();
		packet->val["role"]["uid"]         = m_role->getUid();
		packet->val["role"]["sex"]         = m_role->getSex();
		packet->val["role"]["next_unrole"] = m_role->getIsUnRole()? 1:0;   //是否处于下庄状态
		packet->val["role"]["role_nu"]     = m_role_nu;  //连续当庄次数
		
	}
	else 
	{
		packet->val["role"]["name"]        = m_gameConfig.m_sysRoleName;
		packet->val["role"]["money"]       = m_gameConfig.m_sysRoleMoney;
		packet->val["role"]["avatar"]      = m_gameConfig.m_sysRoleAvatar;
		packet->val["role"]["uid"]         = m_gameConfig.m_sysRoleUid;
		packet->val["role"]["sex"]         = m_gameConfig.m_sysRoleSex;
		packet->val["role"]["next_unrole"] = 0;
		packet->val["role"]["role_nu"]     = m_role_nu;  //连续当庄次数
	}

}

// 上庄列表
void AllKillGame::formatAskRoleList(Jpacket* packet)
{
	int ask_role_nu = m_askRoleList.size();
	for(int i = 0; i < ask_role_nu; i++)
	{
		AllKillPlayer* player = m_askRoleList[i];  // 上庄列表
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



// 中奖第一的玩家
void AllKillGame::formatLotteryFirstReward(Jpacket* packet)
{
	packet->val["rottle_first_reward"]["name"]         = m_lotteryFirstReward.m_name;
	packet->val["rottle_first_reward"]["avatar"]       = m_lotteryFirstReward.m_avatar;
	packet->val["rottle_first_reward"]["sex"]          = m_lotteryFirstReward.m_sex;
	packet->val["rottle_first_reward"]["uid"]          = m_lotteryFirstReward.m_uid;
	packet->val["rottle_first_reward"]["reward_money"] = m_lotteryFirstReward.m_rewardMoney;  //获得的金币
	packet->val["rottle_first_reward"]["reward_time"]  = m_lotteryFirstReward.m_rewardTime;   //获奖时间
}

// 对局玩家信息
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

			//玩家下注宝箱列表
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

// 游戏结果
void AllKillGame::formatGameResult(Jpacket* packet)
{
	int remain_time=0;

	// 若有开奖，延长5秒
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

	packet->val["remain_time"]     = remain_time;                               // 剩余时间(离开局的时间)
	packet->val["has_open_rottle"] = m_lotteryResult.m_hasOpenLottery ? 1 : 0;  // 开奖标记

	int rottle_total_money = m_lotteryResult.m_roleLotteryMoney;

	for (int i = 0;i < AK_SEAT_ID_NU; i++)
	{
		rottle_total_money += m_lotteryResult.m_seatLotteryMoney[i];
	}

	packet->val["rottle_total_money"] = rottle_total_money;  // 开奖总金额


	/* format role info */
	for(unsigned int i = 1;i < MAX_HAND_CARD_COUNT; i++)
	{
		//庄家后4张牌
		packet->val["role_info"]["card"].append(m_gameBetting.m_roleCard[i]);  //庄家后4张牌
	}

	packet->val["role_info"]["rottle_money"] = m_lotteryResult.m_roleLotteryMoney;    // 庄家获得的奖池金额
	packet->val["role_info"]["card_type"]    = m_roleGameResult.m_cardType;           // 庄家的牌型            
	packet->val["role_info"]["reward_money"] = m_roleGameResult.m_roleResultBetMoney; // 获得的金币
	packet->val["role_info"]["bet_times"]    = m_roleGameResult.m_betTime;            // 倍数

	if (m_role != NULL)
	{
		packet->val["role_info"]["money"] = m_role->getMoney();    //庄家的金额
		packet->val["role_info"]["win_count"] = m_role->m_winCount;
	}
	else 
	{
		packet->val["role_info"]["money"] = m_gameConfig.m_sysRoleMoney;
		packet->val["role_info"]["win_count"] = 0;     //系统庄家连赢次数为0
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
			//坐位后4张牌
			packet->val["seat_info"][i]["card"].append(m_gameBetting.m_seatCard[i][j]);
		}

		//赢方倍数
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

			int seatWinMoney = player->getSeatBet(i + AK_SEAT_ID_START) * betTime;  //此位置下输赢的钱（税前）
			if(m_seatGameResult[i].m_isWin <= 0)
			{	
				//输时转为负数
				seatWinMoney = -seatWinMoney;
			}
			packet->val["seat_info"][i]["player_list"][j]["seat_win_money"] = seatWinMoney;			
		}
	}
}



//m_playerSeat[AK_SEAT_ID_NU]
int AllKillGame::doSitDown(AllKillPlayer* player, int seat_id)
{
	// 看看玩家指定的坐位是否为空坐
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

// 玩家坐下（注：玩家坐位是从0开始的）
void AllKillGame::playerSitDown(AllKillPlayer* player, int seat_id)
{	
	int ret = -2;
	std::string desc="";

	//判断位置是否合法
	if (seat_id >= 0 && seat_id < AK_SEAT_ID_NU)
	{
		// 庄家不能坐下
		if (player != m_role)
		{
			//如果之前是坐下的话，先执行站起（支持换位）	
			playerStandUp(player);

			//执行坐下
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
		player->m_seatId = -1;  // 坐下失败，不是空位置
		desc="not empty seat";
	}
	else
	{
		player->m_seatId = ret;  // 更新位置
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

	xt_log.debug("Func[%s] 坐下应答 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());


	// 成功坐下广播
	if (ret >= 0)
	{
		Jpacket packet;

		packet.val["cmd"]     = AK_SIT_DOWN_SB;
		packet.val["uid"]     = player->getUid();
		packet.val["seat_id"] = player->m_seatId;
		packet.end();

		m_server->broadcast(player, packet.tostring());

		xt_log.debug("Func[%s] 广播坐下 [%s]\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
}

// 站起
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

//区别就是有站起应答
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
		//站起失败(还没坐下)
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

	xt_log.debug("Func[%s] 站起应答 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
	

	// 站起成功广播
	if (ret >= 0)
	{
		Jpacket packet;
		packet.val["cmd"]     = AK_STAND_UP_SB;
		packet.val["uid"]     = player->getUid();
		packet.val["seat_id"] = ret;
		packet.end();

		m_server->broadcast(NULL, packet.tostring());

		xt_log.debug("Func[%s] 广播站起 [%s]\n", 
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

	// 站起成功广播
	if (seat_id >= 0)
	{
		Jpacket packet;
		packet.val["cmd"]     = AK_STAND_UP_SB;
		packet.val["uid"]     = player->getUid();
		packet.val["seat_id"] = seat_id;
		packet.end();

		m_server->broadcast(NULL, packet.tostring());

		xt_log.debug("Func[%s] 广播站起 [%s]\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
}

// 保存中奖第一名
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


// 一等奖获得者应答
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

	xt_log.debug("Func[%s] 一等奖获得者列表 [%s]\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
}

