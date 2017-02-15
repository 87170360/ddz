#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <algorithm>
#include <assert.h>

#include "bull.h"
#include "game.h"
#include "log.h"
#include "table.h"
#include "client.h"
#include "player.h"
#include "proto.h"
#include "XtCard.h"

#include <math.h>

extern Bull gBull;
extern Log xt_log;


///随机选庄玩法（匹配→抽水→随机选庄→发5张牌→配牌→与庄家比牌→结算）
Table::Table() :
  ready_timer_stamp(READY_TIME),     // 1准备时间（匹配人）
  banker_timer_stamp(BANKER_TIME),   // 2随机选庄时间
  send_card_timer_stamp(GROUP_TIME), // 3发牌(用户上行组牌)
  compare_timer_stamp(COMPARE_TIME), // 4比牌时间（即游戏结束，比牌结算）
  robot_timer_stamp(6)               // 机器人时间
{
 	///1准备（这个时间可以有玩家进来或者离开）
    ready_timer.data = this;
    ev_timer_init(&ready_timer, Table::ready_timer_cb, ready_timer_stamp, ready_timer_stamp);

	///2随机庄家
	banker_timer.data = this;
	ev_timer_init(&banker_timer, Table::banker_timer_cb, banker_timer_stamp, banker_timer_stamp);

	///3发牌
	send_card_timer.data = this;
	ev_timer_init(&send_card_timer, Table::send_card_timer_cb, send_card_timer_stamp, send_card_timer_stamp);

	///4比牌
    compare_timer.data = this;
    ev_timer_init(&compare_timer, Table::compare_timer_cb, compare_timer_stamp, compare_timer_stamp);


    robot_timer.data = this;
    ev_timer_init(&robot_timer, Table::robot_timer_cb, robot_timer_stamp, robot_timer_stamp);


	//int weight[6]={52*1.5,48*1.5,1096*1.5,720*1.5,3744*1.5,16440/2}; ///定义权重值
	//deck.setTypeWeight(weight);     ///设置每种牌型的权重

	//斗牛逻辑
	m_GameLogic = CBullGameLogic::GetInstance();
}

Table::~Table()
{
	ev_timer_stop(gBull.loop, &ready_timer);
	ev_timer_stop(gBull.loop, &banker_timer);
	ev_timer_stop(gBull.loop, &send_card_timer);

	ev_timer_stop(gBull.loop, &compare_timer);
	
	ev_timer_stop(gBull.loop, &robot_timer);
}

///桌子初始化
int Table::init(int my_tid,   
				int my_vid, 
				int my_min_money,
				int my_max_money, 
				int my_base_money, 
				int my_fee,
				int my_lose_exp, 
				int my_win_exp)
{
	//xt_log.debug("begin to init table [%d].\n", my_tid);

	tid = my_tid;                ///桌号
    vid = my_vid;                ///场馆号（高级、中级、低能等）

    min_money = my_min_money;    ///携带金币下限
	max_money = my_max_money;    ///携带金币下限
	base_money = my_base_money;  ///底注
 
	fee = my_fee;                ///抽水费率

	lose_exp = my_lose_exp;      ///输的经验值
	win_exp = my_win_exp;        ///赢的经验值

	
    cur_players = 0;             ///当前玩家个数
	dealer = -1;                 ///无庄家
	playersMap.clear();          ///清空玩家map
	ready_players = 0;           ///已准备好的玩家数
	game_state = WAIT;           ///初始“等待”          

	//清空桌子，分配坐位id
    for (int i = 0; i < MAX_CHAIR_COUNT; i++)
    {
		seats[i].clearSeat();
        seats[i].seatid = i;   
    }

    return 0;
}



/**
   玩家登录进来(满5人就不能再进了)
**/
int Table::handler_login(Player *player)
{
	if (playersMap.find(player->uid) == playersMap.end()) {

		//玩家坐下
		if (sit_down(player) < 0) {
			return -1;
		}

		//下行登录相关信息
		send_login_succ_uc(player);
		send_login_succ_bc(player);
		send_table_info_uc(player);

		xt_log.info("Func[%s] Login Table Succeed, uid[%d] money[%d] cur_players[%d] tid[%d] game_state[%d].\n", 
			        __FUNCTION__, player->uid, player->money, cur_players, tid, game_state);

		//当前状态WAIT状态，且玩家数>=2人，游戏状态为“准备”
		if((WAIT == game_state) && (cur_players >= 2))   
		{
			xt_log.info("Func[%s] *** 游戏准备 READY.\n", __FUNCTION__);

			//游戏状态为"准备"
			game_state = READY;

			//广播游戏准备
			send_game_ready_bc();

			start_ready_time = time(0);   //准备开始时刻	

			//启动“准备”定时器
			ev_timer_again(gBull.loop, &ready_timer);   
		}
		return 0;
	}
	return -1;
}

///广播游戏准备
int Table::send_game_ready_bc()
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_GAME_READY_BC;
	packet.val["state"] = READY;
	packet.end();

	broadcast(NULL, packet.tostring());
	return 0;

}


/**坐下,分配空闲坐位
   param: playrer 玩家
   return: 坐位号
*/
int Table::sit_down(Player *player)
{
	//空坐位列表
	std::vector<int> tmp;     
    for (int i = 0; i < MAX_CHAIR_COUNT; i++)
    {
		if (seats[i].status == STATUS_NULL)  ///空位
		{
			tmp.push_back(i);
		}
    }
	
	//有空位，随机选择空位
	int len = tmp.size();
	if (len > 0)    
	{
		int index = random(0, len - 1);
		int seat_id = tmp[index];

		xt_log.debug("Func[%s] select seatid[%d].\n", __FUNCTION__, seat_id);

		seats[seat_id].clearSeat();

		 //保存坐位号和玩家		
		seats[seat_id].uid    = player->uid;
		seats[seat_id].player = player;  
		seats[seat_id].status = STATUS_LOOK_ON;   //旁观者
		seats[seat_id].role   = ROLE_NULL;        //闲家
				
		player->seatid = seat_id;   //玩家座位ID
		player->tid    = tid;       //玩家桌子ID

		//保存这个玩家(一定要有位置)
		playersMap[player->uid] = player;

		cur_players++;      //累计当前玩家个数

		return seat_id;
	}

	return -1;
}



///下行登录成功应答
int Table::send_login_succ_uc(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGIN_SUCC_UC;
	packet.val["vid"] = player->vid;
	packet.val["tid"] = player->tid;

	//给客户端预期的倒计时
	packet.val["ready_time"]   = READY_TIME;  
	packet.val["banker_time"]  = BANKER_TIME;
	packet.val["group_time"]   = GROUP_TIME;
	packet.val["compare_time"] = COMPARE_TIME;

	packet.end();
	unicast(player, packet.tostring());

	//xt_log.debug("Func[%s] msg=%s.\n", 
	//	__FUNCTION__, packet.val.toStyledString().c_str());

	return 0;	
}

///下行登录成功广播（“我”的信息广播出去）
int Table::send_login_succ_bc(Player *player)
{
	Seat &seat = seats[player->seatid];

	Jpacket packet;
	packet.val["cmd"]    = SERVER_LOGIN_SUCC_BC;
	packet.val["seatid"] = player->seatid;
	packet.val["role"]   = seat.role;
	packet.val["status"] = seat.status;

	packet.val["uid"] = player->uid;
	packet.val["name"] = player->name;
	packet.val["sex"] = player->sex;
	packet.val["avatar"] = player->avatar;
	packet.val["exp"] = player->exp;
	packet.val["rmb"] = player->rmb;
	packet.val["money"] = player->money;
	packet.val["coin"] = player->coin;
	packet.val["total_board"] = player->total_board;
	packet.val["total_win"] = player->total_win;
	packet.val["pcount"] = player->pcount;
	packet.val["vlevel"] = player->vlevel;
	packet.val["ps"] = player->ps;      // cfc add by 20131218

	//++ 最大赢取，及最好的牌
	packet.val["max_win_money"]     = player->max_win_money;
	packet.val["best_board"]        = player->best_board;
	packet.val["best_board_detail"] = player->best_board_detail;

	packet.end();
	broadcast(player, packet.tostring());

	xt_log.debug("Func[%s] msg=%s.\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

	return 0;
}

//下行房间数据
int Table::send_table_info_uc(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_TABLE_INFO_UC;
	packet.val["vid"] = player->vid;        ///场馆id
	packet.val["tid"] = player->tid;        ///桌子的id   
	packet.val["seatid"] = player->seatid;  ///位置id
	packet.val["state"] = game_state;       ///状态

	int state_left_time = 0;
	switch(game_state)
	{
	case WAIT:
		state_left_time = 0;
		break;

	case READY:
		state_left_time = READY_TIME - (time(0) - start_ready_time);
		break;

	case BANKER:
		state_left_time = BANKER_TIME -(time(0) - start_banker_time);
		break;

	case SEND_CARD:
		state_left_time = GROUP_TIME -(time(0) - start_group_time);
		break;

	case COMPARE:
		state_left_time = COMPARE_TIME - (time(0) - start_compare_time);
		break;
	default:
		state_left_time = 0;
		break;
	}
	packet.val["state_left_time"] = state_left_time;

	packet.val["dealer"] = dealer;          ///庄家id
	packet.val["base_money"] = base_money;  ///底注
	packet.val["fee"] = fee;                 ///税

	std::map<int, Player*>::iterator it;    ///---玩家列表
	int i = 0;

	int bankerid = -1;

	for (it = playersMap.begin(); it != playersMap.end(); it++) 
	{
		Player *p = it->second;
		Seat &seat = seats[p->seatid];

		packet.val["players"][i]["uid"]     = p->uid;       ///玩家id
		packet.val["players"][i]["seatid"]  = p->seatid;    ///位置id
		packet.val["players"][i]["role"]    = seat.role;    ///是否是庄家
		if (seat.role > 0)
		{
			bankerid = seat.role;
		}        
		packet.val["players"][i]["status"]  = seat.status;  ///当前状态

		if (player == p) {
			///手面的牌(该字段只有是自己时，才有)s
			for (unsigned int j = 0; j < 5; j++) 
			{
				packet.val["players"][i]["holes"].append((int)seat.m_cbUserHandCard[j]);
			}
			packet.val["players"][i]["card_type"]=(int)seat.m_cardType;			
		}

		packet.val["players"][i]["group_card"] = seat.bPlayerChoiced ? 1 : 0;   //玩家是否组牌标记
		packet.val["players"][i]["name"] = p->name;
		packet.val["players"][i]["sex"] = p->sex;
		packet.val["players"][i]["avatar"] = p->avatar;
		packet.val["players"][i]["birthday"] = p->birthday;
		packet.val["players"][i]["zone"] = p->zone;
		packet.val["players"][i]["exp"] = p->exp;
		packet.val["players"][i]["rmb"] = p->rmb; 
		packet.val["players"][i]["money"] = p->money;
		packet.val["players"][i]["coin"] = p->coin;
		packet.val["players"][i]["total_board"] = p->total_board;
		packet.val["players"][i]["total_win"] = p->total_win;
		packet.val["players"][i]["pcount"] = p->pcount;

		packet.val["players"][i]["vlevel"] = p->vlevel;
		packet.val["players"][i]["ps"] = p->ps;  
		//++最大赢取、最好牌
		packet.val["players"][i]["max_win_money"]  = p->max_win_money;  //最大赢取 
		packet.val["players"][i]["best_board"]        = p->best_board ;     //最大牌类型
		packet.val["players"][i]["best_board_detail"] = p->best_board_detail;     //string#牌数据详情(0x01,0x02,0x03,0x04,0x05) 

		i++;
	}

	packet.val["dealer"] = bankerid;          ///庄家id(更正下)

	packet.end();
	unicast(player, packet.tostring());

	xt_log.debug("Func[%s] msg=%s.\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

	return 0;
}


//“准备”时间结束，进入选庄
void Table::ready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*)w->data;
	ev_timer_stop(gBull.loop, &table->ready_timer);

	//如果玩家人数大于等于两人，则游戏开始，进入随机选庄
	if (table->cur_players >= 2)   
	{		
		table->game_start();
	}
	else
	{
		//人数不够，游戏继续等待		
		xt_log.info("Func[%s] *** 人数不够，游戏进入 WAIT.\n", __FUNCTION__);
		table->game_state = WAIT; 

		//广播游戏等待
		Jpacket packet;
		packet.val["cmd"]   = SERVER_GAME_WAIT_BC;
		packet.val["state"] = WAIT;
		packet.end();
		table->broadcast(NULL, packet.tostring());

		xt_log.debug("Func[%s] 广播等待msg=%s.\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
}


//设置玩家状态为“游戏中” 
int Table::setPlayStatus(int bankSeatid)
{	
	bool hasBanker = false;  //有无庄家
	std::map<int, Player*>::iterator it;    ///---玩家列表
	for (it = playersMap.begin(); it != playersMap.end(); it++) 
	{
		Player *player = it->second;
		
		if (player != NULL)
		{
			Seat &seat = seats[player->seatid];
			seat.role = ROLE_NULL;

			if(seat.status != STATUS_NULL)
			{
				seat.status = STATUS_IN_GAME; 
				if (player->seatid == bankSeatid)
				{
					seat.role = ROLE_BANKER;
					hasBanker = true;
				}
			}
		}
	}

	if (!hasBanker)
	{
		dealer = -1;
	}
	return 0;
}




//游戏开始，进入随机选庄（执行扣税，广播选庄结果）
int Table::game_start()
{	
	//设置玩家状态
	setPlayStatus(dealer);   ///游戏开始，所有玩家置为玩游戏
	
	xt_log.debug("Func[%s] *** 游戏开始 cur_players[%d] BankerSeatId[%d] .\n",
		__FUNCTION__, cur_players, dealer);

	//扣税(抽水)
	pay_tax();

	if (dealer < 0)
	{
		//确定庄家
		make_banker();

		//广播选庄结果
		send_make_banker_bc();

		//游戏状态“选庄”
		game_state = BANKER;
		ev_timer_again(gBull.loop, &banker_timer);
		start_banker_time = time(0);  //选庄开始时刻	
	} 
	else 
	{
		xt_log.debug("Func[%s] *** 有庄家，直接SEND_CARD dealer[%d]\n", __FUNCTION__, dealer);
        game_state = SEND_CARD;
		//有庄家，直接发牌
		send_card();
		///进入发牌环节
		
		ev_timer_again(gBull.loop, &send_card_timer);
	}

	return 0;
}



//扣税（抽水）
int Table::pay_tax()
{
	std::map<int, Player*>::iterator it;    ///---玩家列表
	for (it = playersMap.begin(); it != playersMap.end(); it++) 
	{
		Player *player = it->second;

		if (player != NULL)
		{
			Seat &seat = seats[player->seatid];
			if (seat.status == STATUS_IN_GAME )
			{			
				xt_log.debug("Func[%s] 扣税抽水 seats[%d] uid[%d] fee[%d].\n", 
					__FUNCTION__, player->seatid, player->uid, fee);

				int value = -fee;
				player->incr_money(0, value);
				
			}
		}
	}	
	return 0;
}

///确定庄家
int Table::make_banker()
{
	//截止当前玩家个数
	int playCount = 0;
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		if (seats[i].status >= STATUS_IN_GAME
			&& seats[i].player != NULL)
		{
			playCount++;
		}
	}

	//随机挑个玩家
	int index = random(0, playCount -1);

	int tmpCount = -1;    ///--临时计数器

	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		if (seats[i].status >= STATUS_IN_GAME
			&& seats[i].player != NULL)
		{
			tmpCount++;

			if (tmpCount== index) 
			{
				xt_log.debug("Func[%s] 确定庄家uid[%d] BankerSeatId[%d].\n", 
					__FUNCTION__, seats[i].uid, i);

				//更新庄家和角色
				dealer = seats[i].seatid;    
				seats[i].role = ROLE_BANKER; 
				return 0;
			}

		}
	}

	xt_log.error("Func[%s] 选庄失败.\n",__FUNCTION__);

	return -1;   //没有选到庄家

}


///广播选庄结果
int Table::send_make_banker_bc()
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_MAKE_BANKER_BC;
	packet.val["state"]  = BANKER;	
	packet.val["dealer"] = dealer;  ///庄家坐位id

	std::map<int, Player*>::iterator it;    ///---玩家列表
	int i = 0;
	for (it = playersMap.begin(); it != playersMap.end(); it++) {
		Player *p = it->second;
		Seat &seat = seats[p->seatid];

		packet.val["players"][i]["uid"]     = p->uid;       ///玩家id
		packet.val["players"][i]["seatid"]  = p->seatid;    ///位置id
		packet.val["players"][i]["role"]    = seat.role;    ///是否是庄家
		packet.val["players"][i]["status"]  = seat.status;  ///当前状态
		packet.val["players"][i]["name"]    = p->name;
		packet.val["players"][i]["sex"]     = p->sex;
		packet.val["players"][i]["avatar"]  = p->avatar;
		packet.val["players"][i]["birthday"] = p->birthday;
		packet.val["players"][i]["zone"] = p->zone;
		packet.val["players"][i]["exp"] = p->exp;
		packet.val["players"][i]["rmb"] = p->rmb; 
		packet.val["players"][i]["money"] = p->money;
		packet.val["players"][i]["coin"] = p->coin;
		packet.val["players"][i]["total_board"] = p->total_board;
		packet.val["players"][i]["total_win"] = p->total_win;
		packet.val["players"][i]["pcount"] = p->pcount;
		packet.val["players"][i]["vlevel"] = p->vlevel;
		packet.val["players"][i]["ps"] = p->ps;   
		packet.val["players"][i]["max_win_money"]  = p->max_win_money;  //最大赢取 
		packet.val["players"][i]["best_board"] = p->best_board ;     //最大牌类型
		packet.val["players"][i]["best_board_detail"] = p->best_board_detail;     //string#牌数据详情(0x01,0x02,0x03,0x04,0x05) 

		i++;
	}

	packet.end();
	broadcast(NULL, packet.tostring());

	//xt_log.debug("Func[%s] 广播选庄msg=%s .\n", 
	//	__FUNCTION__, packet.val.toStyledString().c_str());

	return 0;
} 



///"选庄"时间完毕， 发牌
void Table::banker_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	xt_log.debug("Func[%s] *** 游戏发牌 SEND_CARD.\n", __FUNCTION__);

	Table *table = (Table*) w->data;
	ev_timer_stop(gBull.loop, &table->banker_timer);


	///进入发牌环节
	table->game_state = SEND_CARD;

	///给用户发牌
	table->send_card();

	ev_timer_again(gBull.loop, &table->send_card_timer);
}

///随机庄家时间结束，发牌
void Table::send_card()
{
	///记录组牌开始时间
	start_group_time = time(0);

	//1先组好牌
	_uint8 GoodCardBuff[GAME_PLAYER_COUNT][MAX_COUNT];  //5手牌         
	memset(GoodCardBuff, 0, sizeof(GoodCardBuff));
	
	_uint8 GoodChairId[GAME_PLAYER_COUNT] = { -1 }; //好牌的椅子
	
	_uint8 bRandCard[CARDCOUNT] = { 0 };       //52张牌                  
	memset(bRandCard, 0, sizeof(bRandCard));

	int bGoodsCardSwtich = 0;   ///先写死

	//洗牌逻辑
	if (bGoodsCardSwtich == 1)
	{
		//统计每个玩家的局数
		int count[GAME_PLAYER_COUNT] = { -1 };
		bool is_robot[GAME_PLAYER_COUNT] = { false };
		for (int i = 0; i < GAME_PLAYER_COUNT; i++)
		{
			if (seats[i].status < STATUS_IN_GAME)
				continue;

			count[i] = seats[i].player->total_board; ///总的对局数
			if (seats[i].player->uid < 10000)
			{
				is_robot[i] = true;
			}
		}

		//洗好牌
		m_GameLogic->GoodRandCardList(count, is_robot, bRandCard, CARDCOUNT, GAME_PLAYER_COUNT, GoodCardBuff, GoodChairId);
	}
	else
	{
		//随机洗
		memcpy(bRandCard, m_cbCardData, CARDCOUNT);
		m_GameLogic->RandCardList(bRandCard, CARDCOUNT);
	}

	// 2为每个玩家下行发牌(如果玩家是旁观，则没有手牌)
	int len = 0;
	std::map<int, Player*>::iterator it;    ///---玩家列表
	for (it = playersMap.begin(); it != playersMap.end(); it++) 
	{
		Player *player = it->second;
		Seat &seat = seats[player->seatid];


		//跳过空位置(或者没人)
		if (STATUS_NULL == seat.status 
			/*|| NULL == player*/)
		{
			continue;
		}

		if (seat.status >= STATUS_IN_GAME)
		{
			//好牌
			if (bGoodsCardSwtich == 1 /*&& GoodChairId[i] == i*/)
			{
				memcpy(seat.m_cbUserHandCard, GoodCardBuff[0], MAX_HAND_CARD_COUNT);
			}
			else
			{
				memcpy(seat.m_cbUserHandCard, bRandCard + len, MAX_HAND_CARD_COUNT);
				len += MAX_HAND_CARD_COUNT;
			}

			//把牌也组了(并更新了玩家最大牌的记录)
			group_cards(seat);
		} 
		else 
		{
			//旁观者手牌为空
			memset(seat.m_cbUserHandCard, 0, sizeof(seat.m_cbUserHandCard));
			seat.m_cardType = CT_NOBULL;
		}


		//下行发牌给客户端
		Jpacket packet;
		packet.val["cmd"] = SERVER_SEND_CARD_UC;   
		packet.val["state"] = SEND_CARD;
		//牌数据
		for ( int k = 0; k < MAX_HAND_CARD_COUNT; k++) {
			packet.val["holes"].append(seat.m_cbUserHandCard[k]);
		}
		packet.val["card_type"] = seat.m_cardType;
		packet.val["seatid"] = seat.seatid;
		packet.val["uid"] = seat.uid;
		packet.end();

		unicast(player, packet.tostring());

		xt_log.info("Func[%s] send card msg= %shold_card= [%02x %02x %02x %02x %02x]\n", 
			__FUNCTION__, packet.val.toStyledString().c_str(),
			seat.m_cbUserHandCard[0], seat.m_cbUserHandCard[1],seat.m_cbUserHandCard[2], seat.m_cbUserHandCard[3], seat.m_cbUserHandCard[4]);
	}
}

///组牌
void Table::group_cards(Seat &seat)
{
	seat.m_cardType = (CT_BULL_TYPE)m_GameLogic->GetCardTypeAndData(
		seat.m_cbUserHandCard,  MAX_HAND_CARD_COUNT, seat.m_cbChoiceCard);

	//更新玩家历史最大牌
	Player * player = seat.player;
	if (player != NULL)
	{
		char szCard[20] = {0}; //字符串
		sprintf(szCard, "%02x,%02x,%02x,%02x,%02x", 
			seat.m_cbChoiceCard[0], 
			seat.m_cbChoiceCard[1], 
			seat.m_cbChoiceCard[2], 
			seat.m_cbChoiceCard[3], 
			seat.m_cbChoiceCard[4]);

		int cardType = (int)seat.m_cardType;

		player->update_best_board(cardType, szCard);	
	}

}




///"发牌"时间结束， 进入比牌
void Table::send_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*) w->data;
	ev_timer_stop(gBull.loop, &table->send_card_timer);

	xt_log.debug("Func[%s] *** 游戏比牌 COMPARE.\n", __FUNCTION__);

	//比牌
	table->compare_card();

	//游戏进入比牌
	table->game_state = COMPARE;
	ev_timer_again(gBull.loop, &table->compare_timer);	
	
	table->start_compare_time = time(0); //比牌开始时刻

}


///计算下一庄家(闲家拿到牛牛，就要考虑换庄了)
int Table::next_banker()
{
	//闲家有没拿到牛牛的（取钱最多者）
	int tmp_money = 0;
	int bull_seat = -1;  //牛牛牌位置
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{				
		if (i != dealer 
			&& seats[i].status == STATUS_IN_GAME
			&& seats[i].m_cardType == CT_BULL_BULL
			&& seats[i].player != NULL)
		{					
			if (seats[i].player->money > tmp_money)
			{
				bull_seat = i;
				tmp_money = seats[i].player->money;
			}
		}
	}

	//上局庄家还在
	if (dealer >= 0 && dealer < MAX_CHAIR_COUNT 
		&& seats[dealer].status == STATUS_IN_GAME		
		&& seats[dealer].player != NULL)
	{
		///闲家拿到牛牛，庄家没拿到牛牛，换庄吧
		if (bull_seat >= 0)
		{
			if (seats[dealer].m_cardType != CT_BULL_BULL)
			{
				xt_log.debug("Func[%s] 有闲家拿到牛牛了，nextBanker[%d].\n", __FUNCTION__, bull_seat);	
				dealer = bull_seat;
			}
		}	
	}
	else
	{
		//没庄家
		dealer = bull_seat;
	}
		
	xt_log.debug("Func[%s], 继续上局庄家，若没有，则随机选吧 dealer[%d].\n", __FUNCTION__, dealer);
	return dealer;   //继续当庄家
}



//计算与庄家组牌时间差（闲家时间 - 庄家时间）
int Table::CalGroupTimeDiff()
{
	if (dealer >= 0 && dealer < MAX_CHAIR_COUNT)
	{	
		xt_log.debug("庄家[%d]组牌时间[%d].\n", 
			seats[dealer].uid, seats[dealer].goup_time);

		for (int i = 0; i < MAX_CHAIR_COUNT; i++)
		{
			if (seats[i].status >= STATUS_IN_GAME
				&& i != dealer)
			{
				//时间差
				seats[i].group_time_diff =  seats[i].goup_time - seats[dealer].goup_time;

				xt_log.debug("闲家[%d]组牌时间[%d] 与庄家时间差[%d].\n", 
					seats[i].uid, seats[i].goup_time, seats[i].group_time_diff);
			}
		}
		return 0;
	} else {
		xt_log.error("庄家位置ID非法seat[%d]\n", dealer);
		return -1;
	}
}




///比牌,结算
int Table::compare_card()
{
	//计算下组牌时间
	CalGroupTimeDiff();

	int GameGold[GAME_PLAYER_COUNT]      = { 0 };         //每个玩家输赢的金币          
	enEndKind EndKind[GAME_PLAYER_COUNT] = { KIND_NULL }; //输赢类型

	_tint32 BankerWinResult  = 0; //庄家赢结果
	_tint32 BankerLoseResult = 0; //庄家输结果

	Player *pBankerUser = seats[dealer].player;
	if (NULL == pBankerUser)
	{
		xt_log.error("Func[%s] 错误，比牌没庄家player了,seat_id[%d]\n", __FUNCTION__, dealer);
		return -1;
	}

	///庄家牌型倍数
	int bankerCardTimes = m_GameLogic->GetTimes(seats[dealer].m_cbUserHandCard, MAX_HAND_CARD_COUNT);

	xt_log.info("Func[%s] 比牌结算cur_players[%d]\n", __FUNCTION__, cur_players);
	//计算庄家输赢数目
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		//没玩游戏的人
		if (seats[i].status < STATUS_IN_GAME 
			|| i == dealer
			|| seats[i].player == NULL)
		{
			continue;
		}


		//庄家赢了
		if (m_GameLogic->CompareCard(
			seats[dealer].m_cbChoiceCard, 
			seats[i].m_cbChoiceCard, MAX_HAND_CARD_COUNT, 0, 0))
		{

			Player *player = seats[i].player; //闲家

			//时间权值double floor( double arg );
			double timeWeight = 1 + (double)2*seats[i].group_time_diff/100;

			GameGold[i]  = - int(ceil(bankerCardTimes * base_money * timeWeight));  ///赢方牌型倍数*底注*时间加成

			xt_log.info("闲家uid[%d]输[%d]  bankerCardTimes=[%d] base_money[%d] timeWeight[%f].\n", 
				seats[i].uid, GameGold[i], bankerCardTimes, base_money, timeWeight);
			///闲家不够输时，输光
			if (abs(GameGold[i]) > player->money)
			{
				GameGold[i] = -player->money;
				xt_log.info("不够输 GameGold[%d]=%d\n", i, GameGold[i]);
			}

			BankerWinResult = BankerWinResult - GameGold[i];   ///累计庄家赢的
			EndKind[i] = KING_LOSE;    ///此人输						
		}
		else 
		{
			//闲家赢，赢方牌型倍数
			int otherCardTimes = m_GameLogic->GetTimes(seats[i].m_cbUserHandCard, MAX_HAND_CARD_COUNT);


			//时间权值
			double timeWeight = 1 - (double)(2*seats[i].group_time_diff)/100;

			GameGold[i]  = int (ceil(otherCardTimes * base_money * timeWeight));  ///赢方牌型倍数*底注*时间加成

			//GameGold[i]  = otherCardTimes * base_money *(1 + 2*seats[i].group_time_diff/100);  ///赢方牌型倍数*底注*时间加成

			BankerLoseResult += GameGold[i];  //庄家输的总和
			EndKind[i] = KIND_WIN; //此人赢

			xt_log.info("闲家[%d]赢[%d] otherCardTimes=[%d] base_money[%d] timeWeight[%f].\n", 
				seats[i].uid, GameGold[i], otherCardTimes, base_money, timeWeight);

		}

		//保存下输赢类型
		seats[i].end_kind = EndKind[i];
	}

	xt_log.debug("庄家赢总数[%d]\n", BankerWinResult);

	//计算庄家输，庄家不够输，按比例算
	int bankerMoney =  pBankerUser->money + BankerWinResult;
	if (bankerMoney < BankerLoseResult)
	{
		xt_log.debug("庄家不够输.\n");
		int lostResult = BankerLoseResult;

		BankerLoseResult = 0;  ///重新置0
		for (int i = 0; i < MAX_CHAIR_COUNT; i++)
		{
			if (seats[i].status < STATUS_IN_GAME 
				|| i == dealer 
				|| seats[i].end_kind != KIND_WIN)
			{
				continue;
			}

			//闲家按比例赢这么多
			int GoldResult = bankerMoney * (GameGold[i]/lostResult);
			GameGold[i] = GoldResult;

			if ((bankerMoney - BankerLoseResult - 1) <= GoldResult)  ///多出的一块，给最后一位，庄家输光
			{
				GameGold[i] = bankerMoney - BankerLoseResult;
				xt_log.debug("最后一位吧BankerLoseResult[%d]\n", BankerLoseResult);
			} 

			BankerLoseResult += GameGold[i];   ///累计庄家输的
		}
	} 

	xt_log.debug("庄家输总数[%d]\n", BankerLoseResult);

	//庄家最后输赢的金币
	int bankerResult = BankerWinResult - BankerLoseResult;
	GameGold[dealer] = bankerResult;
	seats[dealer].end_kind = (bankerResult > 0 ) ? KIND_WIN : KING_LOSE;  ///记录庄家的输赢类型

	xt_log.debug("庄家最后输赢[%d]\n", bankerResult);

	//更新玩家金币
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		if (seats[i].status >= STATUS_IN_GAME)
		{
			Player *player = seats[i].player;
			if (player != NULL)
			{
				player->incr_money(0, GameGold[i]);        //更新金币
				player->update_max_win_money(GameGold[i]); //更新最大赢取			
			}
		}
	}


	///广播比牌结果
	Jpacket packet;
	packet.val["cmd"] = SERVER_COMPARE_CARD_BC;

	int k = 0;
	for (int i = 0; i < MAX_CHAIR_COUNT; i++) 
	{		
		Seat &seat = seats[i];
		Player *p = seat.player;

		if (seat.status >= STATUS_IN_GAME 
			&& seat.player != NULL)
		{
			packet.val["players"][k]["uid"]    = p->uid;      ///玩家id
			packet.val["players"][k]["seatid"] = seat.seatid; ///位置id
			packet.val["players"][k]["role"]   = seat.role;   ///是否是庄家
			packet.val["players"][k]["status"] = seat.status; //状态
			
			//组好的手牌
			for (unsigned int j = 0; j < MAX_HAND_CARD_COUNT; j++) {
				packet.val["players"][k]["holes"].append(seat.m_cbChoiceCard[j]);
			}
			packet.val["players"][k]["card_type"] = seat.m_cardType;

			packet.val["players"][k]["name"]        = p->name;
			packet.val["players"][k]["sex"]         = p->sex;
			packet.val["players"][k]["avatar"]      = p->avatar;
			packet.val["players"][k]["birthday"]    = p->birthday;
			packet.val["players"][k]["zone"]        = p->zone;
			packet.val["players"][k]["exp"]         = p->exp;
			packet.val["players"][k]["rmb"]         = p->rmb; 
			packet.val["players"][k]["money"]       = p->money;
			packet.val["players"][k]["coin"]        = p->coin;
			packet.val["players"][k]["total_board"] = p->total_board;
			packet.val["players"][k]["total_win"]   = p->total_win;
			packet.val["players"][k]["pcount"]      = p->pcount;
			packet.val["players"][k]["vlevel"]      = p->vlevel;
			packet.val["players"][k]["ps"]          = p->ps;   

			packet.val["players"][k]["win_money"]         = GameGold[i];       //本局输赢金币（正数赢，负数为输）
			packet.val["players"][k]["max_win_money"]     = p->max_win_money;  //最大赢取 
			packet.val["players"][k]["best_board"]        = p->best_board ;     //最大牌类型
			packet.val["players"][k]["best_board_detail"] = p->best_board_detail;     //string#牌数据详情(0x01,0x02,0x03,0x04,0x05) 


			///[+++ 为了测试定位
			char szCard[64] = {0}; //字符串
			sprintf(szCard, "%02x,%02x,%02x,%02x,%02x,%d,%d", 
				seat.m_cbChoiceCard[0], 
				seat.m_cbChoiceCard[1], 
				seat.m_cbChoiceCard[2], 
				seat.m_cbChoiceCard[3], 
				seat.m_cbChoiceCard[4],
				GameGold[i],
				seat.role);
			packet.val["players"][k]["hex_hole_win_role"]=szCard;
			///+++]

			k++;
		}
	}
	packet.end();

	broadcast(NULL, packet.tostring());

	xt_log.debug("Func[%s] msg=%s.\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());
	return 0;
}

/**
   用户上行组牌(校验牌、记录组牌时间)
*/
int Table::handler_group_card(Player *player)
{
	int errCode = 0;   //错误码

	if (game_state == SEND_CARD)
	{
		//判断下坐位
		int seatid = player->seatid;

		if (seatid < 0 && seatid >= MAX_CHAIR_COUNT)
		{
			xt_log.error("Func[%s] seatid[%d]", __FUNCTION__, seatid);
			return -1;
		}

		seats[seatid].bPlayerChoiced = true;                  //玩家组牌标记		
		seats[seatid].goup_time = time(0) - start_group_time; //组牌所花时间


		//组牌数据
		Json::Value &val = player->client->packet.tojson();

		int cardCount = val["holes"].size();
		if (cardCount >= 3)
		{
			int i = 0;
			xt_log.debug("用户uid[%d]组牌[%02x,%02x,%02x] 所花时间[%d]\n", 
				seats[seatid].uid,
				val["holes"][i++].asInt(),
				val["holes"][i++].asInt(),
				val["holes"][i++].asInt(),
				seats[seatid].goup_time);
		}

		//取出牌
		for ( int i = 0; i < cardCount; i++)
		{
			unsigned char card = (unsigned char)val["holes"][i].asInt();

			bool isExist = false;

			for (int k = 0; k < MAX_HAND_CARD_COUNT; k++)
			{
				if (card == seats[seatid].m_cbUserHandCard[k])
				{
					isExist = true;
					break;
				}
			}

			if (!isExist)
			{
				xt_log.error("Func[%s] 非法牌数据 card[%d].\n", __FUNCTION__, card);
				errCode = -3;
				break;
			}
		}

		
		//组牌应答
		Jpacket packetUC;
		packetUC.val["cmd"] = SERVER_GROUP_CARD_UC;
		packetUC.val["code"] = errCode;
		packetUC.end();
		unicast(player, packetUC.tostring());

		//广播组牌
		Jpacket packet;
		packet.val["cmd"] = SERVER_GROUP_CARD_BC;
		packet.val["seatid"] = player->seatid;
		packet.val["uid"]    = player->uid;
		packet.end();
		broadcast(player, packet.tostring());
		
		//若所有的人都组了牌，则马上比牌
		if (isAllPlayerChoiced())
		{
			ev_timer_stop(gBull.loop, &send_card_timer);

			//比牌
			compare_card();

			//游戏进入比牌
			game_state = COMPARE;
			ev_timer_again(gBull.loop, &compare_timer);	

			start_compare_time = time(0); //比牌开始时刻
		}

	} 
	else
	{
		xt_log.error("Func[%s] error game_state[%d]", __FUNCTION__, game_state);
	}

	return errCode;
}

bool Table::isAllPlayerChoiced() 
{
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		if (seats[i].status >= STATUS_IN_GAME 
			&& seats[i].player != NULL)
		{
			if (!seats[i].bPlayerChoiced)
			{
				return false;
			}

		}
	}
	return true;
}



///"比牌"时间结束, 游戏结束，再次进入等待
void Table::compare_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	//xt_log.debug("Func[%s] *** 游戏等待 WAIT.\n", __FUNCTION__);

	Table *table = (Table*) w->data;
	ev_timer_stop(gBull.loop, &table->compare_timer);

	//游戏结束,进入等待
	table->game_state = WAIT;

	//清场预处理
	table->pre_ready();
}


///游戏结束，清场处理(并检查玩家是否满足继续游戏的条件，)
int Table::pre_ready()
{	
	//累计玩家空闲次数(没有组牌的玩家)
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{		
		if (STATUS_IN_GAME == seats[i].status 			
			&& seats[i].player != NULL)
		{
			if (seats[i].bPlayerChoiced) 
			{
				seats[i].player->idle_count = 0;
			} 
			else 
			{
				seats[i].player->idle_count++;
			}		
		}
	}


	/* remove idle and offline player */
	std::map<int, Player*> temp;
	std::map<int, Player*>::iterator it;
	for (it = playersMap.begin(); it != playersMap.end(); it++) {

		Player *player = it->second;
		if (player->client != NULL) 
		{
			player->reset();

			if (player->money < min_money)    ///当前金币<底限
			{
				if (player->uid < XT_ROBOT_UID_MAX)  ///若为机器人,随机送钱[200, 375]*min_money
				{
					player->set_money((int)(float(rand()%8+8)/4*100*min_money));
				}
				else 
				{
					temp[player->uid]=player;   ///记录钱不够的玩家
				}
			}

			//系统代打20局(测试用上线后要改)
			if (player->idle_count > 20)         ///记录无动作的玩家 if (player->idle_count > 2)
			{
				temp[player->uid] = player;
				player->logout_type = 1;        
			}

			//主动退出的用户
			if (player->stand_up > 0)
			{
				temp[player->uid] = player;
				player->logout_type = 3;   //玩家主动退出
			}
		} 
		else 
		{
			xt_log.debug("Func[%s] handler wait uid[%d] is offline.\n", __FUNCTION__, player->uid);
			temp[player->uid] = player;     ///记录离线玩家
			player->logout_type = 0;
		}
	}

	///主动退出的玩家
	//for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	//{		
	//	if (STATUS_STAND_UP == seats[i].status 			
	//		&& seats[i].player != NULL)
	//	{
	//		temp[seats[i].player->uid] = seats[i].player;
	//		seats[i].player->logout_type = 3;  //玩家主动退出
	//	}
	//}

	//删除不合条件的玩家
	for (it = temp.begin(); it != temp.end(); it++) 
	{
		Player *player = it->second;
		gBull.game->del_player(player);
	}


	//计算下一庄家
	dealer = next_banker();

	/* reset seat*/
	for (int i = 0; i < MAX_CHAIR_COUNT; i++) {
		seats[i].resetSeat();
	}

    xt_log.info("Func[%s] *** game_state[%d] cur_players[%d].\n", __FUNCTION__, game_state, cur_players);
	/* check state */
	if (WAIT == game_state && cur_players >= 2)
	{
		xt_log.info("Func[%s] ***  游戏准备 READY cur_players[%d].\n", __FUNCTION__, cur_players);
		game_state = READY;     
		ev_timer_again(gBull.loop, &ready_timer);
		send_game_ready_bc();
	} 
	else
	{
		game_state = WAIT;

		xt_log.info("Func[%s] *** 游戏等待 WAIT .\n", __FUNCTION__);
		Jpacket packet;
		packet.val["cmd"] = SERVER_GAME_WAIT_BC;
		packet.val["state"] = WAIT;
		packet.end();
		broadcast(NULL, packet.tostring());   ///广播所有人，游戏预备
	}
	

	return 0;
}





///游戏结束
int Table::game_end()
{

	return 0;
}

///---处理登出
int Table::handler_logout(Player *player)
{	
	//如果此人在玩牌，游戏正在进行中，那暂不能退出
	Seat &seat = seats[player->seatid];

	if ((seat.status  >= STATUS_IN_GAME) 
		&& (game_state == BANKER
		|| game_state == SEND_CARD
		|| game_state == COMPARE) )
	{
		
		seat.status = STATUS_STAND_UP;
		player->stand_up = 1;  //离开桌子

		xt_log.info("Func[%s] in game uid[%d] seatid[%d] money[%d] tid[%d].\n", 
			__FUNCTION__, player->uid, player->seatid, player->money, tid);
		return -1;
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGOUT_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["type"] = player->logout_type;
	packet.end();
	broadcast(NULL, packet.tostring());   ///---广播“玩家退出”

	return 0;
}

///---发聊天
int Table::handler_chat(Player *player)
{
	Json::Value &val = player->client->packet.tojson();

	Jpacket packet;
	packet.val["cmd"] = SERVER_CHAT_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["text"] = val["text"];
	packet.val["chatid"] = val["chatid"];
	packet.end();
	broadcast(NULL, packet.tostring());    ///---广播“聊天消息”

	return 0;
}

///发表情
int Table::handler_face(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	int faceid = val["faceid"].asInt();

	Jpacket packet;
	packet.val["cmd"] = SERVER_FACE_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["faceid"] = faceid;
	packet.end();
	broadcast(NULL, packet.tostring());

	return 0;
}


///机器人定时器回调
void Table::robot_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*) w->data;
	ev_timer_stop(gBull.loop, &table->robot_timer);
	xt_log.info("robot timer cb.\n");

	if (table->cur_players != 1) {
		return;
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_ROBOT_NEED_UC;
	packet.val["number"] = 1;
	packet.end();
	if (gBull.game->robot_client != NULL)
		gBull.game->robot_client->send(packet.tostring());
}

void Table::lose_exp_update(Player *player)
{
	player->incr_exp(lose_exp);
}

void Table::win_exp_update(Player *player)
{
	player->incr_exp(win_exp);
}

///把单个玩家信息广播出去
int Table::handler_info(Player *player)
{
	int ret = 0;
	ret = player->update_info();    ///更新玩家信息

	if (ret < 0) 
	{
		xt_log.error("handler info error update info error.\n");
		return -1;
	}

	xt_log.info("handler info uid[%d] money[%d].\n", player->uid, player->money);


	Jpacket packet;
	packet.val["cmd"] = SERVER_UPDATE_INFO_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["money"] = player->money;
	packet.val["rmb"] = player->rmb;
	packet.val["exp"] = player->exp;
	packet.val["coin"] = player->coin;
	packet.val["total_board"] = player->total_board;
	packet.val["total_win"] = player->total_win;
	packet.end();
	broadcast(NULL, packet.tostring());   ///广播玩家信息

	xt_log.info("handler info uid[%d] seatid[%d] money[%d] tid[%d].\n", player->uid, player->seatid, player->money, tid);

	return 0;
}




///---处理互动表情
int Table::handler_interaction_emotion(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	int seatid = val["seatid"].asInt();
	int target_seatid = val["target_seatid"].asInt();
	int type = val["type"].asInt();

	Jpacket packet;
	packet.val["cmd"] = SERVER_EMOTION_BC;
	packet.val["seatid"] = player->seatid;
	packet.val["money"] = player->money;
	packet.val["target_seatid"] = target_seatid;
	packet.val["type"] = type;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler interaction emotion uid[%d] seatid[%d] money[%d] seat.bet[%d] target_seatid[%d] type[%d] tid[%d].\n",
			player->uid, seatid, target_seatid, type, tid);
	return 0;
}


///站起，则清空坐位
void Table::stand_up(Player *player)
{
	seats[player->seatid].clearSeat();

	xt_log.debug("Func[%s] seatid[%d].\n", __FUNCTION__, player->seatid);
}

///删除玩家
int Table::del_player(Player *player)
{
	xt_log.debug("Func[%s] uid[%d]..\n", __FUNCTION__, player->uid);

	//没有找到此玩家,返回
	if (playersMap.find(player->uid) == playersMap.end()) {		
		xt_log.error("del player talbe uid[%d] is error.\n", player->uid);
		return -1;
	}

    //停止"离线定时器"
	player->stop_offline_timer();    

	//清除此玩家
	playersMap.erase(player->uid);

	//清除庄位dealer
	if (dealer == player->uid)
	{
		dealer = -1;
	}

	//清空坐位
	stand_up(player);

	cur_players--;   //更新当前玩家人数

	return 0;
}



///对所有玩家广播(除了玩家p,如果是p=NULL,则是所有的人)
int Table::broadcast(Player *p, const std::string &packet)
{
	Player *player;
	std::map<int, Player*>::iterator it;
	for (it = playersMap.begin(); it != playersMap.end(); it++)
	{
		player = it->second;
		if (player == p 
			|| player->client == NULL)
		{
			continue;
		}
		player->client->send(packet);
	}

	return 0;
}

///单播
int Table::unicast(Player *p, const std::string &packet)
{
	if (p != NULL && p->client != NULL)
	{
		return p->client->send(packet);
	}
	return -1;
}

///[starr, end]之间的随机数
int Table::random(int start, int end)
{
	return start + rand() % (end - start + 1);
}




///vector转JSON数组
//void Table::vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key)
//{
//	for (unsigned int i = 0; i < cards.size(); i++) {
//		packet.val[key].append(cards[i].m_value);
//	}
//
//	if (cards.size() == 0) {
//		packet.val[key].append(0);
//	}
//}
//
/////map转JSON数组
//void Table::map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key)
//{
//	std::map<int, XtCard>::iterator it;
//	for (it = cards.begin(); it != cards.end(); it++)
//	{
//		XtCard &card = it->second;
//		packet.val[key].append(card.m_value);
//	}
//}
//
/////JSON数组转vector
//void Table::json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key)
//{
//	Json::Value &val = packet.tojson();
//
//	for (unsigned int i = 0; i < val[key].size(); i++)
//	{
//		XtCard card(val[key][i].asInt());
//
//		cards.push_back(card);
//	}
//}


