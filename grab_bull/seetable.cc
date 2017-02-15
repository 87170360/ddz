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
///---��ƥ�����ˮ�����ѡׯ����5���ơ����ơ���ׯ�ұ��ơ����㣩
SeeTable::SeeTable() :
  ready_timer_stamp(5),              ///1׼��ʱ�䣨ƥ���ˣ�
  banker_timer_stamp(2),             ///2���ѡׯʱ��
  send_card_timer_stamp(2),          ///3����
  group_card_timer_stamp(GROUP_TIME),///4����
  compare_timer_stamp(7),            ///5����ʱ��   
  robot_timer_stamp(6)               ///������ʱ��  
  preready_timer_stamp(2),           ///Ԥ��ʱ��
{
 	///1׼�������ʱ���������ҽ��������뿪��
    ready_timer.data = this;
    ev_timer_init(&ready_timer, SeeTable::ready_timer_cb, ready_timer_stamp, ready_timer_stamp);

	///2���ׯ��
	banker_timer.data = this;
	ev_timer_init(&banker_timer, SeeTable::banker_timer_cb, banker_timer_stamp, banker_timer_stamp);

	///3����
	send_card_timer.data = this;
	ev_timer_init(&send_card_timer, SeeTable::send_card_timer_cb, send_card_timer_stamp,
		send_card_timer_stamp);

	///4����
	group_card_timer.data = this;
	ev_timer_init(&group_card_timer, SeeTable::group_card_timer_cb, group_card_timer_stamp,
		group_card_timer_stamp);

	///5����
    compare_timer.data = this;
    ev_timer_init(&compare_timer, SeeTable::compare_timer_cb, compare_timer_stamp,
                    compare_timer_stamp);

	///6Ԥ���������Ϊ��һ�ֻؿ�ʼ��
	preready_timer.data = this;
	ev_timer_init(&preready_timer, SeeTable::preready_timer_cb, preready_timer_stamp,
		preready_timer_stamp);

    robot_timer.data = this;
    ev_timer_init(&robot_timer, SeeTable::robot_timer_cb, robot_timer_stamp,
    		robot_timer_stamp);


	int weight[6]={52*1.5,48*1.5,1096*1.5,720*1.5,3744*1.5,16440/2}; ///---����Ȩ��ֵ
	deck.setTypeWeight(weight);     ///---����ÿ�����͵�Ȩ��
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

///���ӳ�ʼ��
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
	tid = my_tid;                ///����
    vid = my_vid;                ///���ݺţ��߼����м������ܵȣ�

    min_money = my_min_money;    ///Я���������
	max_money = my_max_money;    ///Я���������
	base_money = my_base_money;  ///��ע
 
	fee = my_fee;                ///��ˮ����

	lose_exp = my_lose_exp;      ///��ľ���ֵ
	win_exp = my_win_exp;        ///Ӯ�ľ���ֵ
	seat_max = 5;                ///��λ��
	
    cur_players = 0;             ///��ǰ��Ҹ���
	players.clear();             ///������map
	ready_players = 0;           ///��׼���õ������

	///��λidΪ�����±�
    for (int i = 0; i < seat_max; i++)
    {
		seats[i].clear();
        seats[i].seatid = i;   
    }
	
	reset();
	state = WAIT;             ///��ʼ��ǰ״̬Ϊ���ȴ���WAIT

    return 0;
}


///��������ҹ㲥(�������p,�����p=NULL,�������е���)
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

///����
int SeeTable::unicast(Player *p, const std::string &packet)
{
    if (p->client)
    {
        return p->client->send(packet);
    }
    return -1;
}

///[starr, end]֮��������
int SeeTable::random(int start, int end)
{
	return start + rand() % (end - start + 1);
}

///����״̬��λ
void SeeTable::reset()   
{
	state = READY;
	ready_players = 0;
}

///vectorתJSON����
void SeeTable::vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
	for (unsigned int i = 0; i < cards.size(); i++) {
		packet.val[key].append(cards[i].m_value);
	}

	if (cards.size() == 0) {
		packet.val[key].append(0);
	}
}

///mapתJSON����
void SeeTable::map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key)
{
	std::map<int, XtCard>::iterator it;
	for (it = cards.begin(); it != cards.end(); it++)
	{
		XtCard &card = it->second;
		packet.val[key].append(card.m_value);
	}
}

///JSON����תvector
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
   �û�����  
*/
int SeeTable::handler_group_card(Player *player)
{
	///---���������ѵ�ʱ��
	seats[player->seatid].goup_time = time(0) - start_group_time;

	///---��¼��ҵ�������Ϣ
	if (players.find(player->uid) == players.end()) {
		players[player->uid] = player;
		player->tid = tid;

		/// ������λ
		player->seatid = sit_down(player);
		if (player->seatid < 0) {
			return -1;    ///---����ʧ�ܷ���
		}
		
		Seat &seat = seats[player->seatid];
		seat.uid = player->uid;
		cur_players++;      ///---�ۼƵ�ǰ��Ҹ���

		handler_login_succ_uc(player);
		handler_login_succ_bc(player);
		handler_table_info(player);

		xt_log.info("handler login succ uid[%d] money[%d] cur_players[%d] tid[%d] state[%d]. \n", player->uid, player->money, cur_players, tid,state);

		///---��ǰ״̬WAIT״̬���������>=2�ˣ�תΪ׼��״̬
		if((WAIT == state) && (cur_players>=2))   
		{
			xt_log.info("change state to ready \n");
			state = READY;
			ev_timer_again(zjh.loop, &ready_timer);   ///---������׼������ʱ��
		}
		return 0;
	}

	///---��������������ϣ�ͣ�����ƶ�ʱ����������ƽ׶�
	if (false)
	{
		ev_timer_stop(zjh.loop, &table->group_card_timer);
		///���롰���ơ�����
		ev_timer_again(zjh.loop, &compare_timer);
	}

	return 0;
}



/**
   ��ҵ�¼����
*/
int SeeTable::handler_login(Player *player)
{
	if (players.find(player->uid) == players.end()) {

		players[player->uid] = player;  
		player->tid = tid;

		/// ������λ��
		player->seatid = sit_down(player);
		if (player->seatid < 0) {
			return -1;    ///---����ʧ�ܷ���
		}
		
		Seat &seat = seats[player->seatid];
		seat.uid = player->uid;
		cur_players++;      ///---�ۼƵ�ǰ��Ҹ���

		handler_login_succ_uc(player);
		handler_login_succ_bc(player);
		handler_table_info(player);

		xt_log.info("handler login succ uid[%d] money[%d] cur_players[%d] tid[%d] state[%d]. \n", player->uid, player->money, cur_players, tid,state);

		///---��ǰ״̬WAIT״̬���������>=2�ˣ���Ϸ״̬Ϊ׼��
		if((WAIT == state) && (cur_players>=2))   
		{
			xt_log.info("change state to ready \n");
			state = READY;
			ev_timer_again(zjh.loop, &ready_timer);   ///---������׼������ʱ��
		}
		return 0;
	}
	return -1;
}

///���£�������λ��
int SeeTable::sit_down(Player *player)
{
	std::vector<int> tmp;     ///����λ�б�
    for (int i = 0; i < seat_max; i++)
    {
		if (seats[i].occupied == 0)
		{
			tmp.push_back(i);
		}
    }
	
	///�п�λ�����ڿ�λ�б������ѡһ��λ������
	int len = tmp.size();
	if (len > 0)    
	{
		int index = random(0, len - 1);
		int i = tmp[index];
		xt_log.debug("len[%d] index[%d] i[%d]\n", len, index, i);

		seats[i].occupied = 1;     ///�ɹ�������λ
		seats[i].player = player;
		return i;
	}

	return -1;
}

///վ���������λ
void SeeTable::stand_up(Player *player)
{
	seats[player->seatid].clear();
}

///ɾ�����
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
	player->stop_offline_timer();     ///---ֹͣ"���߶�ʱ��"
	players.erase(player->uid);
	stand_up(player);
	cur_players--;
	
	return 0;
}
///---��¼�ɹ���Ӧ��
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

///---���з������ݸ���¼��
int SeeTable::handler_table_info(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_TABLE_INFO_UC;
	packet.val["vid"] = player->vid;        ///---����id
	packet.val["tid"] = player->tid;        ///--���ӵ�id   
	packet.val["seatid"] = player->seatid;  ///--λ��id
	packet.val["state"] = state;            ///---״̬
	packet.val["dealer"] = dealer;          ///---ׯ��id


	std::map<int, Player*>::iterator it;    ///---����б�
	int i = 0;
	for (it = players.begin(); it != players.end(); it++) {
		Player *p = it->second;
		Seat &seat = seats[p->seatid];

		packet.val["players"][i]["uid"]     = p->uid;       ///���id
		packet.val["players"][i]["seatid"]  = p->seatid;    ///λ��id
		packet.val["players"][i]["ready"]   = seat.ready;   ///�Ƿ�׼��		
		packet.val["players"][i]["role"]    = seat.role;    ///�Ƿ���ׯ��
		packet.val["players"][i]["status"]  = seat.status;  ///��ǰ״̬
		if (player == p) {
			///�������(���ֶ�ֻ�����Լ�ʱ������)
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

// ��¼�ɹ������ҡ�����Ϣ�㲥��ȥ
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



///---Ԥ����ʱ�ص�
void SeeTable::preready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->preready_timer);   ///---ֹͣԤ����ʱ��

	table->handler_preready();
}

///Ԥ������(���Ӻ���Ҹ�λ�����������Ƿ����������Ϸ������)
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

			if (player->money < min_money)    ///��ǰ���<����
			{
				if(player->uid<XT_ROBOT_UID_MAX)  ///��Ϊ������,�����Ǯ[200, 375]*min_money
				{
					player->set_money((int)(float(rand()%8+8)/4*100*min_money));
				}
				else 
				{
					temp[player->uid]=player;   ///---��¼Ǯ���������
				}
			}


			if (player->idle_count > 2)         ///---��¼�������(û�ж��������)
			{
				temp[player->uid] = player;
				player->logout_type = 1;  
			}
		} 
		else 
		{
			xt_log.debug("handler preready uid[%d] is offline.\n", player->uid);
			temp[player->uid] = player;     ///---��¼�������
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
	broadcast(NULL, packet.tostring());   ///�㲥�����ˣ���ϷԤ��


	/* check state */
	if(cur_players>=2)
	{
		state=READY;     ///״̬תΪREADY
		ev_timer_again(zjh.loop,&ready_timer);
	}
	else 
	{
		state=WAIT;
	}

	return 0;
}


///---��׼������ʱ�ص�
void SeeTable::ready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*)w->data;
	ev_timer_stop(zjh.loop, &table->ready_timer);

	table->handler_ready();
}

///---׼�����ڽ�������Ϸ��ʼ
int SeeTable::handler_ready()
{
	if (state != READY) {   ///״̬����,ֱ�ӷ���
		xt_log.error("ready timer  state error(%d)",state);
		return -1;
	}

	if (cur_players >= 2)   ///������ڵ������ˣ�����Ϸ��ʼ
	{
		game_start();
	}
	else
	{
		state=WAIT;   ///---�������������˿������ˣ���������Ϊ׼��״̬
	}
	return 0;
}

///---��Ϸ��ʼ��ִ�п�˰������ѡׯ���ڣ�
int SeeTable::game_start()
{
	///��Ϸ״̬��ѡׯ��
	state = BANKER;

	///������λ״̬
	for(int i = 0; i < seat_max; i++)   
	{
		if(seats[i].occupied)
		{
			seats[i].status = 1;    ///1��λΪ����״̬
		}
	}

	///1��˰(��ˮ)
	tax()

	///����"���ѡׯ"����
	ev_timer_again(zjh.loop, &banker_timer);

	return 0;
}


//��˰
void SeeTable::tax()
{
	///��������ҳ�ˮ
	Player *player;
	std::map<int, Player*>::iterator it;
	for (it = players.begin(); it != players.end(); it++)
	{
		player = it->second;
		if (player->client == NULL)
		{
			continue;
		} 
		///---���¿�˰
		tax_update(player);
	}
}

///---���¿�˰
void SeeTable::tax_update(Player *player)
{
	Seat &seat = seats[player->seatid];
	int value = -fee;
	player->incr_money(0, value);

	///---to do ������־����
	//player->eventlog_update(0, value);
	//player->eventlog_update(2, tips);

	handler_tax_uc(player);
}

///---�㲥��˰�ɹ�(�ܿ��������˵ĵ�ǰ���)
int SeeTable::handler_tax_uc(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_TAX_SUCC_BC;
	packet.val["vid"] = player->vid;        ///---����id
	packet.val["tid"] = player->tid;        ///--���ӵ�id   

	std::map<int, Player*>::iterator it;  ///---����б�
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

///---"����"��ʱ, ϵͳΪû������ҽ������ƣ��������ƻ���
void SeeTable::group_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*)w->data;
	ev_timer_stop(zjh.loop, &table->group_card_timer);	

	table->group_card_timeout();
}


///---"����"ʱ��������㲥����
void SeeTable::group_card_timeout()
{	
	if (state != GROUP_CARD) {    ///---״̬����ֱ�ӷ���
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
			dealer = player->seatid;    ///---��¼��ǰׯ��
			break;
		}

		tmpCount++;
	}

	///---�㲥������ҵ���
	Jpacket packet;
	packet.val["cmd"] = SERVER_MAKE_BANKER_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.end();
	broadcast(NULL, packet.tostring());

	///���롰���ơ�����
	state = COMPARE;
	ev_timer_again(zjh.loop, &compare_timer);
}


///"���ׯ��"ʱ�䵽��
void SeeTable::banker_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->banker_timer);
	
	table->banker_timeout();
}

///ȷ�����ׯ��
void SeeTable::banker_timeout()
{
	if (state != BANKER) {    ///---״̬����ֱ�ӷ���
		xt_log.error("banker timer state error(%d)",state);
		return -1;
	}

	xt_log.debug("banker_timer_cb\n");
	///���ѡ��һ��ׯ��(todo ǮԽ���Խ�ʺ���ׯ��)
	int index = random(0, players.size() -1);

	int tmpCount = 0;    ///--��ʱ������
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
			dealer = player->seatid;    ///---��¼��ǰׯ��
			break;
		}

		tmpCount++;
	}

	///---�㲥�����ׯ�ҡ�
	Jpacket packet;
	packet.val["cmd"] = SERVER_MAKE_BANKER_BC;
	packet.val["uid"] = player->uid;
	packet.val["seatid"] = player->seatid;
	packet.end();
	broadcast(NULL, packet.tostring());

	xt_log.info("banker timeout uid[%d] seatid[%d] tid[%d].\n", player->uid, player->seatid, tid);

	///���뷢�ƻ���
	state = SEND_CARD;
	ev_timer_again(zjh.loop, &send_card_timer);
}

///"����"ʱ�䵽��
void SeeTable::send_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->banker_timer);
	
	///����
	table->send_card_timeout();
}

///---��������ҷ���
void SeeTable::send_card_timeout()
{
	if (state != SEND_CARD) {    ///---״̬����ֱ�ӷ���
		xt_log.error("send card timer state error(%d)",state);
		return -1;
	}
	xt_log.debug("send_card_timeout\n");

	///׼������
	deck.fill();
	deck.shuffle(tid);


	Jpacket packet;
	packet.val["cmd"] = SERVER_GAME_START_BC;     ///---��Ϸ��ʼ�㲥
	int current_betting_seats = count_betting_seats();
	int next = next_betting_seat(dealer);   ///---��һ��betting��λ

	///---����ҷ���
	for (int c = 0; c < current_betting_seats; c++) 
	{
		Seat &seat = seats[next];
		Player *player = seat.player;

		if (player == NULL)   ///���û�У����ֵ���һ�����
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

		deck.getHoleCards(&seat.hole_cards);   ///---����
		seat.hole_cards.analysis();            ///������

		//		Card::sort_by_descending(seat.hole_cards.cards);
		xt_log.info("game start cards[%d, %d, %d] uid[%d] money[%d] seatid[%d] tid[%d].\n",
			seat.hole_cards.m_cards[0].m_value, seat.hole_cards.m_cards[1].m_value,
			seat.hole_cards.m_cards[2].m_value, player->uid, player->money, seat.seatid, tid);
		player->money -= cur_bet; // to check money enough   ///---��ȥ��ǰ��ҵ���ע
		seat.bet += cur_bet;      ///---�ۼƵ�ǰ��λ�Ķ���
		total_bet += cur_bet;     ///---ȫ������
		next = next_betting_seat(next);
		packet.val["seatids"].append(seat.seatid);
	}
	packet.val["dealer"] = dealer;         ///---��ǰ���
	packet.val["cur_round"] = cur_round;   ///---��ǰ�غ�
	packet.val["cur_bet"] = cur_bet;       ///---��ǰ����    
	packet.val["total_bet"] = total_bet;   ///---�ܶ���
	packet.end();
	broadcast(NULL, packet.tostring());      ///---�㲥��Ϸ��ʼ��

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



	///���롰���ơ�����
	state = GROUP_CARD;
	ev_timer_again(zjh.loop, &group_card_timer);
	///��¼���ƿ�ʼʱ�� 
	start_group_time = time(0);
}



///"����"ʱ�����, ������Ӯ����
void SeeTable::compare_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	SeeTable *table = (SeeTable*) w->data;
	ev_timer_stop(zjh.loop, &table->compare_timer);

	table->compare_timeout();
}

///���ƽ���
void SeeTable::compare_timeout()
{
	if (state != COMPARE) {    ///---״̬����ֱ�ӷ���
		xt_log.error("compare timer state error(%d)",state);
		return -1;
	}

	xt_log.debug("compare_timer_cb\n");


	///��Ϸ����
	state = END_GAME;
	ev_timer_again(zjh.loop, &preready_timer_cb);
}



///---�䷽�������������
void SeeTable::lose_update(Player *player)
{
	Seat &seat = seats[player->seatid];
	player->incr_money(1, seat.bet);
	player->incr_total_board(vid, 1);
	player->incr_achievement_count(vid, 1, 1);

	// cfc add eventlog 20130102
	player->eventlog_update(1, seat.bet);
}

///---Ӯ���������������
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
	if (betting_players == 1) {   ///ֻ��һ����ң���Ϸ����
		game_end();
		return 0;
	}
	return -1;
}
///��Ϸ����
int SeeTable::game_end()
{
	//--����
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
	broadcast(NULL, packet.tostring());   ///---�㲥��Ϸ����

	ev_timer_again(zjh.loop, &preready_timer);

	return 0;
}

///---����ǳ�
int SeeTable::handler_logout(Player *player)
{	
	if (state == BETTING) {   ///---����betting�����ƴ���
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
	broadcast(NULL, packet.tostring());   ///---�㲥������˳���

	xt_log.info("handler logout uid[%d] seatid[%d] money[%d] tid[%d].\n", player->uid, player->seatid, player->money, tid);

	return 0;
}

///---������
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
	broadcast(NULL, packet.tostring());    ///---�㲥��������Ϣ��

	return 0;
}

///������
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


///�����˶�ʱ���ص�
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
	ret = player->update_info();    ///---���������Ϣ
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
	broadcast(NULL, packet.tostring());   ///---�㲥�����Ϣ

	xt_log.info("handler info uid[%d] seatid[%d] money[%d] tid[%d].\n", player->uid, player->seatid, player->money, tid);

	return 0;
}

///---����������
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



