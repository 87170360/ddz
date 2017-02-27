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

#include "hlddz.h"
#include "game.h"
#include "log.h"
#include "table.h"
#include "client.h"
#include "player.h"
#include "proto.h"
#include "XtCard.h"

extern DDZ ddz;
extern Log xt_log;

Table::Table() :
preready_timer_stamp(2),
ready_timer_stamp(5),
start_timer_stamp(3),
bet_timer_stamp(20),
compare_timer_stamp(7),
robot_timer_stamp(6)
{
    preready_timer.data = this;
    ev_timer_init(&preready_timer, Table::preready_timer_cb, preready_timer_stamp,
                    preready_timer_stamp);
	
    ready_timer.data = this;
    ev_timer_init(&ready_timer, Table::ready_timer_cb, ready_timer_stamp,
                    ready_timer_stamp);

    start_timer.data = this;
    ev_timer_init(&start_timer, Table::start_timer_cb, start_timer_stamp,
                    start_timer_stamp);
	
    bet_timer.data = this;
    ev_timer_init(&bet_timer, Table::bet_timer_cb, bet_timer_stamp,
                    bet_timer_stamp);

    compare_timer.data = this;
    ev_timer_init(&compare_timer, Table::compare_timer_cb, compare_timer_stamp,
                    compare_timer_stamp);

    robot_timer.data = this;
    ev_timer_init(&robot_timer, Table::robot_timer_cb, robot_timer_stamp,
    		robot_timer_stamp);


	int weight[6]={(int)(52*1.5), (int)(48*1.5), (int)(1096*1.5), (int)(720*1.5), (int)(3744*1.5), (int)(16440/2)};
	deck.setTypeWeight(weight);
}

Table::~Table()
{
	ev_timer_stop(ddz.loop, &preready_timer);
	ev_timer_stop(ddz.loop, &ready_timer);
	ev_timer_stop(ddz.loop, &start_timer);
	ev_timer_stop(ddz.loop, &bet_timer);
	ev_timer_stop(ddz.loop, &compare_timer);
	ev_timer_stop(ddz.loop, &robot_timer);
}

int Table::init(int my_tid, int my_vid, int my_zid, int my_type, int my_min_money,
				int my_max_money, int my_base_money, int my_min_round, int my_max_round, float my_fee,
				int my_lose_exp, int my_win_exp)
{
	// xt_log.debug("begin to init table [%d]\n", table_id);
	tid = my_tid;
    vid = my_vid;
	zid = my_zid;
	type = my_type;
    min_money = my_min_money;
	max_money = my_max_money;
	base_money = my_base_money;
    min_round = my_min_round;
	max_round = my_max_round;
	fee = my_fee;
	lose_exp = my_lose_exp;
	win_exp = my_win_exp;
	seat_max = 5;
	
    cur_players = 0;
	players.clear();
	ready_players = 0;
    for (int i = 0; i < seat_max; i++)
    {
		seats[i].clear();
        seats[i].seatid = i;
    }
	
	reset();
	state = WAIT;

    return 0;
}



int Table::broadcast(Player *p, const std::string &packet)
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

int Table::unicast(Player *p, const std::string &packet)
{
    if (p->client)
    {
        return p->client->send(packet);
    }
    return -1;
}

int Table::random(int start, int end)
{
	return start + rand() % (end - start + 1);
}

void Table::reset()
{
	state = READY;
	ready_players = 0;
	cur_round = 0;
	start_seat = 0;
	cur_seat = 0;
	cur_action = 0;
	cur_bet = 0;
	total_bet = 0;
	cur_allin_bet = 0;
}

void Table::vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
	for (unsigned int i = 0; i < cards.size(); i++) {
		packet.val[key].append(cards[i].m_value);
	}

	if (cards.size() == 0) {
		packet.val[key].append(0);
	}
}

void Table::map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key)
{
	std::map<int, XtCard>::iterator it;
	for (it = cards.begin(); it != cards.end(); it++)
	{
		XtCard &card = it->second;
		packet.val[key].append(card.m_value);
	}
}

void Table::json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
	Json::Value &val = packet.tojson();
	
	for (unsigned int i = 0; i < val[key].size(); i++)
	{
		XtCard card(val[key][i].asInt());
		
		cards.push_back(card);
	}
}

int Table::handler_login(Player *player)
{
	if (players.find(player->uid) == players.end()) {
		players[player->uid] = player;
        refreshUserState();
		player->tid = tid;
		// todo check.
		player->seatid = sit_down(player);
		Seat &seat = seats[player->seatid];
		// seat.ready = 0;
		seat.uid = player->uid;
		if (player->seatid < 0) {
			return -1;
		}
		cur_players++;

		handler_login_succ_uc(player);
		handler_login_succ_bc(player);
		handler_table_info(player);

		xt_log.info("handler login succ uid[%d] money[%d] cur_players[%d] tid[%d] state[%d]. \n", player->uid, player->money, cur_players, tid,state);

		if((state==WAIT)&&(cur_players>=2))
		{
			xt_log.info("change state to ready \n");
			state=READY;
			ev_timer_again(ddz.loop,&ready_timer);
		}
		return 0;
	}
	return -1;
}

int Table::sit_down(Player *player)
{
	std::vector<int> tmp;
    for (int i = 0; i < seat_max; i++)
    {
		if (seats[i].occupied == 0)
		{
			tmp.push_back(i);
		}
    }
	
	int len = tmp.size();
	if (len > 0)
	{
		int index = random(0, len - 1);
		int i = tmp[index];
		xt_log.debug("len[%d] index[%d] i[%d]\n", len, index, i);
		// seats[i].reset(); 
		seats[i].occupied = 1;
		seats[i].player = player;
		return i;
	}

	return -1;
}

void Table::stand_up(Player *player)
{
	seats[player->seatid].clear();
}


int Table::del_player(Player *player)
{
	if (players.find(player->uid) == players.end()) {
		xt_log.error("del player talbe uid[%d] is error.\n", player->uid);
		return -1;
	}
	Seat &seat = seats[player->seatid];
	if (seat.ready == 1) {
		ready_players--;
	}
	player->stop_offline_timer();
	players.erase(player->uid);
    refreshUserState();
	stand_up(player);
	cur_players--;
	
	return 0;
}

int Table::handler_login_succ_uc(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGIN_SUCC_UC;
	packet.val["vid"] = player->vid;
	packet.val["zid"] = player->zid;
	packet.val["tid"] = player->tid;
	packet.end();
	unicast(player, packet.tostring());
	return 0;	
}

// SERVER_LOGIN_SUCC_BC
int Table::handler_login_succ_bc(Player *player)
{
	Seat &seat = seats[player->seatid];

	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGIN_SUCC_BC;
	// packet.val["vid"] = player->vid;
	// packet.val["zid"] = player->zid;
	// packet.val["tid"] = player->tid;
	packet.val["seatid"] = player->seatid;
	packet.val["ready"] = seat.ready;
	packet.val["betting"] = seat.betting;
	packet.val["see"] = seat.see;
	packet.val["role"] = seat.role;
	packet.val["status"] = seat.status;
	packet.val["bet"] = seat.bet;

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
	// broadcast(NULL, packet.tostring());

	return 0;
}

int Table::handler_table_info(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_TABLE_INFO_UC;
	packet.val["vid"] = player->vid;
	packet.val["zid"] = player->zid;
	packet.val["tid"] = player->tid;
	packet.val["seatid"] = player->seatid;

	packet.val["state"] = state;
	packet.val["dealer"] = dealer;
	packet.val["cur_round"] = cur_round;
	packet.val["cur_seat"] = cur_seat;
	packet.val["cur_bet"] = cur_bet;
	packet.val["total_bet"] = total_bet;

	std::map<int, Player*>::iterator it;
	int i = 0;
	for (it = players.begin(); it != players.end(); it++) {
		Player *p = it->second;
		Seat &seat = seats[p->seatid];

		packet.val["players"][i]["uid"] = p->uid;
		packet.val["players"][i]["seatid"] = p->seatid;
		packet.val["players"][i]["ready"] = seat.ready;
		packet.val["players"][i]["betting"] = seat.betting;
		packet.val["players"][i]["see"] = seat.see;
		packet.val["players"][i]["role"] = seat.role;
		packet.val["players"][i]["status"] = seat.status;
		packet.val["players"][i]["bet"] = seat.bet;
		packet.val["players"][i]["is_forbidden"] = seat.is_forbidden;
		packet.val["players"][i]["is_double"] = seat.is_double;
		packet.val["players"][i]["is_change"] = seat.is_change;
		if (player == p) {
			if (seat.see == 1) {
				for (unsigned int j = 0; j < seat.hole_cards.m_cards.size(); j++) {
					packet.val["players"][i]["holes"].append(seat.hole_cards.m_cards[j].m_value);
				}
				packet.val["players"][i]["card_type"]=seat.hole_cards.m_cardType;
			}
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
//		packet.val["players"][i]["vtime"] = p->vtime;
		packet.val["players"][i]["vlevel"] = p->vlevel;
		packet.val["players"][i]["ps"] = p->ps;   // cfc add by 20131218
		// cfc add 20140321
		packet.val["players"][i]["forbidden_card"] = p->forbidden_card;
		packet.val["players"][i]["change_card"] = p->change_card;
		packet.val["players"][i]["double_card"] = p->double_card;
		i++;
	}

	packet.end();
	unicast(player, packet.tostring());

	return 0;
}


int Table::handler_preready()
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

            if(player->uid < XT_ROBOT_UID_MAX)
            {
                resetRobotMoney(player);
            }
            else if(player->money < min_money) 
            {
                temp[player->uid]=player;
            }

			if (player->idle_count > 2) 
			{
				temp[player->uid] = player;
				player->logout_type = 1;  
			}
		} 
		else 
		{
			xt_log.debug("handler preready uid[%d] is offline.\n", player->uid);
			temp[player->uid] = player;
			player->logout_type = 0;
		}
	}

	for (it = temp.begin(); it != temp.end(); it++) {
		Player *player = it->second;
		ddz.game->del_player(player);
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_GAME_PREREADY_BC;
	for (it = players.begin(); it != players.end(); it++) {
		Player *player = it->second;
		packet.val["seatids"].append(player->seatid);
	}
	packet.end();
	broadcast(NULL, packet.tostring());


	/* check state */
	if(cur_players>=2)
	{
		state=READY;
		ev_timer_again(ddz.loop,&ready_timer);
	}
	else 
	{
		state=WAIT;
	}

	return 0;
}

void Table::preready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*) w->data;
	ev_timer_stop(ddz.loop, &table->preready_timer);
	table->handler_preready();
}

void Table::ready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*)w->data;
	ev_timer_stop(ddz.loop, &table->ready_timer);
	table->handler_ready();
}


int Table::handler_ready()
{
	if (state != READY) {
		xt_log.error("ready timer  state error(%d)",state);
		return -1;
	}

	if(cur_players>=2)
	{
		game_start();
	}
	else
	{
		state=WAIT;
	}
	return 0;
}


void Table::start_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*)w->data;
	ev_timer_stop(ddz.loop, &table->start_timer);

	table->start_next_bet();
}


int Table::game_start()
{
	state = BETTING;


	for(int i=0;i<seat_max;i++)
	{
		if(seats[i].occupied)
		{
			seats[i].ready=1;
			seats[i].betting=1;
		}
	}


	cur_bet = base_money;
	total_bet = 0;
	dealer = next_betting_seat(dealer);
	//	dealer = 0; // todo delete this line.
	deck.fill();
	deck.shuffle(tid);
	cur_round = 1;

	Jpacket packet;
	packet.val["cmd"] = SERVER_GAME_START_BC;
	int current_betting_seats = count_betting_seats();
	int next = next_betting_seat(dealer);
	for (int c = 0; c < current_betting_seats; c++) 
	{
		Seat &seat = seats[next];
		Player *player = seat.player;

		if (player == NULL) 
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

		deck.getHoleCards(&seat.hole_cards);
		seat.hole_cards.analysis();

		//		Card::sort_by_descending(seat.hole_cards.cards);
		xt_log.info("game start cards[%d, %d, %d] uid[%d] money[%d] seatid[%d] tid[%d].\n",
				seat.hole_cards.m_cards[0].m_value, seat.hole_cards.m_cards[1].m_value,
				seat.hole_cards.m_cards[2].m_value, player->uid, player->money, seat.seatid, tid);
		player->money -= cur_bet; // to check money enough
		seat.bet += cur_bet;
		total_bet += cur_bet;
		next = next_betting_seat(next);
		packet.val["seatids"].append(seat.seatid);
	}
	packet.val["dealer"] = dealer;
	packet.val["cur_round"] = cur_round;
	packet.val["cur_bet"] = cur_bet;
	packet.val["total_bet"] = total_bet;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("game start ready_players[%d] cur_players[%d] tid[%d].\n", ready_players, cur_players, tid);

	start_seat = cur_seat = next_betting_seat(dealer);

	// start_next_bet();
	ev_timer_again(ddz.loop, &start_timer);

	return 0;
}

int Table::count_next_bet()
{
	int _seat = next_betting_seat(cur_seat);
	if (_seat == -1) {
		xt_log.error("count next bet no active player.\n");
		handler_preready();
		return -1;
	}
	cur_seat = _seat;                // cfc add 7 line
	//	cur_seat = next_betting_seat(cur_seat);  // cfc remark this line
	if (start_seat == cur_seat) {
		cur_round++;
		recover_forbidden_card();  // cfc add 20140321
	}
	start_next_bet();

	return 0;
}

int Table::start_next_bet()
{
	Seat &seat = seats[cur_seat];
	Player *player = seat.player;

	Jpacket packet;
	packet.val["cmd"] = SERVER_NEXT_BET_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = seat.seatid;
	packet.val["cur_round"] = cur_round;
	if (seat.see == 1)
		packet.val["cur_bet"] = cur_bet * 2;
	else
		packet.val["cur_bet"] = cur_bet;
	packet.val["total_bet"] = total_bet;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("start next bet uid[%d] seatid[%d] tid[%d].\n", player->uid, seat.seatid, tid);

	ev_timer_again(ddz.loop, &bet_timer);

	return 0;
}

void Table::bet_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*) w->data;
	ev_timer_stop(ddz.loop, &table->bet_timer);
	xt_log.debug("bet_timer_cb\n");
	table->bet_timeout();
}

void Table::compare_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*) w->data;
	ev_timer_stop(ddz.loop, &table->compare_timer);
	xt_log.debug("compare_timer_cb\n");
	table->compare_timeout();
}


int Table::handler_bet(Player *player)
{
	if (state != BETTING) {
		xt_log.error("handler bet state[%d] is not betting tid[%d]\n", state, tid);
		return -1;
	}

	Seat &seat = seats[player->seatid];
	if (seat.player != player) {
		xt_log.error("handler_bet player is no match with seat.player\n");
		return -1;
	}

	player->idle_count=0;
	Json::Value &val = player->client->packet.tojson();
	int action = val["action"].asInt();
	if (action == PLAYER_SEE) {
		handler_see(player);
		return 0;
	}

	if (action == PLAYER_FOLD) {
		handler_fold(player);
		int ret = test_game_end();
		if (ret < 0) {
			if (player->seatid == cur_seat) {
				count_next_bet();
			}
		}
		return 0;
	}

	if (cur_seat != player->seatid)
	{
		xt_log.error("handler bet player seatid[%d] cur_seat[%d]\n", player->seatid, cur_seat);
		return -1;
	}

	//	int round = val["round"].asInt();
	//	if (round != cur_round) {
	//		xt_log.warn("handler_bet cur_round[%d] round[%d]\n", cur_round, round);
	//	}

	if (action == PLAYER_CALL) {
		int ret=handler_call(player);
		if(ret!=-1)
		{
			ev_timer_stop(ddz.loop, &bet_timer);
			count_next_bet();
		}
		return 0;
	}

	if (action == PLAYER_RAISE) {
		int ret=handler_raise(player);
		if(ret!=-1)
		{
			ev_timer_stop(ddz.loop, &bet_timer);
			count_next_bet();
		}
		return 0;
	}

	if (action == PLAYER_COMPARE)
	{
		ev_timer_stop(ddz.loop, &bet_timer);
		handler_compare(player);
		// int ret = test_game_end();
		// if (ret < 0)
		// {
		// 	count_next_bet();
		// }
		ev_timer_again(ddz.loop, &compare_timer);
		return 0;
	}

	if (action == PLAYER_ALLIN) {
		ev_timer_stop(ddz.loop, &bet_timer);
		handler_allin(player);
		count_next_bet();
		return 0;
	}

	if (action == PLAYER_ALLIN_COMPARE) {
		ev_timer_stop(ddz.loop, &bet_timer);
		handler_allin_compare(player);
		ev_timer_again(ddz.loop, &compare_timer);
		return 0;
	}

	return 0;
}

void Table::bet_timeout()
{
	Seat &seat = seats[cur_seat];
	Player *player = seat.player;

	if (player == NULL) {
		xt_log.error("bet timeout null player ready_players[%d] cur_players[%d] tid[%d].\n", ready_players, cur_players, tid);
		return;
	}

	lose_update(player);

	seat.betting = 0;
	seat.status = 1;
	player->idle_count++;

	if (start_seat == cur_seat) {
		start_seat = prev_betting_seat(start_seat);
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_BET_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["action"] = PLAYER_FOLD;
	packet.val["cur_round"] = cur_round;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("bet timeout uid[%d] seatid[%d] tid[%d].\n", player->uid, player->seatid, tid);


	int ret = test_game_end();
	if (ret < 0) {
		count_next_bet();
	}	
}

void Table::compare_timeout()
{
	if (state != BETTING) {
		xt_log.error("compare timeout state[%d] is not BETTING.\n", state);
		return;
	} // cfc add if 20140314
	int ret = test_game_end();
	if (ret < 0) {
		count_next_bet();
	}
}

void Table::lose_update(Player *player)
{
	Seat &seat = seats[player->seatid];
	player->incr_money(1, seat.bet);
	player->incr_total_board(vid, 1);
	player->incr_achievement_count(vid, 1, 1);

	// cfc add eventlog 20130102
	player->eventlog_update(1, seat.bet);
}

void Table::win_update(Player *player)
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

    saveChangeValue(static_cast<double>(tips));
}
  
void Table::saveChangeValue(double value)
{
	if(ddz.cache_rc->command("hincrby ak_info m_change_value %.0f", value))
	{
		xt_log.error("Table saveChangeValue error, value:%.0f", value);
	}
}

int Table::handler_call(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	Seat &seat = seats[player->seatid];
	int bet_ratio = 0;

	// money
	if (seat.see == 1) {
		if (player->money >= cur_bet * 2) {
			player->money -= cur_bet * 2;
			seat.bet += cur_bet * 2;
			total_bet += cur_bet * 2;
			bet_ratio = 2;
		} else {
			xt_log.error("handler call error uid[%d] money[%d] cur_bet*2[%d].\n", player->uid, player->money, cur_bet * 2);
			return -1;
		}
	} else {
		if (player->money >= cur_bet) {
			player->money -= cur_bet;
			seat.bet += cur_bet;
			total_bet += cur_bet;
			bet_ratio = 1;
		} else {
			xt_log.error("handler call error uid[%d] money[%d] cur_bet[%d].\n", player->uid, player->money, cur_bet);
			return -1;
		}
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_BET_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["money"] = player->money;
	packet.val["bet"] = seat.bet;
	packet.val["action"] = val["action"].asInt();
	packet.val["bet_ratio"] = bet_ratio;
	packet.val["cur_round"] = cur_round;
	packet.val["cur_bet"] = cur_bet;
	packet.val["total_bet"] = total_bet;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler call uid[%d] seatid[%d] money[%d] cur_bet[%d] seat.bet[%d] total_bet[%d] tid[%d].\n", player->uid, player->seatid, player->money, cur_bet, seat.bet, total_bet, tid);

	return 0;
}

int Table::handler_raise(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	int raise_bet = val["raise_bet"].asInt();
	int bet_ratio = 0;

	if (raise_bet <= cur_bet) {
		xt_log.error("handler raise raise_bet[%d] cur_bet[%d].\n", raise_bet, cur_bet);
		return 0;
	}

	Seat &seat = seats[player->seatid];
	// money
	if (seat.see == 1) {
		if (player->money >= raise_bet * 2) {
			cur_bet = raise_bet;
			player->money -= cur_bet * 2;
			seat.bet += cur_bet * 2;
			total_bet += cur_bet * 2;
			bet_ratio = 2;
		} else {
			xt_log.error("handler raise error money[%d] cur_bet*2[%d].\n", player->money, cur_bet * 2);
			return -1;
		}
	} else {
		if (player->money >= raise_bet) {
			cur_bet = raise_bet;
			player->money -= cur_bet;
			seat.bet += cur_bet;
			total_bet += cur_bet;
			bet_ratio = 1;
		} else {
			xt_log.error("handler raise error money[%d] cur_bet[%d].\n", player->money, cur_bet);
			return -1;
		}
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_BET_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["money"] = player->money;
	packet.val["bet"] = seat.bet;
	packet.val["action"] = val["action"].asInt();
	packet.val["bet_ratio"] = bet_ratio;
	packet.val["cur_bet"] = cur_bet;
	packet.val["cur_round"] = cur_round;
	packet.val["total_bet"] = total_bet;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler raise uid[%d] seatid[%d] money[%d] cur_bet[%d] seat.bet[%d] total_bet[%d] tid[%d].\n", player->uid, player->seatid, player->money, cur_bet, seat.bet, total_bet, tid);

	return 0;
}

int Table::handler_see(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	Seat &seat = seats[player->seatid];
	if (seat.betting == 0)
	{
		return 0;
	}
	seat.see = 1;

	Jpacket packet;
	packet.val["cmd"] = SERVER_BET_SUCC_UC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["action"] = val["action"].asInt();
	packet.val["cur_round"] = cur_round;
	packet.val["card_type"] = seat.hole_cards.m_cardType;
	vector_to_json_array(seat.hole_cards.m_cards, packet, "holes");
	packet.end();
	unicast(player, packet.tostring());

	Jpacket packet2;
	packet2.val["cmd"] = SERVER_BET_SUCC_BC;
	packet2.val["uid"] = player->uid;
	packet2.val["seatid"] = player->seatid;
	packet2.val["action"] = val["action"].asInt();
	packet2.val["cur_round"] = cur_round;
	packet2.end();
	broadcast(player, packet2.tostring());

	xt_log.info("handler see uid[%d] seatid[%d] tid[%d].\n", player->uid, player->seatid, tid);

	if (player->seatid == cur_seat) {
		start_next_bet();
	}

	return 0;
}

/*
   cmd
   uid
   seatid
   target_uid
   target_seatid

*/

int Table::handler_compare(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	int seatid = val["seatid"].asInt();
	int target_seatid = val["target_seatid"].asInt();

	if(target_seatid>=5)
	{
		return 0;
	}


	Seat &seat = seats[player->seatid];
	Seat &target_seat = seats[target_seatid];
	int bet_ratio = 0;


	// money
	int _compare_bet = 0;
	if (seat.see == 1) {
		_compare_bet = cur_bet * 2 * target_seat.is_double;
		if (player->money >= _compare_bet) {
			player->money -= _compare_bet;
			seat.bet += _compare_bet;
			total_bet += _compare_bet;
			bet_ratio = 2 * target_seat.is_double;
		} else {
			seat.bet += player->money;
			total_bet += player->money;
			player->money = 0;
			bet_ratio = 2 * target_seat.is_double;
		}
	} else {
		_compare_bet = cur_bet * target_seat.is_double;
		if (player->money >= _compare_bet) {
			player->money -= _compare_bet;
			seat.bet += _compare_bet;
			total_bet += _compare_bet;
			bet_ratio = 1 * target_seat.is_double;
		} else {
			seat.bet += player->money;
			total_bet += player->money;
			player->money = 0;
			bet_ratio = 1 * target_seat.is_double;
		}
	}

	if (target_seat.betting == 0) {
		xt_log.warn("handler compare target_seat betting[0] uid[%d] seatid[%d] target_seatid[%d] tid[%d].\n", player->uid, seatid, target_seatid, tid);
		return 0;
	} // cfc add if 20140317

	if (target_seat.is_forbidden > 0) {
		xt_log.warn("handler compare target use forbidden_card. uid[%d] seatid[%d] target_seatid[%d] tid[%d].\n", player->uid, seatid, target_seatid, tid);
		return 0;
	}

	xt_log.info("handler compare uid[%d] seatid[%d] money[%d] cur_bet[%d] seat.bet[%d] total_bet[%d] tid[%d].\n", player->uid, seatid, player->money, cur_bet, seat.bet, total_bet, tid);

	int ret = seat.hole_cards.compare(target_seat.hole_cards);
	xt_log.debug("handler compare ret[%d] type1[%d][%d][%d][%d] type2[%d][%d][%d][%d].\n",
			ret, seat.hole_cards.m_cardType, seat.hole_cards.m_cards[0].m_face,
			seat.hole_cards.m_cards[1].m_face, seat.hole_cards.m_cards[2].m_face,
			target_seat.hole_cards.m_cardType, target_seat.hole_cards.m_cards[0].m_face,
			target_seat.hole_cards.m_cards[1].m_face, target_seat.hole_cards.m_cards[2].m_face);
	if (ret <= 0) {
		lose_update(seat.player);
		lose_exp_update(seat.player);  // add cfc 20131228
		seat.status = 2;
		seat.betting = 0;
		if (start_seat == seatid) {
			start_seat = prev_betting_seat(start_seat);
		}
	} else {
		lose_update(target_seat.player);
		lose_exp_update(target_seat.player);  // add cfc 20131228
		target_seat.status = 2;
		target_seat.betting = 0;
		if (start_seat == target_seatid) {
			start_seat = prev_betting_seat(start_seat);
		}
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_BET_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["status"] = seat.status;
	packet.val["money"] = player->money;
	packet.val["bet"] = seat.bet;
	packet.val["action"] = val["action"].asInt();
	packet.val["bet_ratio"] = bet_ratio;
	packet.val["target_uid"] = val["target_uid"].asInt();
	packet.val["target_seatid"] = val["target_seatid"].asInt();
	packet.val["target_status"] = target_seat.status;
	packet.val["action"] = val["action"].asInt();
	packet.val["cur_round"] = cur_round;
	packet.val["cur_bet"] = cur_bet;
	packet.val["total_bet"] = total_bet;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler compare ret[%d] uid[%d] seatid[%d] money[%d] target_uid[%d] target_seatid[%d] target_money[%d] tid[%d].\n",
			ret, player->uid, player->seatid, player->money, target_seat.player->uid, target_seatid, target_seat.player->money, tid);

	return 0;
}

int Table::handler_fold(Player *player)
{
	int seatid = player->seatid;
	Seat &seat = seats[player->seatid];

	if (seat.betting == 0) {
		xt_log.warn("handler fold seat betting[0] uid[%d] tid[%d].\n", player->uid, tid);
		return 0;
	}  // cfc add if 20131219

	lose_update(seat.player);
	seat.betting = 0;
	seat.status = 1;

	if (start_seat == seatid) {
		start_seat = prev_betting_seat(start_seat);
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_BET_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["action"] = PLAYER_FOLD;
	packet.val["cur_round"] = cur_round;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler fold uid[%d] seatid[%d] money[%d] seat.bet[%d] tid[%d].\n", player->uid, player->seatid, player->money, seat.bet, tid);

	return 0;
}

int Table::handler_allin(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	int allin_bet = val["allin_bet"].asInt();
	Seat &seat = seats[player->seatid];
	int bet_ratio = 0;

	if (allin_bet != 0) {
		if (player->money >= allin_bet * 2) {
			player->money -= allin_bet * 2;
			seat.bet += allin_bet * 2;
			total_bet += allin_bet * 2;
			bet_ratio = 2;
			cur_allin_bet = allin_bet;
		} else {
			xt_log.error("handler allin money[%d] allin bet*2[%d].\n", player->money, allin_bet * 2);
			return -1;
		}
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_BET_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["money"] = player->money;
	packet.val["bet"] = seat.bet;
	packet.val["action"] = val["action"].asInt();
	packet.val["bet_ratio"] = bet_ratio;
	packet.val["cur_bet"] = cur_allin_bet;
	packet.val["cur_round"] = cur_round;
	packet.val["total_bet"] = total_bet;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler allin uid[%d] seatid[%d] money[%d] cur_allin_bet[%d] seat.bet[%d] total_bet[%d] tid[%d].\n", player->uid, player->seatid, player->money, cur_allin_bet, seat.bet, total_bet, tid);

	return 0;
}

int Table::test_game_end()
{
	int betting_players = count_betting_seats();
	if (betting_players == 1) {
		game_end();
        robotChange();
		return 0;
	}
	return -1;
}

int Table::game_end()
{
	xt_log.info("game end tid[%d].\n", tid);
	ev_timer_stop(ddz.loop, &bet_timer);
	ev_timer_stop(ddz.loop, &start_timer);  // cfc add 20131218

	if (state != BETTING) {
		xt_log.error("game end state[%d] is not betting tid[%d]\n", state, tid);
		return -1;
	}  // cfc add if 20140314
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
	broadcast(NULL, packet.tostring());

	ev_timer_again(ddz.loop, &preready_timer);

	return 0;
}

int Table::handler_logout(Player *player)
{	
	if (state == BETTING) {
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
	broadcast(NULL, packet.tostring());

	xt_log.info("handler logout uid[%d] seatid[%d] money[%d] tid[%d].\n", player->uid, player->seatid, player->money, tid);

	return 0;
}

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
	broadcast(NULL, packet.tostring());

	return 0;
}

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

int Table::next_seat(int pos)
{
	for (int i = 0; i < 3; i++) {
		pos++;
		if (pos >= 3)
			pos = 0;
		return pos;
	}

	return -1;
}

int Table::next_betting_seat(int pos)
{
	int cur = pos;
	for (int i = 0; i < seat_max; i++) {
		cur++;
		if (cur >= seat_max)
			cur = 0;
		if (seats[cur].betting == 1) {
			xt_log.debug("cur is betting seat[%d]\n", cur);
			return cur;
		}
	}
	xt_log.error("next betting seat error active player.\n");
	return -1;
}

int Table::prev_betting_seat(int pos)
{
	int cur = pos;
	for (int i = 0; i < seat_max; i++) {
		cur--;
		if (cur < 0)
			cur = seat_max - 1;
		if (seats[cur].betting == 1) {
			xt_log.debug("cur is betting seat[%d]\n", cur);
			return cur;
		}
	}
	xt_log.error("prev betting seat error active player.\n");
	return -1;
}

int Table::count_betting_seats()
{
	int count = 0;
	for (int i = 0; i < seat_max; i++) {
		if (seats[i].betting == 1) {
			count++;
		}
	}
	return count;
}

int Table::get_last_betting()
{
	for (int i = 0; i < seat_max; i++) {
		if (seats[i].betting == 1) {
			return i;
		}
	}
	return -1;
}

void Table::robot_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*) w->data;
	ev_timer_stop(ddz.loop, &table->robot_timer);
	xt_log.info("robot timer cb\n");

	if (table->cur_players != 1) {
		return;
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_ROBOT_NEED_UC;
	packet.val["number"] = 1;
	packet.end();
	if (ddz.game->robot_client != NULL)
		ddz.game->robot_client->send(packet.tostring());
}

void Table::lose_exp_update(Player *player)
{
	player->incr_exp(lose_exp);
}

void Table::win_exp_update(Player *player)
{
	player->incr_exp(win_exp);
}

int Table::handler_info(Player *player)
{
	int ret = 0;
	ret = player->update_info();
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

    updateInfoBC(player);
	xt_log.info("handler info uid[%d] seatid[%d] money[%d] tid[%d].\n", player->uid, player->seatid, player->money, tid);

	return 0;
}

int Table::handler_allin_compare(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	int seatid = val["seatid"].asInt();
	Seat &seat = seats[player->seatid];
	int bet_ratio = 0;

	if (player->money >= cur_allin_bet * 2) {
		player->money -= cur_allin_bet * 2;
		seat.bet += cur_allin_bet * 2;
		total_bet += cur_allin_bet * 2;
		bet_ratio = 2;
	} else {
		seat.bet += player->money;
		total_bet += player->money;
		player->money = 0;
		bet_ratio = 1;
	}

	int target_seatid = val["target_seatid"].asInt();
	Seat &target_seat = seats[target_seatid];
	if (target_seat.betting == 0) {
		xt_log.warn("handler allin compare target_seat betting[0] uid[%d] seatid[%d] target_seatid[%d] tid[%d].\n", player->uid, seatid, target_seatid, tid);
		return 0;
	} // cfc add if 20140317

	xt_log.info("handler allin compare uid[%d] seatid[%d] money[%d] cur_allin_bet[%d] seat.bet[%d] total_bet[%d] tid[%d].\n", player->uid, seatid, player->money, cur_allin_bet, seat.bet, total_bet, tid);

	int ret = seat.hole_cards.compare(target_seat.hole_cards);
	xt_log.debug("ret[%d] type1[%d][%d][%d][%d] type2[%d][%d][%d][%d].\n",
			ret, seat.hole_cards.m_cardType, seat.hole_cards.m_cards[0].m_face,
			seat.hole_cards.m_cards[1].m_face, seat.hole_cards.m_cards[2].m_face,
			target_seat.hole_cards.m_cardType, target_seat.hole_cards.m_cards[0].m_face,
			target_seat.hole_cards.m_cards[1].m_face, target_seat.hole_cards.m_cards[2].m_face);
	if (ret <= 0) {
		lose_update(seat.player);
		lose_exp_update(seat.player);  // add cfc 20131228
		seat.status = 2;
		seat.betting = 0;
		if (start_seat == seatid) {
			start_seat = prev_betting_seat(start_seat);
		}
	} else {
		lose_update(target_seat.player);
		lose_exp_update(target_seat.player);  // add cfc 20131228
		target_seat.status = 2;
		target_seat.betting = 0;
		if (start_seat == target_seatid) {
			start_seat = prev_betting_seat(start_seat);
		}
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_BET_SUCC_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.val["status"] = seat.status;
	packet.val["money"] = player->money;
	packet.val["bet"] = seat.bet;
	packet.val["action"] = val["action"].asInt();
	packet.val["bet_ratio"] = bet_ratio;
	packet.val["target_uid"] = val["target_uid"].asInt();
	packet.val["target_seatid"] = val["target_seatid"].asInt();
	packet.val["target_status"] = target_seat.status;
	packet.val["action"] = val["action"].asInt();
	packet.val["cur_round"] = cur_round;
	packet.val["cur_bet"] = cur_allin_bet;
	packet.val["total_bet"] = total_bet;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler allin compare ret[%d] uid[%d] seatid[%d] target_uid[%d] target_seatid[%d] tid[%d].\n",
			ret, player->uid, player->seatid, val["target_uid"].asInt(), target_seatid, tid);

	return 0;
}

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

int Table::handler_prop(Player *player)
{
	if (state != BETTING) {
		xt_log.error("handler_prop state[%d] is not betting tid[%d]\n", state, tid);
		return -1;
	}

	Seat &seat = seats[player->seatid];
	if (seat.player != player) {
		xt_log.error("handler_prop player is no match with seat.player\n");
		return -1;
	}

	Json::Value &val = player->client->packet.tojson();
	int prop_id = val["prop_id"].asInt();

	int ret = 0;
	if (prop_id == CHANGE_CARD) {
		ret = handler_change_card(player);
	} else if (prop_id == FORBIDDEN_CARD) {
		ret = handler_forbidden_card(player);
	} else if (prop_id == DOUBLE_CARD_FOUR) {
		ret = handler_double_card(player, 4);
	} else if (prop_id == DOUBLE_CARD_SIX) {
		ret = handler_double_card(player, 6);
	} else if (prop_id == DOUBLE_CARD_SIX) {
		ret = handler_double_card(player, 8);
	}

	if (ret < 0) {
		xt_log.warn("handler_prop error. uid[%d] seatid[%d] prop_id[%d] tid[%d].\n", player->uid, player->seatid, prop_id, tid);
		return -1;
	}

	xt_log.info("handler_prop succ. uid[%d] seatid[%d] prop_id[%d] tid[%d].\n", player->uid, player->seatid, prop_id, tid);
	return 0;
}

int Table::handler_change_card(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	int seatid = val["seatid"].asInt();
	int pos = val["pos"].asInt();
	int value = val["value"].asInt();
	int prop_id = val["prop_id"].asInt();

	Seat &seat = seats[player->seatid];
	if (seat.betting == 0) {
		xt_log.error("handler change card error. betting[0] uid[%d] seatid[%d] tid[%d].\n", player->uid, player->seatid, tid);
		handler_prop_error(player, prop_id);
		return -1;
	}
	if (seat.see == 0) {
		handler_prop_error(player, prop_id);
		xt_log.error("handler change card error. see[0] uid[%d] seatid[%d] tid[%d].\n", player->uid, player->seatid, tid);
		return -1;
	}

	int _count_change = seat.is_change + 1;
	if (_count_change == 1) {
		if (player->change_card < 1) {
			handler_prop_error(player, prop_id);
			xt_log.error("handler change card error. not card uid[%d] seatid[%d] change_card[%d] count_change[%d] tid[%d].\n", player->uid, player->seatid, player->change_card, _count_change, tid);
			return -1;
		}
	} else if (_count_change == 2) {
		if (player->change_card < 2) {
			handler_prop_error(player, prop_id);
			xt_log.error("handler change card error. not card uid[%d] seatid[%d] change_card[%d] count_change[%d] tid[%d].\n", player->uid, player->seatid, player->change_card, _count_change, tid);
			return -1;
		}
	} else if (_count_change == 3) {
		if (player->change_card < 3) {
			handler_prop_error(player, prop_id);
			xt_log.error("handler change card error. not card uid[%d] seatid[%d] change_card[%d] count_change[%d] tid[%d].\n", player->uid, player->seatid, player->change_card, _count_change, tid);
			return -1;
		}
	} else {
		xt_log.error("handler change card error. not card uid[%d] seatid[%d] change_card[%d] count_change[%d] tid[%d].\n", player->uid, player->seatid, player->change_card, _count_change, tid);
		return -1;
	}

	if (pos < 0 || pos > 2 ) {
		handler_prop_error(player, prop_id);
		xt_log.error("handler change card error. hole_cards size[%d] pos[%d].\n", seat.hole_cards.m_cards.size(), pos);
		return -1;
	}
	if (seat.hole_cards.m_cards[pos].m_value != value) {
		handler_prop_error(player, prop_id);
		xt_log.error("handler change card error. hole_cards card[%d] value[%d] error.\n", seat.hole_cards.m_cards[pos].m_value, value);
		return -1;
	}

	deck.changeHoleCards(pos, &seat.hole_cards);
	xt_log.info("handler change card cards[%d, %d, %d] is_change[%d].\n",
			seat.hole_cards.m_cards[0].m_value, seat.hole_cards.m_cards[1].m_value, seat.hole_cards.m_cards[2].m_value, _count_change);
	seat.is_change = _count_change;
	player->incr_change_card(_count_change);

	Jpacket packet;
	packet.val["cmd"] = SERVER_PROP_SUCC_UC;
	packet.val["seatid"] = seatid;
	packet.val["uid"] = player->uid;
	packet.val["prop_id"] = val["prop_id"].asInt();
	packet.val["change_card"] = player->change_card;
	packet.val["card_type"] = seat.hole_cards.m_cardType;
	packet.val["is_change"] = seat.is_change;
	vector_to_json_array(seat.hole_cards.m_cards, packet, "holes");
	packet.end();
	unicast(player, packet.tostring());

	Jpacket packet2;
	packet2.val["cmd"] = SERVER_PROP_SUCC_BC;
	packet2.val["seatid"] = seatid;
	packet2.val["uid"] = player->uid;
	packet.val["prop_id"] = val["prop_id"].asInt();
	packet2.end();
	broadcast(player, packet2.tostring());

	xt_log.info("handler change card uid[%d] seatid[%d] is_change[%d] change_card[%d] tid[%d].\n", player->uid, player->seatid, seat.is_change, player->change_card, tid);
	return 0;
}

int Table::handler_forbidden_card(Player *player)
{
	Json::Value &val = player->client->packet.tojson();
	int seatid = val["seatid"].asInt();
	int prop_id = val["prop_id"].asInt();

	Seat &seat = seats[player->seatid];
	if (seat.betting == 0) {
		handler_prop_error(player, prop_id);
		xt_log.error("handler forbidden card error. betting[0] uid[%d] seatid[%d] tid[%d].\n", player->uid, seatid, tid);
		return -1;
	}
	if (player->forbidden_card < 1) {
		handler_prop_error(player, prop_id);
		xt_log.error("handler forbidden card not card. uid[%d] seatid[%d] forbidden_card[%d] tid[%d].\n", player->uid, seatid, player->forbidden_card, tid);
		return -1;
	}
	if (cur_round >= max_round) {
		handler_prop_error(player, prop_id);
		xt_log.error("handler forbidden card error. uid[%d] seatid[%d] cur_round[%d] max_round[%d] tid[%d].\n", player->uid, seatid, cur_round, max_round, tid);
		return -1;
	}

	seat.is_forbidden = cur_round + 5;  // 5 round later recover_forbidden_card
	if (seat.is_forbidden > 20)
		seat.is_forbidden = 20;
	player->incr_forbidden_card(1);
	Jpacket packet;
	packet.val["cmd"] = SERVER_PROP_SUCC_BC;
	packet.val["seatid"] = seatid;
	packet.val["uid"] = player->uid;
	packet.val["prop_id"] = prop_id;
	packet.val["forbidden_card"] = player->forbidden_card;
	packet.val["is_forbidden"] = seat.is_forbidden;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler forbidden card uid[%d] seatid[%d] is_forbidden[%d] forbidden_card[%d] tid[%d].\n", player->uid, player->seatid, seat.is_forbidden, player->forbidden_card, tid);
	return 0;
}

int Table::handler_double_card(Player *player, int multiple)
{
	Json::Value &val = player->client->packet.tojson();
	int seatid = val["seatid"].asInt();
	Seat &seat = seats[player->seatid];
	int prop_id = val["prop_id"].asInt();

	if (seat.betting == 0) {
		handler_prop_error(player, prop_id);
		xt_log.error("handler double card error. betting[0] uid[%d] seatid[%d] tid[%d].\n", player->uid, player->seatid, tid);
		return -1;
	}
	if (player->double_card < 1) {
		handler_prop_error(player, prop_id);
		xt_log.error("handler double card not card. uid[%d] seatid[%d] double_card[%d] tid[%d].\n", player->uid, player->seatid, player->double_card, tid);
		return -1;
	}

	seat.is_double = multiple;
	player->incr_double_card(1);
	Jpacket packet;
	packet.val["cmd"] = SERVER_PROP_SUCC_BC;
	packet.val["seatid"] = seatid;
	packet.val["uid"] = player->uid;
	packet.val["prop_id"] = prop_id;
	packet.val["double_card"] = player->double_card;
	packet.val["is_double"] = seat.is_double;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("handler double card uid[%d] seatid[%d] is_double[%d] double_card[%d] tid[%d].\n", player->uid, player->seatid, seat.is_double, player->double_card, tid);
	return 0;
}

void Table::handler_prop_error(Player *player, int prop_id)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_PROP_ERR_UC;
	packet.val["uid"] = player->uid;
	packet.val["prop_id"] = prop_id;
	packet.end();
	unicast(player, packet.tostring());
}

void Table::recover_forbidden_card()
{
	for (int i = 0; i < seat_max; i++) {
		if (seats[i].betting == 1) {
			if (seats[i].is_forbidden == cur_round)
				seats[i].is_forbidden = 0;
		}
	}
}

UserState Table::getUserState(void) const
{
    return m_userState;
}

bool Table::isRobotChange(void) const
{
   int userState = getUserState();
   if(userState == us_none || userState == us_all_human)
   {
        return false;
   }

   if(userState == us_all_robot)
   {
        return true;
   }
    
   if(userState == us_mix)
   {
        return players.size() > 3;
   }
   return false;
}

void Table::robotChange(void)
{
    if(!isRobotChange())
    {
       return;  
    }

    for (std::map<int, Player*>::iterator it = players.begin(); it != players.end(); it++) 
    {
        if(it->first < XT_ROBOT_UID_MAX) 
        {
            handler_robot_change_uc(it->second);
            break;
        }
    }
}

int Table::handler_robot_change_uc(Player *player)
{
    if(!player)
    {
        return 0;
    }
	Jpacket packet;
	packet.val["cmd"] = SERVER_ROBOT_CHANGE_UC;
	packet.end();
	unicast(player, packet.tostring());
	return 0;	
}
    
void Table::resetRobotMoney(Player *player)
{
    if((player->money < min_money) || (max_money > 0 && player->money > max_money))
    {
        int min = min_money;
        int max = std::max(max_money, 10 * min_money);
        int tmp = getRandBetween(min, max);
        player->set_money(tmp); 
        if(getUserState() == us_mix)
        {
            updateInfoBC(player);
        }
    }
}
    
void Table::updateInfoBC(Player* player)
{
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
	broadcast(NULL, packet.tostring());
}
    
void Table::refreshUserState(void)
{
    if(players.empty())
    {
        m_userState = us_none;
        return;
    }

    int human_num = 0;
    int robot_num = 0;
    for (std::map<int, Player*>::iterator it = players.begin(); it != players.end(); it++) 
    {
        if(it->first > XT_ROBOT_UID_MAX) 
        {
            human_num++;
        }
        else
        {
            robot_num++;
        }
    }

    if (human_num == 0) 
    {
        m_userState = us_all_robot;
        return;
    }

    if (robot_num == 0) 
    {
        m_userState = us_all_human;
        return;
    }

    m_userState = us_mix;
}
    
int Table::getRandBetween(int _min, int _max) 
{
    int min = std::min(_min, _max);
    int max = std::max(_min, _max);
    return rand() % (max - min + 1) + min;
}
