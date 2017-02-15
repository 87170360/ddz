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

#include "zjh.h"
#include "game.h"
#include "log.h"
#include "table.h"
#include "client.h"
#include "player.h"
#include "proto.h"
#include "XtCard.h"

extern ZJH zjh;
extern Log xt_log;
///---（匹配→抽水→随机选庄→发5张牌→配牌→与庄家比牌→结算）
SeeTable::SeeTable() :
  ready_timer_stamp(5),              ///1准备时间（匹配人）
  banker_timer_stamp(2),             ///2随机选庄时间
  send_card_timer_stamp(2),          ///3发牌
  group_card_timer_stamp(GROUP_TIME),///4组牌
  compare_timer_stamp(7),            ///5比牌时间   
  robot_timer_stamp(6)               ///机器人时间  
  preready_timer_stamp(2),           ///预备时间
{
 	///1准备（这个时间可以有玩家进来或者离开）
    ready_timer.data = this;
    ev_timer_init(&ready_timer, SeeTable::ready_timer_cb, ready_timer_stamp, ready_timer_stamp);

	///2随机庄家
	banker_timer.data = this;
	ev_timer_init(&banker_timer, SeeTable::banker_timer_cb, banker_timer_stamp, banker_timer_stamp);

	///3发牌
	send_card_timer.data = this;
	ev_timer_init(&send_card_timer, SeeTable::send_card_timer_cb, send_card_timer_stamp,
		send_card_timer_stamp);

	///4组牌
	group_card_timer.data = this;
	ev_timer_init(&group_card_timer, SeeTable::group_card_timer_cb, group_card_timer_stamp,
		group_card_timer_stamp);

	///5比牌
    compare_timer.data = this;
    ev_timer_init(&compare_timer, SeeTable::compare_timer_cb, compare_timer_stamp,
                    compare_timer_stamp);

	///6预备处理（清空为下一轮回开始）
	preready_timer.data = this;
	ev_timer_init(&preready_timer, SeeTable::preready_timer_cb, preready_timer_stamp,
		preready_timer_stamp);

    robot_timer.data = this;
    ev_timer_init(&robot_timer, SeeTable::robot_timer_cb, robot_timer_stamp,
    		robot_timer_stamp);


	int weight[6]={52*1.5,48*1.5,1096*1.5,720*1.5,3744*1.5,16440/2}; ///---定义权重值
	deck.setTypeWeight(weight);     ///---设置每种牌型的权重
}

SeeTable::~SeeTable()
{
	ev_timer_stop(zjh.loop, &ready_timer);
	ev_timer_stop(zjh.loop, &banker_timer);
	ev_timer_stop(zjh.loop, &send_card_timer);
	ev_timer_stop(zjh.loop, &group_card_timer);
	ev_timer_stop(zjh.loop, &compare_timer);
	ev_timer_stop(zjh.loop, &preready_timer);
	ev_timer_stop(zjh.loop, &robot_timer);
}

///桌子初始化
int SeeTable::init(int my_tid,     
				int my_vid, 
				int my_min_money,
				int my_max_money, 
				int my_base_money, 
				int my_fee,
				int my_lose_exp, 
				int my_win_exp)
{
	// xt_log.debug("begin to init table [%d]\n", table_id);
	tid = my_tid;                ///桌号
    vid = my_vid;                ///场馆号（高级、中级、低能等）

    min_money = my_min_money;    ///携带金币下限
	max_money = my_max_money;    ///携带金币下限
	base_money = my_base_money;  ///底注
 
	fee = my_fee;                ///抽水费率

	lose_exp = my_lose_exp;      ///输的经验值
	win_exp = my_win_exp;        ///赢的经验值
	seat_max = 5;                ///座位数
	
    cur_players = 0;             ///当前玩家个数
	players.clear();             ///清空玩家map
	ready_players = 0;           ///已准备好的玩家数

	///坐位id为数组下标
    for (int i = 0; i < seat_max; i++)
    {
		seats[i].clear();
        seats[i].seatid = i;   
    }
	
	reset();
	state = WAIT;             ///初始当前状态为“等待”WAIT

    return 0;
}


///对所有玩家广播(除了玩家p,如果是p=NULL,则是所有的人)
int SeeTable::broadcast(Player *p, const std::string &packet)
{
    Player *player;
    std::map<int, Player*>::iterator it;
    for (it = players.begin(); it != players.end(); it++)
    {
        player = it->second;
        if (player == p || player->client == NULL)
        {
            continue;
        }
        player->client->send(packet);
    }

    return 0;
}

///单播
int SeeTable::unicast(Player *p, const std::string &packet)
{
    if (p->client)
    {
        return p->client->send(packet);
    }
    return -1;
}

///[starr, end]之间的随机数
int SeeTable::random(int start, int end)
{
	return start + rand() % (end - start + 1);
}

///桌子状态复位
void SeeTable::reset()   
{
	state = READY;
	ready_players = 0;
}

///vector转JSON数组
void SeeTable::vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
	for (unsigned int i = 0; i < cards.size(); i++) {
		packet.val[key].append(cards[i].m_value);
	}

	if (cards.size() == 0) {
		packet.val[key].append(0);
	}
}

///map转JSON数组
void SeeTable::map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key)
{
	std::map<int, XtCard>::iterator it;
	for (it = cards.begin(); it != cards.end(); it++)
	{
		XtCard &card = it->second;
		packet.val[key].append(card.m_value);
	}
}

///JSON数组转vector
void SeeTable::json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
	Json::Value &val = packet.tojson();
	
	for (unsigned int i = 0; i < val[key].size(); i++)
	{
		XtCard card(val[key][i].asInt());
		
		cards.push_back(card);
	}
}


/**
   用户组牌  
*/
int SeeTable::handler_group_card(Player *player)
{
	///---组牌所花费的时间
	seats[player->seatid].goup_time = time(0) - start_group_time;

	///---记录玩家的组牌信息
	if (players.find(player->uid) == players.end()) {
		players[player->uid] = player;
		player->tid = tid;

		/// 分配座位
		player->seatid = sit_down(player);
		if (player->seatid < 0) {
			return -1;    ///---坐下失败返回
		}
		
		Seat &seat = seats[player->seatid];
		seat.uid = player->uid;
		cur_players++;      ///---累计当前玩家个数

		handler_login_succ_uc(player);
		handler_login_succ_bc(player);
		handler_table_info(player);

		xt_log.info("handler login succ uid[%d] money[%d] cur_players[%d] tid[%d] state[%d]. \n", player->uid, player->money, cur_players, tid,state);

		///---当前状态WAIT状态，且玩家数>=2人，转为准备状态
		if((WAIT == state) && (cur_players>=2))   
		{
			xt_log.info("change state to ready \n");
			state = READY;
			ev_timer_again(zjh.loop, &ready_timer);   ///---启动“准备”定时器
		}
		return 0;
	}

	///---如果所玩家组牌完毕，停掉组牌定时器，进入比牌阶段
	if (false)
	{
		ev_timer_stop(zjh.loop, &table->group_card_timer);
		///进入“比牌”环节
		ev_timer_again(zjh.loop, &compare_timer);
	}

	return 0;
}



/**
   玩家登录进来
*/
int SeeTable::handler_login(Player *player)
{
	if (players.find(player->uid) == players.end()) {

		players[player->uid] = player;  
		player->tid = tid;

		/// 分配座位号
		player->seatid = sit_down(player);
		if (player->seatid < 0) {
			return -1;    ///---坐下失败返回
		}
		
		Seat &seat = seats[player->seatid];
		seat.uid = player->uid;
		cur_players++;      ///---累计当前玩家个数

		handler_login_succ_uc(player);
		handler_login_succ_bc(player);
		handler_table_info(player);

		xt_log.info("handler login succ uid[%d] money[%d] cur_players[%d] tid[%d] state[%d]. \n", player->uid, player->money, cur_players, tid,state);

		///---当前状态WAIT状态，且玩家数>=2人，游戏状态为准备
		if((WAIT == state) && (cur_players>=2))   
		{
			xt_log.info("change state to ready \n");
			state = READY;
			ev_timer_again(zjh.loop, &ready_timer);   ///---启动“准备”定时器
		}
		return 0;
	}
	return -1;
}

///坐下，返回坐位号
int SeeTable::sit_down(Player *player)
{
	std::vector<int> tmp;     ///空坐位列表
    for (int i = 0; i < seat_max; i++)
    {
		if (seats[i].occupied == 0)
		{
			tmp.push_back(i);
		}
    }
	
	///有空位，则在空位列表中随机选一个位子坐下
	int len = tmp.size();
	if (len > 0)    
	{
		int index = random(0, len - 1);
		int i = tmp[index];
		xt_log.debug("len[%d] index[%d] i[%d]\n", len, index, i);

		seats[i].occupied = 1;     ///成功坐到坐位
		seats[i].player = player;
		return i;
	}

	return -1;
}

///站起，则清空坐位
void SeeTable::stand_up(Player *player)
{
	seats[player->seatid].clear();
}

///删除玩家
int SeeTable::del_player(Player *player)
{
	if (players.find(player->uid) == players.end()) {
		xt_log.error("del player talbe uid[%d] is error.\n", player->uid);
		return -1;
	}
	Seat &seat = seats[player->seatid];
	if (seat.ready == 1) {
		ready_players--;
	}
	player->stop_offline_timer();     ///---停止"下线定时器"
	players.erase(player->uid);
	stand_up(player);
	cur_players--;
	
	return 0;
}
///---登录成功的应答
int SeeTable::handler_login_succ_uc(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGIN_SUCC_UC;
	packet.val["vid"] = player->vid;
	//packet.val["zid"] = player->zid;
	packet.val["tid"] = player->tid;
	packet.end();
	unicast(player, packet.tostring());
	return 0;	
}

///---下行房间数据给登录者
int SeeTable::handler_table_info(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_TABLE_INFO_UC;
	packet.val["vid"] = player->vid;        ///---场馆id
	packet.val["tid"] = player->tid;        ///--桌子的id   
	packet.val["seatid"] = player->seatid;  ///--位置id
	packet.val["state"] = state;            ///---状态
	packet.val["dealer"] = dealer;          ///---庄家id


	std::map<int, Player*>::iterator it;    ///---玩家列表
	int i = 0;
	for (it = players.begin(); it != players.end(); it++) {
		Player *p = it->second;
		Seat &seat = seats[p->seatid];

		packet.val["players"][i]["uid"]     = p->uid;       ///玩家id
		packet.val["players"][i]["seatid"]  = p->seatid;    ///位置id
		packet.val["players"][i]["ready"]   = seat.ready;   ///是否准备		
		packet.val["players"][i]["role"]    = seat.role;    ///是否是庄家
		packet.val["players"][i]["status"]  = seat.status;  ///当前状态
		if (player == p) {
			///手面的牌(该字段只有是自己时，才有)
			for (unsigned int j = 0; j < seat.hole_cards.m_cards.size(); j++) {
				packet.val["players"][i]["holes"].append(seat.hole_cards.m_cards[j].m_value);
			}
			packet.val["players"][i]["card_type"]=seat.hole_cards.m_cardType;			
		}
		packet.val["players"][i]["name"] = p->name;
		packet.val["players"][i]["sex"] = p->sex;
		packet.val["players"][i]["avatar"] = p->avatar;
		packet.val["players"][i]["birthday"] = p->birthday;
		packet.val["players"][i]["zone"] = p->zone;
		packet.val["players"][i]["exp"] = p->exp;
		packet.val["players"][i]["rmb"] = p->rmb; // cfc 20131205 diamond change rmb
		packet.val["players"][i]["money"] = p->money;
		packet.val["players"][i]["coin"] = p->coin;
		packet.val["players"][i]["total_board"] = p->total_board;
		packet.val["players"][i]["total_win"] = p->total_win;
		packet.val["players"][i]["pcount"] = p->pcount;

		packet.val["players"][i]["vlevel"] = p->vlevel;
		packet.val["players"][i]["ps"] = p->ps;   // cfc add by 20131218

		i++;
	}

	packet.end();
	unicast(player, packet.tostring());
	return 0;
}

// 登录成功，“我”的信息广播出去
int SeeTable::handler_login_succ_bc(Player *player)
{
	Seat &seat = seats[player->seatid];

	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGIN_SUCC_BC;

	packet.val["seatid"] = player->seatid;
	packet.val["ready"] = seat.ready;

	packet.val["role"] = seat.role;
	packet.val["status"] = seat.status;


	packet.val["uid"] = player->uid;
	packet.val["name"] = player->name;
	packet.val["sex"] = player->sex;
	packet.val["avatar"] = player->avatar;
	//packet.val["birthday"] = player->birthday;
	//packet.val["zone"] = player->zone;
	packet.val["exp"] = player->exp;
	packet.val["rmb"] = player->rmb;
	packet.val["money"] = player->money;
	packet.val["coin"] = player->coin;
	packet.val["total_board"] = player->total_board;
	packet.val["total_win"] = player->total_win;
	packet.val["pcount"] = player->pcount;
//	packet.val["vtime"] = player->vtime;
	packet.val["vlevel"] = player->vlevel;
	packet.val["ps"] = player->ps;      // cfc add by 20131218

	packet.end();

	broadcast(player, packet.tostring());
	return 0;
}



///---预备超时回调
void SeeTable::preready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->preready_timer);   ///---停止预备定时器

	table->handler_preready();
}

///预备处理(桌子和玩家复位，并检查玩家是否满足继续游戏的条件)
int SeeTable::handler_preready()
{	
	if (state != END_GAME) {
		xt_log.error("handler_preready state_error(%d)",state);
	}


	/* reset table */
	reset();

	/* reset reset */
	for (int i = 0; i < seat_max; i++) {
		seats[i].reset();
	}
	

	/* remove idle and offline player */
	std::map<int, Player*> temp;
	std::map<int, Player*>::iterator it;

	for (it = players.begin(); it != players.end(); it++) {
		Player *player = it->second;

		if (player->client) 
		{
			player->reset();

			if (player->money < min_money)    ///当前金币<底限
			{
				if(player->uid<XT_ROBOT_UID_MAX)  ///若为机器人,随机送钱[200, 375]*min_money
				{
					player->set_money((int)(float(rand()%8+8)/4*100*min_money));
				}
				else 
				{
					temp[player->uid]=player;   ///---记录钱不够的玩家
				}
			}


			if (player->idle_count > 2)         ///---记录空闲玩家(没有动作的玩家)
			{
				temp[player->uid] = player;
				player->logout_type = 1;  
			}
		} 
		else 
		{
			xt_log.debug("handler preready uid[%d] is offline.\n", player->uid);
			temp[player->uid] = player;     ///---记录离线玩家
			player->logout_type = 0;
		}
	}

	for (it = temp.begin(); it != temp.end(); it++) {
		Player *player = it->second;
		zjh.game->del_player(player);
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_GAME_PREREADY_BC;
	for (it = players.begin(); it != players.end(); it++) {
		Player *player = it->second;
		packet.val["seatids"].append(player->seatid);
	}
	packet.end();
	broadcast(NULL, packet.tostring());   ///广播所有人，游戏预备


	/* check state */
	if(cur_players>=2)
	{
		state=READY;     ///状态转为READY
		ev_timer_again(zjh.loop,&ready_timer);
	}
	else 
	{
		state=WAIT;
	}

	return 0;
}


///---“准备”超时回调
void SeeTable::ready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*)w->data;
	ev_timer_stop(zjh.loop, &table->ready_timer);

	table->handler_ready();
}

///---准备环节结束，游戏开始
int SeeTable::handler_ready()
{
	if (state != READY) {   ///状态错误,直接返回
		xt_log.error("ready timer  state error(%d)",state);
		return -1;
	}

	if (cur_players >= 2)   ///如果大于等于两人，则游戏开始
	{
		game_start();
	}
	else
	{
		state=WAIT;   ///---人数不够（有人可能走了），重新置为准备状态
	}
	return 0;
}

///---游戏开始（执行扣税，进入选庄环节）
int SeeTable::game_start()
{
	///游戏状态“选庄”
	state = BANKER;

	///设置坐位状态
	for(int i = 0; i < seat_max; i++)   
	{
		if(seats[i].occupied)
		{
			seats[i].status = 1;    ///1坐位为玩牌状态
		}
	}

	///1扣税(抽水)
	tax()

	///进入"随机选庄"环节
	ev_timer_again(zjh.loop, &banker_timer);

	return 0;
}


//扣税
void SeeTable::tax()
{
	///对所有玩家抽水
	Player *player;
	std::map<int, Player*>::iterator it;
	for (it = players.begin(); it != players.end(); it++)
	{
		player = it->second;
		if (player->client == NULL)
		{
			continue;
		} 
		///---更新扣税
		tax_update(player);
	}
}

///---更新扣税
void SeeTable::tax_update(Player *player)
{
	Seat &seat = seats[player->seatid];
	int value = -fee;
	player->incr_money(0, value);

	///---to do 定义日志类型
	//player->eventlog_update(0, value);
	//player->eventlog_update(2, tips);

	handler_tax_uc(player);
}

///---广播扣税成功(能看到所有人的当前金币)
int SeeTable::handler_tax_uc(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_TAX_SUCC_BC;
	packet.val["vid"] = player->vid;        ///---场馆id
	packet.val["tid"] = player->tid;        ///--桌子的id   

	std::map<int, Player*>::iterator it;  ///---玩家列表
	int i = 0;
	for (it = players.begin(); it != players.end(); it++) {
		Player *p = it->second;
		Seat &seat = seats[p->seatid];

		packet.val["players"][i]["rmb"] = p->rmb; // cfc 20131205 diamond change rmb
		packet.val["players"][i]["money"] = p->money;
		packet.val["players"][i]["coin"] = p->coin;
		i++;
	}

	packet.end();

	unicast(player, packet.tostring());
	return 0;	
}

///---"组牌"超时, 系统为没组牌玩家进行组牌，进入亮牌环节
void SeeTable::group_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*)w->data;
	ev_timer_stop(zjh.loop, &table->group_card_timer);	

	table->group_card_timeout();
}


///---"组牌"时间结束，广播亮牌
void SeeTable::group_card_timeout()
{	
	if (state != GROUP_CARD) {    ///---状态错误，直接返回
		xt_log.error("group timer state error(%d)",state);
		return -1;
	}

	xt_log.debug("group_card_timer_cb");

	Player *player;
	std::map<int, Player*>::iterator it;
	for (it = players.begin(); it != players.end(); it++)
	{
		player = it->second;
		if (player->client == NULL)
		{
			continue;
		} 

		if (tmpCount == index) 
		{
			dealer = player->seatid;    ///---记录当前庄家
			break;
		}

		tmpCount++;
	}

	///---广播所有玩家的牌
	Jpacket packet;
	packet.val["cmd"] = SERVER_MAKE_BANKER_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.end();
	broadcast(NULL, packet.tostring());

	///进入“比牌”环节
	state = COMPARE;
	ev_timer_again(zjh.loop, &compare_timer);
}


///"随机庄家"时间到了
void SeeTable::banker_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->banker_timer);
	
	table->banker_timeout();
}

///确定随机庄家
void SeeTable::banker_timeout()
{
	if (state != BANKER) {    ///---状态错误，直接返回
		xt_log.error("banker timer state error(%d)",state);
		return -1;
	}

	xt_log.debug("banker_timer_cb\n");
	///随机选择一个庄家(todo 钱越多的越适合做庄家)
	int index = random(0, players.size() -1);

	int tmpCount = 0;    ///--临时计数器
	Player *player;
	std::map<int, Player*>::iterator it;
	for (it = players.begin(); it != players.end(); it++)
	{
		player = it->second;
		if (player->client == NULL)
		{
			continue;
		} 

		if (tmpCount == index) 
		{
			dealer = player->seatid;    ///---记录当前庄家
			break;
		}

		tmpCount++;
	}

	///---广播“随机庄家”
	Jpacket packet;
	packet.val["cmd"] = SERVER_MAKE_BANKER_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("banker timeout uid[%d] seatid[%d] tid[%d].\n", player->uid, player->seatid, tid);

	///进入发牌环节
	state = SEND_CARD;
	ev_timer_again(zjh.loop, &send_card_timer);
}

///"发牌"时间到了
void SeeTable::send_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->banker_timer);
	
	///发牌
	table->send_card_timeout();
}

///---对所有玩家发牌
void SeeTable::send_card_timeout()
{
	if (state != SEND_CARD) {    ///---状态错误，直接返回
		xt_log.error("send card timer state error(%d)",state);
		return -1;
	}
	xt_log.debug("send_card_timeout\n");

	///准备发牌
	deck.fill();
	deck.shuffle(tid);


	Jpacket packet;
	packet.val["cmd"] = SERVER_GAME_START_BC;     ///---游戏开始广播
	int current_betting_seats = count_betting_seats();
	int next = next_betting_seat(dealer);   ///---下一个betting坐位

	///---对玩家发牌
	for (int c = 0; c < current_betting_seats; c++) 
	{
		Seat &seat = seats[next];
		Player *player = seat.player;

		if (player == NULL)   ///如果没有，则轮到下一个玩家
		{
			xt_log.error("game start error player[NULL] seatid[%d].\n", seat.seatid);
			seat.betting = 0;
			seat.ready = 0;
			if (dealer == seat.seatid)
			{
				dealer = next_betting_seat(dealer);
			}
			continue;
		}


		xt_log.debug("seatid[%d]next[%d]player_p[%p]\n", seat.seatid, next, player);

		deck.getHoleCards(&seat.hole_cards);   ///---发牌
		seat.hole_cards.analysis();            ///分析牌

		//		Card::sort_by_descending(seat.hole_cards.cards);
		xt_log.info("game start cards[%d, %d, %d] uid[%d] money[%d] seatid[%d] tid[%d].\n",
			seat.hole_cards.m_cards[0].m_value, seat.hole_cards.m_cards[1].m_value,
			seat.hole_cards.m_cards[2].m_value, player->uid, player->money, seat.seatid, tid);
		player->money -= cur_bet; // to check money enough   ///---扣去当前玩家的下注
		seat.bet += cur_bet;      ///---累计当前坐位的赌资
		total_bet += cur_bet;     ///---全桌赌资
		next = next_betting_seat(next);
		packet.val["seatids"].append(seat.seatid);
	}
	packet.val["dealer"] = dealer;         ///---当前玩家
	packet.val["cur_round"] = cur_round;   ///---当前回合
	packet.val["cur_bet"] = cur_bet;       ///---不前赌资    
	packet.val["total_bet"] = total_bet;   ///---总赌资
	packet.end();
	broadcast(NULL, packet.tostring());      ///---广播游戏开始了

	xt_log.info("game start ready_players[%d] cur_players[%d] tid[%d].\n", ready_players, cur_players, tid);

	start_seat = cur_seat = next_betting_seat(dealer);



	Player *player;
	std::map<int, Player*>::iterator it;
	for (it = players.begin(); it != players.end(); it++)
	{
		player = it->second;
		if (player->client == NULL)
		{
			continue;
		} 

	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_MAKE_BANKER_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("send card timeout uid[%d] seatid[%d] tid[%d].\n", player->uid, player->seatid, tid);



	///进入“组牌”环节
	state = GROUP_CARD;
	ev_timer_again(zjh.loop, &group_card_timer);
	///记录组牌开始时间 
	start_group_time = time(0);
}



///"比牌"时间结束, 进行输赢结算
void SeeTable::compare_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->compare_timer);

	table->compare_timeout();
}

///比牌结算
void SeeTable::compare_timeout()
{
	if (state != COMPARE) {    ///---状态错误，直接返回
		xt_log.error("compare timer state error(%d)",state);
		return -1;
	}

	xt_log.debug("compare_timer_cb\n");


	///游戏结束
	state = END_GAME;
	ev_timer_again(zjh.loop, &preready_timer_cb);
}



///---输方，更新玩家数据
void SeeTable::lose_update(Player *player)
{
	Seat &seat = seats[player->seatid];
	player->incr_money(1, seat.bet);
	player->incr_total_board(vid, 1);
	player->incr_achievement_count(vid, 1, 1);

	// cfc add eventlog 20130102
	player->eventlog_update(1, seat.bet);
}

///---赢方，更新玩家数据
void SeeTable::win_update(Player *player)
{
	Seat &seat = seats[player->seatid];
	int tips = (win_bet - seat.bet) * fee;
	int value = (win_bet - seat.bet) - tips;
	player->incr_money(0, value);
	player->incr_total_board(vid, 1);
	player->incr_total_win(vid, 1);
	player->incr_achievement_count(vid, 0, 1);

	// cfc add eventlog 20130102
	player->eventlog_update(0, value);
	player->eventlog_update(2, tips);
}




int SeeTable::test_game_end()
{
	int betting_players = count_betting_seats();
	if (betting_players == 1) {   ///只有一个玩家，游戏结束
		game_end();
		return 0;
	}
	return -1;
}
///游戏结束
int SeeTable::game_end()
{
	//--结算
	xt_log.info("game end tid[%d].\n", tid);
	ev_timer_stop(zjh.loop, &bet_timer);
	ev_timer_stop(zjh.loop, &start_timer);  // cfc add 20131218

	if (state != BETTING) {
		xt_log.error("game end state[%d] is not betting tid[%d]\n", state, tid);
		return -1;
	}  
	state = END_GAME;

	int win_seatid = get_last_betting();
	Seat &seat = seats[win_seatid];
	Player *winPlayer = seat.player;

	Jpacket packet;
	packet.val["cmd"] = SERVER_GAME_END_BC;
	packet.val["win_seatid"] = win_seatid;
	packet.val["total_bet"] = total_bet;
	win_bet = total_bet;  // * 0.9;
	win_update(winPlayer);
	win_exp_update(winPlayer);  // add cfc 20131228
	packet.val["win_money"] = winPlayer->money;
	packet.val["win_coin"] = winPlayer->coin;

	vector_to_json_array(seat.hole_cards.m_cards, packet, "holes");
	int j = 0;
	for (int i = 0; i < seat_max; i++) {
		if (seats[i].ready == 1) {
			Player *player = seats[i].player;
			if (player) {
				packet.val["players"][j]["seatid"] = player->seatid;
				packet.val["players"][j]["uid"] = player->uid;
				packet.val["players"][j]["name"] = player->name;
				packet.val["players"][j]["rmb"] = player->rmb;
				packet.val["players"][j]["money"] = player->money;
				packet.val["players"][j]["coin"] = player->coin;
				packet.val["players"][j]["vlevel"]=player->vlevel;
				packet.val["players"][j]["bet"] = seats[i].bet;
				if (seats[i].status == 0) {
					packet.val["players"][j]["win"] = 1;
					for (unsigned int k = 0; k < seats[i].hole_cards.m_cards.size(); k++) {
						packet.val["players"][j]["holes"].append(seats[i].hole_cards.m_cards[k].m_value);
					}
					packet.val["players"][j]["card_type"] = seats[i].hole_cards.m_cardType;
					player->incr_achievement_card(seats[i].hole_cards.m_cardType, 1);
				} else if (seats[i].status == 2) {
					packet.val["players"][j]["win"] = 0;
					for (unsigned int k = 0; k < seats[i].hole_cards.m_cards.size(); k++) {
						packet.val["players"][j]["holes"].append(seats[i].hole_cards.m_cards[k].m_value);
					}
					packet.val["players"][j]["card_type"] = seats[i].hole_cards.m_cardType;
					player->incr_achievement_card(seats[i].hole_cards.m_cardType, 1);
				} else {
					// fold
					packet.val["players"][j]["win"] = 0;
					for (unsigned int k = 0; k < seats[i].hole_cards.m_cards.size(); k++) {
						packet.val["players"][j]["holes"].append(0);
					}
				}
				j++;
			}
		}
	}
	packet.end();
	broadcast(NULL, packet.tostring());   ///---广播游戏结结果

	ev_timer_again(zjh.loop, &preready_timer);

	return 0;
}

///---处理登出
int SeeTable::handler_logout(Player *player)
{	
	if (state == BETTING) {   ///---正在betting作弃牌处理
		Seat &seat = seats[player->seatid];
		if (seat.betting == 1) {
			handler_fold(player);
			int ret = test_game_end();
			if (ret < 0) {
				if (player->seatid == cur_seat) {
					count_next_bet();
				}
			}
		}
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGOUT_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["type"] = player->logout_type;
	packet.end();
	broadcast(NULL, packet.tostring());   ///---广播“玩家退出”

	xt_log.info("handler logout uid[%d] seatid[%d] money[%d] tid[%d].\n", player->uid, player->seatid, player->money, tid);

	return 0;
}

///---发聊天
int SeeTable::handler_chat(Player *player)
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
int SeeTable::handler_face(Player *player)
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
void SeeTable::robot_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->robot_timer);
	xt_log.info("robot timer cb\n");

	if (table->cur_players != 1) {
		return;
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_ROBOT_NEED_UC;
	packet.val["number"] = 1;
	packet.end();
	if (zjh.game->robot_client != NULL)
		zjh.game->robot_client->send(packet.tostring());
}

void SeeTable::lose_exp_update(Player *player)
{
	player->incr_exp(lose_exp);
}

void SeeTable::win_exp_update(Player *player)
{
	player->incr_exp(win_exp);
}

int SeeTable::handler_info(Player *player)
{
	int ret = 0;
	ret = player->update_info();    ///---更新玩家信息
	if (ret < 0) {
		xt_log.error("handler info error update info error.\n");
		return -1;
	}
	xt_log.info("handler info uid[%d] money[%d].\n", player->uid, player->money);

	if (state == BETTING) {
		Seat &seat = seats[player->seatid];
		if (seat.betting == 1) {
			player->money -= seat.bet;
			xt_log.info("handler info uid[%d] money[%d] bet[%d].\n", player->uid, player->money, seat.bet);
		}
	}

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
	broadcast(NULL, packet.tostring());   ///---广播玩家信息

	xt_log.info("handler info uid[%d] seatid[%d] money[%d] tid[%d].\n", player->uid, player->seatid, player->money, tid);

	return 0;
}

///---处理互动表情
int SeeTable::handler_interaction_emotion(Player *player)
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



