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


///���ѡׯ�淨��ƥ�����ˮ�����ѡׯ����5���ơ����ơ���ׯ�ұ��ơ����㣩
Table::Table() :
  ready_timer_stamp(READY_TIME),     // 1׼��ʱ�䣨ƥ���ˣ�
  banker_timer_stamp(BANKER_TIME),   // 2���ѡׯʱ��
  send_card_timer_stamp(GROUP_TIME), // 3����(�û���������)
  compare_timer_stamp(COMPARE_TIME), // 4����ʱ�䣨����Ϸ���������ƽ��㣩
  robot_timer_stamp(6)               // ������ʱ��
{
 	///1׼�������ʱ���������ҽ��������뿪��
    ready_timer.data = this;
    ev_timer_init(&ready_timer, Table::ready_timer_cb, ready_timer_stamp, ready_timer_stamp);

	///2���ׯ��
	banker_timer.data = this;
	ev_timer_init(&banker_timer, Table::banker_timer_cb, banker_timer_stamp, banker_timer_stamp);

	///3����
	send_card_timer.data = this;
	ev_timer_init(&send_card_timer, Table::send_card_timer_cb, send_card_timer_stamp, send_card_timer_stamp);

	///4����
    compare_timer.data = this;
    ev_timer_init(&compare_timer, Table::compare_timer_cb, compare_timer_stamp, compare_timer_stamp);


    robot_timer.data = this;
    ev_timer_init(&robot_timer, Table::robot_timer_cb, robot_timer_stamp, robot_timer_stamp);


	//int weight[6]={52*1.5,48*1.5,1096*1.5,720*1.5,3744*1.5,16440/2}; ///����Ȩ��ֵ
	//deck.setTypeWeight(weight);     ///����ÿ�����͵�Ȩ��

	//��ţ�߼�
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

///���ӳ�ʼ��
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

	tid = my_tid;                ///����
    vid = my_vid;                ///���ݺţ��߼����м������ܵȣ�

    min_money = my_min_money;    ///Я���������
	max_money = my_max_money;    ///Я���������
	base_money = my_base_money;  ///��ע
 
	fee = my_fee;                ///��ˮ����

	lose_exp = my_lose_exp;      ///��ľ���ֵ
	win_exp = my_win_exp;        ///Ӯ�ľ���ֵ

	
    cur_players = 0;             ///��ǰ��Ҹ���
	dealer = -1;                 ///��ׯ��
	playersMap.clear();          ///������map
	ready_players = 0;           ///��׼���õ������
	game_state = WAIT;           ///��ʼ���ȴ���          

	//������ӣ�������λid
    for (int i = 0; i < MAX_CHAIR_COUNT; i++)
    {
		seats[i].clearSeat();
        seats[i].seatid = i;   
    }

    return 0;
}



/**
   ��ҵ�¼����(��5�˾Ͳ����ٽ���)
**/
int Table::handler_login(Player *player)
{
	if (playersMap.find(player->uid) == playersMap.end()) {

		//�������
		if (sit_down(player) < 0) {
			return -1;
		}

		//���е�¼�����Ϣ
		send_login_succ_uc(player);
		send_login_succ_bc(player);
		send_table_info_uc(player);

		xt_log.info("Func[%s] Login Table Succeed, uid[%d] money[%d] cur_players[%d] tid[%d] game_state[%d].\n", 
			        __FUNCTION__, player->uid, player->money, cur_players, tid, game_state);

		//��ǰ״̬WAIT״̬���������>=2�ˣ���Ϸ״̬Ϊ��׼����
		if((WAIT == game_state) && (cur_players >= 2))   
		{
			xt_log.info("Func[%s] *** ��Ϸ׼�� READY.\n", __FUNCTION__);

			//��Ϸ״̬Ϊ"׼��"
			game_state = READY;

			//�㲥��Ϸ׼��
			send_game_ready_bc();

			start_ready_time = time(0);   //׼����ʼʱ��	

			//������׼������ʱ��
			ev_timer_again(gBull.loop, &ready_timer);   
		}
		return 0;
	}
	return -1;
}

///�㲥��Ϸ׼��
int Table::send_game_ready_bc()
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_GAME_READY_BC;
	packet.val["state"] = READY;
	packet.end();

	broadcast(NULL, packet.tostring());
	return 0;

}


/**����,���������λ
   param: playrer ���
   return: ��λ��
*/
int Table::sit_down(Player *player)
{
	//����λ�б�
	std::vector<int> tmp;     
    for (int i = 0; i < MAX_CHAIR_COUNT; i++)
    {
		if (seats[i].status == STATUS_NULL)  ///��λ
		{
			tmp.push_back(i);
		}
    }
	
	//�п�λ�����ѡ���λ
	int len = tmp.size();
	if (len > 0)    
	{
		int index = random(0, len - 1);
		int seat_id = tmp[index];

		xt_log.debug("Func[%s] select seatid[%d].\n", __FUNCTION__, seat_id);

		seats[seat_id].clearSeat();

		 //������λ�ź����		
		seats[seat_id].uid    = player->uid;
		seats[seat_id].player = player;  
		seats[seat_id].status = STATUS_LOOK_ON;   //�Թ���
		seats[seat_id].role   = ROLE_NULL;        //�м�
				
		player->seatid = seat_id;   //�����λID
		player->tid    = tid;       //�������ID

		//����������(һ��Ҫ��λ��)
		playersMap[player->uid] = player;

		cur_players++;      //�ۼƵ�ǰ��Ҹ���

		return seat_id;
	}

	return -1;
}



///���е�¼�ɹ�Ӧ��
int Table::send_login_succ_uc(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGIN_SUCC_UC;
	packet.val["vid"] = player->vid;
	packet.val["tid"] = player->tid;

	//���ͻ���Ԥ�ڵĵ���ʱ
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

///���е�¼�ɹ��㲥�����ҡ�����Ϣ�㲥��ȥ��
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

	//++ ���Ӯȡ������õ���
	packet.val["max_win_money"]     = player->max_win_money;
	packet.val["best_board"]        = player->best_board;
	packet.val["best_board_detail"] = player->best_board_detail;

	packet.end();
	broadcast(player, packet.tostring());

	xt_log.debug("Func[%s] msg=%s.\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

	return 0;
}

//���з�������
int Table::send_table_info_uc(Player *player)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_TABLE_INFO_UC;
	packet.val["vid"] = player->vid;        ///����id
	packet.val["tid"] = player->tid;        ///���ӵ�id   
	packet.val["seatid"] = player->seatid;  ///λ��id
	packet.val["state"] = game_state;       ///״̬

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

	packet.val["dealer"] = dealer;          ///ׯ��id
	packet.val["base_money"] = base_money;  ///��ע
	packet.val["fee"] = fee;                 ///˰

	std::map<int, Player*>::iterator it;    ///---����б�
	int i = 0;

	int bankerid = -1;

	for (it = playersMap.begin(); it != playersMap.end(); it++) 
	{
		Player *p = it->second;
		Seat &seat = seats[p->seatid];

		packet.val["players"][i]["uid"]     = p->uid;       ///���id
		packet.val["players"][i]["seatid"]  = p->seatid;    ///λ��id
		packet.val["players"][i]["role"]    = seat.role;    ///�Ƿ���ׯ��
		if (seat.role > 0)
		{
			bankerid = seat.role;
		}        
		packet.val["players"][i]["status"]  = seat.status;  ///��ǰ״̬

		if (player == p) {
			///�������(���ֶ�ֻ�����Լ�ʱ������)s
			for (unsigned int j = 0; j < 5; j++) 
			{
				packet.val["players"][i]["holes"].append((int)seat.m_cbUserHandCard[j]);
			}
			packet.val["players"][i]["card_type"]=(int)seat.m_cardType;			
		}

		packet.val["players"][i]["group_card"] = seat.bPlayerChoiced ? 1 : 0;   //����Ƿ����Ʊ��
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
		//++���Ӯȡ�������
		packet.val["players"][i]["max_win_money"]  = p->max_win_money;  //���Ӯȡ 
		packet.val["players"][i]["best_board"]        = p->best_board ;     //���������
		packet.val["players"][i]["best_board_detail"] = p->best_board_detail;     //string#����������(0x01,0x02,0x03,0x04,0x05) 

		i++;
	}

	packet.val["dealer"] = bankerid;          ///ׯ��id(������)

	packet.end();
	unicast(player, packet.tostring());

	xt_log.debug("Func[%s] msg=%s.\n", 
		__FUNCTION__, packet.val.toStyledString().c_str());

	return 0;
}


//��׼����ʱ�����������ѡׯ
void Table::ready_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*)w->data;
	ev_timer_stop(gBull.loop, &table->ready_timer);

	//�������������ڵ������ˣ�����Ϸ��ʼ���������ѡׯ
	if (table->cur_players >= 2)   
	{		
		table->game_start();
	}
	else
	{
		//������������Ϸ�����ȴ�		
		xt_log.info("Func[%s] *** ������������Ϸ���� WAIT.\n", __FUNCTION__);
		table->game_state = WAIT; 

		//�㲥��Ϸ�ȴ�
		Jpacket packet;
		packet.val["cmd"]   = SERVER_GAME_WAIT_BC;
		packet.val["state"] = WAIT;
		packet.end();
		table->broadcast(NULL, packet.tostring());

		xt_log.debug("Func[%s] �㲥�ȴ�msg=%s.\n", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}
}


//�������״̬Ϊ����Ϸ�С� 
int Table::setPlayStatus(int bankSeatid)
{	
	bool hasBanker = false;  //����ׯ��
	std::map<int, Player*>::iterator it;    ///---����б�
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




//��Ϸ��ʼ���������ѡׯ��ִ�п�˰���㲥ѡׯ�����
int Table::game_start()
{	
	//�������״̬
	setPlayStatus(dealer);   ///��Ϸ��ʼ�����������Ϊ����Ϸ
	
	xt_log.debug("Func[%s] *** ��Ϸ��ʼ cur_players[%d] BankerSeatId[%d] .\n",
		__FUNCTION__, cur_players, dealer);

	//��˰(��ˮ)
	pay_tax();

	if (dealer < 0)
	{
		//ȷ��ׯ��
		make_banker();

		//�㲥ѡׯ���
		send_make_banker_bc();

		//��Ϸ״̬��ѡׯ��
		game_state = BANKER;
		ev_timer_again(gBull.loop, &banker_timer);
		start_banker_time = time(0);  //ѡׯ��ʼʱ��	
	} 
	else 
	{
		xt_log.debug("Func[%s] *** ��ׯ�ң�ֱ��SEND_CARD dealer[%d]\n", __FUNCTION__, dealer);
        game_state = SEND_CARD;
		//��ׯ�ң�ֱ�ӷ���
		send_card();
		///���뷢�ƻ���
		
		ev_timer_again(gBull.loop, &send_card_timer);
	}

	return 0;
}



//��˰����ˮ��
int Table::pay_tax()
{
	std::map<int, Player*>::iterator it;    ///---����б�
	for (it = playersMap.begin(); it != playersMap.end(); it++) 
	{
		Player *player = it->second;

		if (player != NULL)
		{
			Seat &seat = seats[player->seatid];
			if (seat.status == STATUS_IN_GAME )
			{			
				xt_log.debug("Func[%s] ��˰��ˮ seats[%d] uid[%d] fee[%d].\n", 
					__FUNCTION__, player->seatid, player->uid, fee);

				int value = -fee;
				player->incr_money(0, value);
				
			}
		}
	}	
	return 0;
}

///ȷ��ׯ��
int Table::make_banker()
{
	//��ֹ��ǰ��Ҹ���
	int playCount = 0;
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		if (seats[i].status >= STATUS_IN_GAME
			&& seats[i].player != NULL)
		{
			playCount++;
		}
	}

	//����������
	int index = random(0, playCount -1);

	int tmpCount = -1;    ///--��ʱ������

	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		if (seats[i].status >= STATUS_IN_GAME
			&& seats[i].player != NULL)
		{
			tmpCount++;

			if (tmpCount== index) 
			{
				xt_log.debug("Func[%s] ȷ��ׯ��uid[%d] BankerSeatId[%d].\n", 
					__FUNCTION__, seats[i].uid, i);

				//����ׯ�Һͽ�ɫ
				dealer = seats[i].seatid;    
				seats[i].role = ROLE_BANKER; 
				return 0;
			}

		}
	}

	xt_log.error("Func[%s] ѡׯʧ��.\n",__FUNCTION__);

	return -1;   //û��ѡ��ׯ��

}


///�㲥ѡׯ���
int Table::send_make_banker_bc()
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_MAKE_BANKER_BC;
	packet.val["state"]  = BANKER;	
	packet.val["dealer"] = dealer;  ///ׯ����λid

	std::map<int, Player*>::iterator it;    ///---����б�
	int i = 0;
	for (it = playersMap.begin(); it != playersMap.end(); it++) {
		Player *p = it->second;
		Seat &seat = seats[p->seatid];

		packet.val["players"][i]["uid"]     = p->uid;       ///���id
		packet.val["players"][i]["seatid"]  = p->seatid;    ///λ��id
		packet.val["players"][i]["role"]    = seat.role;    ///�Ƿ���ׯ��
		packet.val["players"][i]["status"]  = seat.status;  ///��ǰ״̬
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
		packet.val["players"][i]["max_win_money"]  = p->max_win_money;  //���Ӯȡ 
		packet.val["players"][i]["best_board"] = p->best_board ;     //���������
		packet.val["players"][i]["best_board_detail"] = p->best_board_detail;     //string#����������(0x01,0x02,0x03,0x04,0x05) 

		i++;
	}

	packet.end();
	broadcast(NULL, packet.tostring());

	//xt_log.debug("Func[%s] �㲥ѡׯmsg=%s .\n", 
	//	__FUNCTION__, packet.val.toStyledString().c_str());

	return 0;
} 



///"ѡׯ"ʱ����ϣ� ����
void Table::banker_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	xt_log.debug("Func[%s] *** ��Ϸ���� SEND_CARD.\n", __FUNCTION__);

	Table *table = (Table*) w->data;
	ev_timer_stop(gBull.loop, &table->banker_timer);


	///���뷢�ƻ���
	table->game_state = SEND_CARD;

	///���û�����
	table->send_card();

	ev_timer_again(gBull.loop, &table->send_card_timer);
}

///���ׯ��ʱ�����������
void Table::send_card()
{
	///��¼���ƿ�ʼʱ��
	start_group_time = time(0);

	//1�������
	_uint8 GoodCardBuff[GAME_PLAYER_COUNT][MAX_COUNT];  //5����         
	memset(GoodCardBuff, 0, sizeof(GoodCardBuff));
	
	_uint8 GoodChairId[GAME_PLAYER_COUNT] = { -1 }; //���Ƶ�����
	
	_uint8 bRandCard[CARDCOUNT] = { 0 };       //52����                  
	memset(bRandCard, 0, sizeof(bRandCard));

	int bGoodsCardSwtich = 0;   ///��д��

	//ϴ���߼�
	if (bGoodsCardSwtich == 1)
	{
		//ͳ��ÿ����ҵľ���
		int count[GAME_PLAYER_COUNT] = { -1 };
		bool is_robot[GAME_PLAYER_COUNT] = { false };
		for (int i = 0; i < GAME_PLAYER_COUNT; i++)
		{
			if (seats[i].status < STATUS_IN_GAME)
				continue;

			count[i] = seats[i].player->total_board; ///�ܵĶԾ���
			if (seats[i].player->uid < 10000)
			{
				is_robot[i] = true;
			}
		}

		//ϴ����
		m_GameLogic->GoodRandCardList(count, is_robot, bRandCard, CARDCOUNT, GAME_PLAYER_COUNT, GoodCardBuff, GoodChairId);
	}
	else
	{
		//���ϴ
		memcpy(bRandCard, m_cbCardData, CARDCOUNT);
		m_GameLogic->RandCardList(bRandCard, CARDCOUNT);
	}

	// 2Ϊÿ��������з���(���������Թۣ���û������)
	int len = 0;
	std::map<int, Player*>::iterator it;    ///---����б�
	for (it = playersMap.begin(); it != playersMap.end(); it++) 
	{
		Player *player = it->second;
		Seat &seat = seats[player->seatid];


		//������λ��(����û��)
		if (STATUS_NULL == seat.status 
			/*|| NULL == player*/)
		{
			continue;
		}

		if (seat.status >= STATUS_IN_GAME)
		{
			//����
			if (bGoodsCardSwtich == 1 /*&& GoodChairId[i] == i*/)
			{
				memcpy(seat.m_cbUserHandCard, GoodCardBuff[0], MAX_HAND_CARD_COUNT);
			}
			else
			{
				memcpy(seat.m_cbUserHandCard, bRandCard + len, MAX_HAND_CARD_COUNT);
				len += MAX_HAND_CARD_COUNT;
			}

			//����Ҳ����(���������������Ƶļ�¼)
			group_cards(seat);
		} 
		else 
		{
			//�Թ�������Ϊ��
			memset(seat.m_cbUserHandCard, 0, sizeof(seat.m_cbUserHandCard));
			seat.m_cardType = CT_NOBULL;
		}


		//���з��Ƹ��ͻ���
		Jpacket packet;
		packet.val["cmd"] = SERVER_SEND_CARD_UC;   
		packet.val["state"] = SEND_CARD;
		//������
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

///����
void Table::group_cards(Seat &seat)
{
	seat.m_cardType = (CT_BULL_TYPE)m_GameLogic->GetCardTypeAndData(
		seat.m_cbUserHandCard,  MAX_HAND_CARD_COUNT, seat.m_cbChoiceCard);

	//���������ʷ�����
	Player * player = seat.player;
	if (player != NULL)
	{
		char szCard[20] = {0}; //�ַ���
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




///"����"ʱ������� �������
void Table::send_card_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Table *table = (Table*) w->data;
	ev_timer_stop(gBull.loop, &table->send_card_timer);

	xt_log.debug("Func[%s] *** ��Ϸ���� COMPARE.\n", __FUNCTION__);

	//����
	table->compare_card();

	//��Ϸ�������
	table->game_state = COMPARE;
	ev_timer_again(gBull.loop, &table->compare_timer);	
	
	table->start_compare_time = time(0); //���ƿ�ʼʱ��

}


///������һׯ��(�м��õ�ţţ����Ҫ���ǻ�ׯ��)
int Table::next_banker()
{
	//�м���û�õ�ţţ�ģ�ȡǮ����ߣ�
	int tmp_money = 0;
	int bull_seat = -1;  //ţţ��λ��
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

	//�Ͼ�ׯ�һ���
	if (dealer >= 0 && dealer < MAX_CHAIR_COUNT 
		&& seats[dealer].status == STATUS_IN_GAME		
		&& seats[dealer].player != NULL)
	{
		///�м��õ�ţţ��ׯ��û�õ�ţţ����ׯ��
		if (bull_seat >= 0)
		{
			if (seats[dealer].m_cardType != CT_BULL_BULL)
			{
				xt_log.debug("Func[%s] ���м��õ�ţţ�ˣ�nextBanker[%d].\n", __FUNCTION__, bull_seat);	
				dealer = bull_seat;
			}
		}	
	}
	else
	{
		//ûׯ��
		dealer = bull_seat;
	}
		
	xt_log.debug("Func[%s], �����Ͼ�ׯ�ң���û�У������ѡ�� dealer[%d].\n", __FUNCTION__, dealer);
	return dealer;   //������ׯ��
}



//������ׯ������ʱ���м�ʱ�� - ׯ��ʱ�䣩
int Table::CalGroupTimeDiff()
{
	if (dealer >= 0 && dealer < MAX_CHAIR_COUNT)
	{	
		xt_log.debug("ׯ��[%d]����ʱ��[%d].\n", 
			seats[dealer].uid, seats[dealer].goup_time);

		for (int i = 0; i < MAX_CHAIR_COUNT; i++)
		{
			if (seats[i].status >= STATUS_IN_GAME
				&& i != dealer)
			{
				//ʱ���
				seats[i].group_time_diff =  seats[i].goup_time - seats[dealer].goup_time;

				xt_log.debug("�м�[%d]����ʱ��[%d] ��ׯ��ʱ���[%d].\n", 
					seats[i].uid, seats[i].goup_time, seats[i].group_time_diff);
			}
		}
		return 0;
	} else {
		xt_log.error("ׯ��λ��ID�Ƿ�seat[%d]\n", dealer);
		return -1;
	}
}




///����,����
int Table::compare_card()
{
	//����������ʱ��
	CalGroupTimeDiff();

	int GameGold[GAME_PLAYER_COUNT]      = { 0 };         //ÿ�������Ӯ�Ľ��          
	enEndKind EndKind[GAME_PLAYER_COUNT] = { KIND_NULL }; //��Ӯ����

	_tint32 BankerWinResult  = 0; //ׯ��Ӯ���
	_tint32 BankerLoseResult = 0; //ׯ������

	Player *pBankerUser = seats[dealer].player;
	if (NULL == pBankerUser)
	{
		xt_log.error("Func[%s] ���󣬱���ûׯ��player��,seat_id[%d]\n", __FUNCTION__, dealer);
		return -1;
	}

	///ׯ�����ͱ���
	int bankerCardTimes = m_GameLogic->GetTimes(seats[dealer].m_cbUserHandCard, MAX_HAND_CARD_COUNT);

	xt_log.info("Func[%s] ���ƽ���cur_players[%d]\n", __FUNCTION__, cur_players);
	//����ׯ����Ӯ��Ŀ
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		//û����Ϸ����
		if (seats[i].status < STATUS_IN_GAME 
			|| i == dealer
			|| seats[i].player == NULL)
		{
			continue;
		}


		//ׯ��Ӯ��
		if (m_GameLogic->CompareCard(
			seats[dealer].m_cbChoiceCard, 
			seats[i].m_cbChoiceCard, MAX_HAND_CARD_COUNT, 0, 0))
		{

			Player *player = seats[i].player; //�м�

			//ʱ��Ȩֵdouble floor( double arg );
			double timeWeight = 1 + (double)2*seats[i].group_time_diff/100;

			GameGold[i]  = - int(ceil(bankerCardTimes * base_money * timeWeight));  ///Ӯ�����ͱ���*��ע*ʱ��ӳ�

			xt_log.info("�м�uid[%d]��[%d]  bankerCardTimes=[%d] base_money[%d] timeWeight[%f].\n", 
				seats[i].uid, GameGold[i], bankerCardTimes, base_money, timeWeight);
			///�мҲ�����ʱ�����
			if (abs(GameGold[i]) > player->money)
			{
				GameGold[i] = -player->money;
				xt_log.info("������ GameGold[%d]=%d\n", i, GameGold[i]);
			}

			BankerWinResult = BankerWinResult - GameGold[i];   ///�ۼ�ׯ��Ӯ��
			EndKind[i] = KING_LOSE;    ///������						
		}
		else 
		{
			//�м�Ӯ��Ӯ�����ͱ���
			int otherCardTimes = m_GameLogic->GetTimes(seats[i].m_cbUserHandCard, MAX_HAND_CARD_COUNT);


			//ʱ��Ȩֵ
			double timeWeight = 1 - (double)(2*seats[i].group_time_diff)/100;

			GameGold[i]  = int (ceil(otherCardTimes * base_money * timeWeight));  ///Ӯ�����ͱ���*��ע*ʱ��ӳ�

			//GameGold[i]  = otherCardTimes * base_money *(1 + 2*seats[i].group_time_diff/100);  ///Ӯ�����ͱ���*��ע*ʱ��ӳ�

			BankerLoseResult += GameGold[i];  //ׯ������ܺ�
			EndKind[i] = KIND_WIN; //����Ӯ

			xt_log.info("�м�[%d]Ӯ[%d] otherCardTimes=[%d] base_money[%d] timeWeight[%f].\n", 
				seats[i].uid, GameGold[i], otherCardTimes, base_money, timeWeight);

		}

		//��������Ӯ����
		seats[i].end_kind = EndKind[i];
	}

	xt_log.debug("ׯ��Ӯ����[%d]\n", BankerWinResult);

	//����ׯ���䣬ׯ�Ҳ����䣬��������
	int bankerMoney =  pBankerUser->money + BankerWinResult;
	if (bankerMoney < BankerLoseResult)
	{
		xt_log.debug("ׯ�Ҳ�����.\n");
		int lostResult = BankerLoseResult;

		BankerLoseResult = 0;  ///������0
		for (int i = 0; i < MAX_CHAIR_COUNT; i++)
		{
			if (seats[i].status < STATUS_IN_GAME 
				|| i == dealer 
				|| seats[i].end_kind != KIND_WIN)
			{
				continue;
			}

			//�мҰ�����Ӯ��ô��
			int GoldResult = bankerMoney * (GameGold[i]/lostResult);
			GameGold[i] = GoldResult;

			if ((bankerMoney - BankerLoseResult - 1) <= GoldResult)  ///�����һ�飬�����һλ��ׯ�����
			{
				GameGold[i] = bankerMoney - BankerLoseResult;
				xt_log.debug("���һλ��BankerLoseResult[%d]\n", BankerLoseResult);
			} 

			BankerLoseResult += GameGold[i];   ///�ۼ�ׯ�����
		}
	} 

	xt_log.debug("ׯ��������[%d]\n", BankerLoseResult);

	//ׯ�������Ӯ�Ľ��
	int bankerResult = BankerWinResult - BankerLoseResult;
	GameGold[dealer] = bankerResult;
	seats[dealer].end_kind = (bankerResult > 0 ) ? KIND_WIN : KING_LOSE;  ///��¼ׯ�ҵ���Ӯ����

	xt_log.debug("ׯ�������Ӯ[%d]\n", bankerResult);

	//������ҽ��
	for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	{
		if (seats[i].status >= STATUS_IN_GAME)
		{
			Player *player = seats[i].player;
			if (player != NULL)
			{
				player->incr_money(0, GameGold[i]);        //���½��
				player->update_max_win_money(GameGold[i]); //�������Ӯȡ			
			}
		}
	}


	///�㲥���ƽ��
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
			packet.val["players"][k]["uid"]    = p->uid;      ///���id
			packet.val["players"][k]["seatid"] = seat.seatid; ///λ��id
			packet.val["players"][k]["role"]   = seat.role;   ///�Ƿ���ׯ��
			packet.val["players"][k]["status"] = seat.status; //״̬
			
			//��õ�����
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

			packet.val["players"][k]["win_money"]         = GameGold[i];       //������Ӯ��ң�����Ӯ������Ϊ�䣩
			packet.val["players"][k]["max_win_money"]     = p->max_win_money;  //���Ӯȡ 
			packet.val["players"][k]["best_board"]        = p->best_board ;     //���������
			packet.val["players"][k]["best_board_detail"] = p->best_board_detail;     //string#����������(0x01,0x02,0x03,0x04,0x05) 


			///[+++ Ϊ�˲��Զ�λ
			char szCard[64] = {0}; //�ַ���
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
   �û���������(У���ơ���¼����ʱ��)
*/
int Table::handler_group_card(Player *player)
{
	int errCode = 0;   //������

	if (game_state == SEND_CARD)
	{
		//�ж�����λ
		int seatid = player->seatid;

		if (seatid < 0 && seatid >= MAX_CHAIR_COUNT)
		{
			xt_log.error("Func[%s] seatid[%d]", __FUNCTION__, seatid);
			return -1;
		}

		seats[seatid].bPlayerChoiced = true;                  //������Ʊ��		
		seats[seatid].goup_time = time(0) - start_group_time; //��������ʱ��


		//��������
		Json::Value &val = player->client->packet.tojson();

		int cardCount = val["holes"].size();
		if (cardCount >= 3)
		{
			int i = 0;
			xt_log.debug("�û�uid[%d]����[%02x,%02x,%02x] ����ʱ��[%d]\n", 
				seats[seatid].uid,
				val["holes"][i++].asInt(),
				val["holes"][i++].asInt(),
				val["holes"][i++].asInt(),
				seats[seatid].goup_time);
		}

		//ȡ����
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
				xt_log.error("Func[%s] �Ƿ������� card[%d].\n", __FUNCTION__, card);
				errCode = -3;
				break;
			}
		}

		
		//����Ӧ��
		Jpacket packetUC;
		packetUC.val["cmd"] = SERVER_GROUP_CARD_UC;
		packetUC.val["code"] = errCode;
		packetUC.end();
		unicast(player, packetUC.tostring());

		//�㲥����
		Jpacket packet;
		packet.val["cmd"] = SERVER_GROUP_CARD_BC;
		packet.val["seatid"] = player->seatid;
		packet.val["uid"]    = player->uid;
		packet.end();
		broadcast(player, packet.tostring());
		
		//�����е��˶������ƣ������ϱ���
		if (isAllPlayerChoiced())
		{
			ev_timer_stop(gBull.loop, &send_card_timer);

			//����
			compare_card();

			//��Ϸ�������
			game_state = COMPARE;
			ev_timer_again(gBull.loop, &compare_timer);	

			start_compare_time = time(0); //���ƿ�ʼʱ��
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



///"����"ʱ�����, ��Ϸ�������ٴν���ȴ�
void Table::compare_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	//xt_log.debug("Func[%s] *** ��Ϸ�ȴ� WAIT.\n", __FUNCTION__);

	Table *table = (Table*) w->data;
	ev_timer_stop(gBull.loop, &table->compare_timer);

	//��Ϸ����,����ȴ�
	table->game_state = WAIT;

	//�峡Ԥ����
	table->pre_ready();
}


///��Ϸ�������峡����(���������Ƿ����������Ϸ��������)
int Table::pre_ready()
{	
	//�ۼ���ҿ��д���(û�����Ƶ����)
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

			if (player->money < min_money)    ///��ǰ���<����
			{
				if (player->uid < XT_ROBOT_UID_MAX)  ///��Ϊ������,�����Ǯ[200, 375]*min_money
				{
					player->set_money((int)(float(rand()%8+8)/4*100*min_money));
				}
				else 
				{
					temp[player->uid]=player;   ///��¼Ǯ���������
				}
			}

			//ϵͳ����20��(���������ߺ�Ҫ��)
			if (player->idle_count > 20)         ///��¼�޶�������� if (player->idle_count > 2)
			{
				temp[player->uid] = player;
				player->logout_type = 1;        
			}

			//�����˳����û�
			if (player->stand_up > 0)
			{
				temp[player->uid] = player;
				player->logout_type = 3;   //��������˳�
			}
		} 
		else 
		{
			xt_log.debug("Func[%s] handler wait uid[%d] is offline.\n", __FUNCTION__, player->uid);
			temp[player->uid] = player;     ///��¼�������
			player->logout_type = 0;
		}
	}

	///�����˳������
	//for (int i = 0; i < MAX_CHAIR_COUNT; i++)
	//{		
	//	if (STATUS_STAND_UP == seats[i].status 			
	//		&& seats[i].player != NULL)
	//	{
	//		temp[seats[i].player->uid] = seats[i].player;
	//		seats[i].player->logout_type = 3;  //��������˳�
	//	}
	//}

	//ɾ���������������
	for (it = temp.begin(); it != temp.end(); it++) 
	{
		Player *player = it->second;
		gBull.game->del_player(player);
	}


	//������һׯ��
	dealer = next_banker();

	/* reset seat*/
	for (int i = 0; i < MAX_CHAIR_COUNT; i++) {
		seats[i].resetSeat();
	}

    xt_log.info("Func[%s] *** game_state[%d] cur_players[%d].\n", __FUNCTION__, game_state, cur_players);
	/* check state */
	if (WAIT == game_state && cur_players >= 2)
	{
		xt_log.info("Func[%s] ***  ��Ϸ׼�� READY cur_players[%d].\n", __FUNCTION__, cur_players);
		game_state = READY;     
		ev_timer_again(gBull.loop, &ready_timer);
		send_game_ready_bc();
	} 
	else
	{
		game_state = WAIT;

		xt_log.info("Func[%s] *** ��Ϸ�ȴ� WAIT .\n", __FUNCTION__);
		Jpacket packet;
		packet.val["cmd"] = SERVER_GAME_WAIT_BC;
		packet.val["state"] = WAIT;
		packet.end();
		broadcast(NULL, packet.tostring());   ///�㲥�����ˣ���ϷԤ��
	}
	

	return 0;
}





///��Ϸ����
int Table::game_end()
{

	return 0;
}

///---����ǳ�
int Table::handler_logout(Player *player)
{	
	//������������ƣ���Ϸ���ڽ����У����ݲ����˳�
	Seat &seat = seats[player->seatid];

	if ((seat.status  >= STATUS_IN_GAME) 
		&& (game_state == BANKER
		|| game_state == SEND_CARD
		|| game_state == COMPARE) )
	{
		
		seat.status = STATUS_STAND_UP;
		player->stand_up = 1;  //�뿪����

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
	broadcast(NULL, packet.tostring());   ///---�㲥������˳���

	return 0;
}

///---������
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
	broadcast(NULL, packet.tostring());    ///---�㲥��������Ϣ��

	return 0;
}

///������
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


///�����˶�ʱ���ص�
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

///�ѵ��������Ϣ�㲥��ȥ
int Table::handler_info(Player *player)
{
	int ret = 0;
	ret = player->update_info();    ///���������Ϣ

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
	broadcast(NULL, packet.tostring());   ///�㲥�����Ϣ

	xt_log.info("handler info uid[%d] seatid[%d] money[%d] tid[%d].\n", player->uid, player->seatid, player->money, tid);

	return 0;
}




///---����������
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


///վ���������λ
void Table::stand_up(Player *player)
{
	seats[player->seatid].clearSeat();

	xt_log.debug("Func[%s] seatid[%d].\n", __FUNCTION__, player->seatid);
}

///ɾ�����
int Table::del_player(Player *player)
{
	xt_log.debug("Func[%s] uid[%d]..\n", __FUNCTION__, player->uid);

	//û���ҵ������,����
	if (playersMap.find(player->uid) == playersMap.end()) {		
		xt_log.error("del player talbe uid[%d] is error.\n", player->uid);
		return -1;
	}

    //ֹͣ"���߶�ʱ��"
	player->stop_offline_timer();    

	//��������
	playersMap.erase(player->uid);

	//���ׯλdealer
	if (dealer == player->uid)
	{
		dealer = -1;
	}

	//�����λ
	stand_up(player);

	cur_players--;   //���µ�ǰ�������

	return 0;
}



///��������ҹ㲥(�������p,�����p=NULL,�������е���)
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

///����
int Table::unicast(Player *p, const std::string &packet)
{
	if (p != NULL && p->client != NULL)
	{
		return p->client->send(packet);
	}
	return -1;
}

///[starr, end]֮��������
int Table::random(int start, int end)
{
	return start + rand() % (end - start + 1);
}




///vectorתJSON����
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
/////mapתJSON����
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
/////JSON����תvector
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


