#include <iostream>
#include <algorithm>
#include <vector>
#include <ctime>
#include <cstdlib>

#include "speaker.h"
#include "log.h"
#include "proto.h"
#include "client.h"
#include "player.h"
#include "server.h"
#include "lottery.h"
#include "eventlog.h"

extern Speaker speaker;
extern Log xt_log;

// 0  方块  1 梅花   2  红桃    3黑桃
static int card_d[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
static int card_c[] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};
static int card_h[] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D};
static int card_s[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D};

Lottery::Lottery() :
wait_timer_stamp(10),
lottery_timer_stamp(60 * 2),
update_timer_stamp(2)
{
	all_cards.clear();
	c_diamonds.clear();
	c_clubs.clear();
	c_hearts.clear();
	c_spades.clear();

	issue = 10000;
	total_money = 0;
	total_count = 0;
	single_money = 0;
	lstate = 0;
	card_type = 0;
	awards_count = 0;

	action_client.clear();
	awards_client.clear();
	bet_client.clear();

	wait_timer.data = this;
	ev_timer_init(&wait_timer, Lottery::wait_timer_cb, wait_timer_stamp, wait_timer_stamp);

	lottery_timer.data = this;
	ev_timer_init(&lottery_timer, Lottery::lottery_timer_cb, lottery_timer_stamp, lottery_timer_stamp);

	update_timer.data = this;
	ev_timer_init(&update_timer, Lottery::update_timer_cb, update_timer_stamp, update_timer_stamp);

}

Lottery::~Lottery()
{
	ev_timer_stop(speaker.loop, &wait_timer);
	ev_timer_stop(speaker.loop, &lottery_timer);
}

int Lottery::init(Server *s)
{
	int i;
	for (i = 0; i < 13; i++) {
		c_diamonds.push_back(card_d[i]);
		c_clubs.push_back(card_c[i]);
		c_hearts.push_back(card_h[i]);
		c_spades.push_back(card_s[i]);
	}

	all_cards.push_back(c_diamonds);
	all_cards.push_back(c_clubs);
	all_cards.push_back(c_hearts);
	all_cards.push_back(c_spades);

	server = s;
	lottery_money = server->lottery_money;

	return 0;
}

int Lottery::start()
{
	ev_timer_again(speaker.loop, &wait_timer);
	return 0;
}

void Lottery::reset()
{
	card_type = 0;
	total_count = 0;
	single_money = 0;
	awards_count = 0;
	c_awards.clear();
	awards_client.clear();
	bet_client.clear();
}

void Lottery::wait_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	xt_log.debug("wait timer cb.\n");
	Lottery *lottery = (Lottery *)w->data;
	ev_timer_stop(speaker.loop, &lottery->wait_timer);
	lottery->start_bet();
}

void Lottery::lottery_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	xt_log.debug("lottery timer cb.\n");
	Lottery *lottery = (Lottery *)w->data;
	ev_timer_stop(speaker.loop, &lottery->lottery_timer);
	ev_timer_stop(speaker.loop, &lottery->update_timer);
	lottery->stop_bet();
}

void Lottery::update_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	xt_log.debug("lottery update timer cb.\n");
	Lottery *lottery = (Lottery *)w->data;
	lottery->action_update();
}

int Lottery::start_bet()
{
	xt_log.info("lottery start bet.\n");
	reset();

	int ret;
	ret = shuffle_card();
	if (ret < 0) {
		xt_log.error("lottery start bet error.\n");
		return -1;
	}
	issue += 1;
	lstate = 1;
	xt_log.info("lottery issue[%d] total_money[%d].\n", issue, total_money);

	ev_timer_again(speaker.loop, &lottery_timer);
	ev_timer_again(speaker.loop, &update_timer);

	Jpacket packet;
	packet.val["cmd"] = SERVER_LOTTERY_START;
	packet.val["lstate"] = lstate;
	packet.val["issue"] = issue;
	packet.val["total_money"] = total_money;
	packet.val["betting_ts"] = get_betting_remain();
	packet.end();
	server->broadcast(NULL, packet.tostring());

	return 0;
}

int Lottery::stop_bet()
{
	xt_log.info("lottery stop bet.\n");
	lstate = 0;
	ev_timer_again(speaker.loop, &wait_timer);

//	int start = int(time(NULL));
//	xt_log.debug("stop bet time start[%d].\n", start);
	xt_log.info("lottery stop bet card_type[%d].\n", card_type);

	int ret = handler_awards();
	if (ret < 0) {
		xt_log.error("lottery stop bet handler_awards error.\n");
		return -1;
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_LOTTERY_STOP;
	packet.val["card_type"] = card_type;
	packet.val["total_money"] = total_money;
	packet.val["total_count"] = total_count;
	packet.val["win_numbers"] = awards_count;
	packet.val["single_money"] = single_money;
	packet.val["lstate"] = lstate;
	packet.val["wait_ts"] = get_wait_remain();

	std::list<int>::iterator it;
	for (it = c_awards.begin(); it != c_awards.end(); ++it) {
		packet.val["cards"].append(*it);
		xt_log.info("stop bet cards[%d].\n", *it);
	}
	packet.end();
	server->broadcast(NULL, packet.tostring());
//	action_broadcast(packet.tostring());

	if (awards_count > 0)
		total_money = 0;  // end total money set 0

//	int end = int(time(NULL));
//	xt_log.debug("stop bet time end[%d] differ[%d].\n", end, end - start);

	return 0;
}

int Lottery::handler_betting(Client *client, int type,int bet_nu)
{
	if (lstate == 0) {
		xt_log.error("handle betting is not betting lstate[%d].\n", lstate);
		return -1;
	}

	total_money += bet_nu*lottery_money;
	total_count += bet_nu;

	int _uid = client->uid;
	if (type == card_type) 
	{
		if(awards_client.find(_uid) != awards_client.end()) 
		{
			awards_client[_uid] += bet_nu;
		} 
		else 
		{
			awards_client[_uid] = bet_nu;
		}

		awards_count += bet_nu;

		xt_log.info("lottery beeting win uid[%d] type[%d] card_type[%d].\n", _uid, type, card_type);

	}
   	else 
	{
		xt_log.info("lottery beeting lose uid[%d] type[%d] card_type[%d].\n", _uid, type, card_type);
	}


	if (bet_client[type].find(_uid) != bet_client[type].end()) 
	{
		bet_client[type][_uid] += bet_nu;
	}
   	else 
	{
		bet_client[type][_uid] = bet_nu;
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_BETTING_SUCC;
	packet.val["total_money"] = total_money;
	packet.val["total_count"] = total_count;
	packet.val["card_type"] = type;

	packet.val["bet_nu"] = bet_client[type][_uid];

	packet.end();
	client->send(packet.tostring());

	return 0;
}

int Lottery::handler_awards()
{
	if (total_count == 0) 
	{
		xt_log.info("handler_awards total_money[%d] total_count[%d].\n", total_money, total_count);
		return 0;
	}

	if (awards_count == 0) 
	{
		xt_log.info("handler_awards no player awards_count[%d].\n", awards_count);
		return 0;
	}


	single_money = (total_money / awards_count )*0.95f;

	xt_log.info("single lottery total_money[%d] single_money[%d] total_count [%d].\n", total_money, single_money, awards_count);

	for (std::map<int,int>::iterator it=awards_client.begin(); it!=awards_client.end(); ++it) 
	{
		int _uid = it->first;
		int value = it->second * single_money;
		int cur_money=incr_money(_uid, value);
		eventlog_update(_uid, single_money*it->second,cur_money);
		incr_achievement_count(_uid, 1);
//		send_awards_unicast(_uid, value);
	}

	return 0;
}

int Lottery::action_broadcast(const std::string &packet)
{
	Client *client;
	std::map<int, Client*>::iterator iter;
	iter = action_client.begin();

	xt_log.info("action account num:%d \n",action_client.size());


	for (; iter != action_client.end(); iter++) 
	{
		client = iter->second;
		if (client == NULL)
		{
			continue;
		}

		//xt_log.info("send to uid(%d) ",client->uid);

		client->send(packet);

	}

	return 0;
}

int Lottery::random_type(int start, int end, int seed)
{
	std::vector<int> myvector;
	for (int i = start; i <= end; ++i) {
		myvector.push_back(i);
	}
	std::srand(unsigned(std::time(NULL)) + seed);
	std::random_shuffle(myvector.begin(), myvector.end());
	return myvector[0];
}

int Lottery::shuffle_card()
{
	card_type = random_type(1, 6, issue);
	xt_log.info("lottery shuffle card card_type[%d].\n", card_type);

	switch (card_type) {
	case CARD_TYPE_BAOZI:
		handler_baozi();
		break;
	case CARD_TYPE_SHUNJIN:
		handler_shunjin();
		break;
	case CARD_TYPE_JINHUA:
		handler_jinhua();
		break;
	case CARD_TYPE_SHUNZI:
		handler_shunzi();
		break;
	case CARD_TYPE_DUIZI:
		handler_duizi();
		break;
	case CARD_TYPE_DANPAI:
		handler_gaopai();
		break;
	default:
		xt_log.error("lottery shuffle card error card_type[%d].\n", card_type);
		return -1;
	}
	c_awards.sort();

	std::list<int>::iterator it;
	for (it = c_awards.begin(); it != c_awards.end(); ++it) {
		xt_log.info("lottery shuffle cards: %d .\n", *it);
	}

	return 0;
}

int Lottery::handler_baozi()
{
	int face = random_type(0, 12, issue);
	int suit = random_type(0, 3, issue+1);

	int i;
	for (i = 0; i <= 3; i++) {
		if (suit == i)
			continue;
		c_awards.push_back(all_cards[i][face]);
	}

	return 0;
}

int Lottery::handler_shunjin()
{
	int face = random_type(1, 11, issue);  // 保证不会抽到A或K
	int suit = random_type(0, 3, issue+1);

	int i;
	for (i = 0; i <= 3; i++) {
		if (suit == i) {
			c_awards.push_back(all_cards[i][face-1]);
			c_awards.push_back(all_cards[i][face]);
			c_awards.push_back(all_cards[i][face+1]);
		}
	}

	return 0;
}

int Lottery::handler_jinhua()
{
	int suit = random_type(0, 3, issue);
	int face1 = random_type(0, 4, issue+1);
	int face2 = random_type(5, 8, issue+2);
	int face3 = random_type(9, 12, issue+3);

	int i;
	for (i = 0; i <= 3; i++) {
		if (suit == i) {
			c_awards.push_back(all_cards[i][face1]);
			c_awards.push_back(all_cards[i][face2]);
			c_awards.push_back(all_cards[i][face3]);
		}
	}

	return 0;
}

int Lottery::handler_shunzi()
{
	int face = random_type(0, 10, issue);  // 保证不会抽到Q K
	int suit = random_type(0, 3, issue+1);

	int i;
	for (i = 0; i <= 3; i++) {
		if (suit == i)
			continue;
		c_awards.push_back(all_cards[i][face]);
		face = face + 1;
	}

	return 0;
}

int Lottery::handler_duizi()
{
	int face = random_type(0, 11, issue);  // 保证不会抽到K
	int suit = random_type(0, 2, issue+1);   // 保证不会抽到spades

	bool single = false;
	int i;
	for (i = 0; i <= 3; i++) {
		if (suit == i) {
			c_awards.push_back(all_cards[i][face]);
			c_awards.push_back(all_cards[i+1][face]);
		} else {
			if (single)
				continue;
			c_awards.push_back(all_cards[i][face]);
			single = true;
		}
		if (c_awards.size() == 3)
			break;
		if (face > 8)
			face = face + 1;
		else
			face = face + 4;
	}

	return 0;
}

int Lottery::handler_gaopai()
{
	int suit = random_type(0, 3, issue);
	int face1 = random_type(0, 4, issue+1);
	int face2 = random_type(5, 8, issue+2);
	int face3 = random_type(9, 12, issue+3);
	int face[] = {face1, face2, face3};

	int c = 0;
	int i;
	for (i = 0; i <= 3; i++) {
		if (suit == i)
			continue;
		c_awards.push_back(all_cards[i][face[c]]);
		c++;
	}

	return 0;
}

int Lottery::get_betting_remain()
{
	int remain = (int)ev_timer_remaining(speaker.loop, &lottery_timer);
	int ts = (int)time(NULL);
	return ts + remain;
	return remain;
}

int Lottery::get_wait_remain()
{
	int remain = (int)ev_timer_remaining(speaker.loop, &wait_timer);
	int ts = (int)time(NULL);
	return ts + remain;
	return remain;
}

int Lottery::handler_action(Client *client, int action_type)
{
	if (action_type == 1) {
		if (action_client.find(client->uid) == action_client.end()) {
			action_client[client->uid] = client;
		}
		Jpacket packet;
		packet.val["cmd"] = SERVER_ACTION_SUCC;
		packet.val["uid"] = client->uid;
		packet.val["action"] = action_type;
		packet.val["total_money"] = total_money;
		packet.val["total_count"] = total_count;
		packet.val["lstate"] = lstate;
		if (lstate == 0) {
			packet.val["wait_ts"] = get_wait_remain();
			packet.val["win_numbers"] = awards_count;
			packet.val["single_money"] = single_money;
			packet.val["card_type"] = card_type;
			std::list<int>::iterator it;
			for (it = c_awards.begin(); it != c_awards.end(); ++it) {
				packet.val["cards"].append(*it);
			}
		}
		else {
			packet.val["betting_ts"] = get_betting_remain();
		}
		packet.end();
		client->send(packet.tostring());
	} else {
		if (action_client.find(client->uid) != action_client.end()) {
			action_client.erase(client->uid);
		}
	}

	xt_log.info("handler action uid[%d] action_type[%d].\n", client->uid, action_type);
	return 0;
}

int Lottery::handler_login(Client *client)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGIN_SUCC;
	packet.val["uid"] = client->uid;
	packet.val["total_money"] = total_money;
	packet.val["total_count"] = total_count;
	packet.val["lstate"] = lstate;
	if (lstate == 0)
		packet.val["wait_ts"] = get_wait_remain();
	else
		packet.val["betting_ts"] = get_betting_remain();
	packet.val["current_ts"] = (int)time(NULL);

	int _uid = client->uid;
	int i = 0;
	std::map<int, std::map<int, int> >::iterator iter;
	iter = bet_client.begin();
	for (; iter != bet_client.end(); iter++) {
		std::map<int, int> tmp = iter->second;
		if (tmp.find(_uid) != tmp.end()) {
			packet.val["bet"][i]["card_type"] = iter->first;
			packet.val["bet"][i]["bet_count"] = tmp[_uid];
			i++;
		}
	}

	packet.end();
	client->send(packet.tostring());

	xt_log.info("handler_login uid[%d] lstate[%d].\n", client->uid, lstate);

	return 0;
}

int Lottery::action_update()
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_ACTION_UPDATE;
	packet.val["total_money"] = total_money;
	packet.val["total_count"] = total_count;
	packet.val["lstate"] = lstate;
	packet.end();

	action_broadcast(packet.tostring());

	xt_log.info("action update total_money[%d] total_count[%d] lstate[%d].\n", total_money, total_count, lstate);

	return 0;
}

int Lottery::handler_del_action(int uid)
{
	if (action_client.find(uid) != action_client.end()) {
		action_client.erase(uid);
	}
	return 0;
}




int Lottery::incr_money(int uid, int value)
{
	if(uid<XT_LOTTERY_ROBOT_MAX_ID)
	{
		return 0;
	}


	int index = uid % speaker.main_size;
	int ret = speaker.main_rc[index]->command("hincrby hu:%d money %d", uid, value);

	if (ret < 0) 
	{
		xt_log.error("incr money uid[%d] incr_money error.\n", uid);
		return -1;
	}
	//	xt_log.info("incr money money new[%d].\n", speaker.main_rc[index]->reply->integer);

	return 0;
}



int Lottery::eventlog_update(int uid, int value, int money)
{
	int ret;
	ret = commit_eventlog(uid, 0, value, money, 205, 2);
	if (ret < 0) 
	{
		xt_log.error("eventlog update error value[%d].\n", value);
		return -1;
	}
	return 0;
}

int Lottery::incr_achievement_count(int uid, int value)
{
	if (value == 0) {
		return 0;
	}
	int index = uid % speaker.main_size;
	int ret = speaker.main_rc[index]->command("hincrby st:%d lottery_win %d", uid, value);
	if (ret < 0) 
	{
		xt_log.error("incr achievement count error value[%d].\n", value);
		return -1;
	}
	return 0;
}

int Lottery::send_awards_unicast(int uid, int value)
{
	Jpacket packet_win;
	packet_win.val["cmd"] = SERVER_LOTTERY_WIN;
	packet_win.val["single_money"] = single_money;
	packet_win.val["card_type"] = card_type;
	packet_win.val["uid"] = uid;
	packet_win.val["prize"] = value;
	packet_win.end();

	Client *client = server->uid_client[uid];
	if (client == NULL)
		return -1;
	client->send(packet_win.tostring());
	return 0;
}



