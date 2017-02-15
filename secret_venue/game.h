#ifndef _GAME_H_
#define _GAME_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ev++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "normal_table.h"
#include "grab_table.h"

class Client;
class Player;


//��λ
struct TableConfig
{
	int         status;          // ����״̬
	int		    tid ;            // ����ID��Ҳ�Ƿ���ID��
	int         vid ;            // ����ID
	int         create_uid;      // ������uid
	std::string room_name  ;     // ��������
	std::string password ;  // ��������
	int         max_player; // �����������
	int         fee;             // ��ˮ
	int         min_money;       // ���Я��
	int	        max_money ;      // ���Я��
	int         base_money;      // ��ע
	int         game_rule;       // ��Ϸ�淨����
	int         lose_exp;        // ��ľ���ֵ
	int         win_exp;	     // Ӯ�ľ���ֵ
};


///---��Ϸ���(��װTable, Client, Player)
class Game
{
public:	
	int getTableInfo(int tableID, TableConfig& tblConfig);
	std::map<int, NormalTable*>     normal_tables;      // ���ׯ��
	std::map<int, GrabTable*>       grab_tables;        // ������ׯ

	std::map<int, Client*> 		fd_client;         // �����ն�
	std::map<int, Player*>      offline_players;   // �������
	std::map<int, Player*>      online_players;    // �������

	Client *robot_client;						   // �����˿ͻ���

private:
	ev_io _ev_accept;

	///����socket
	int _fd;

public:
	Game();
	virtual ~Game();	

	///����������
	int start();

	///�����û�����
	static void accept_cb(struct ev_loop *loop, struct ev_io *w, int revents);

	///�ַ���Ϣ
	int dispatch(Client *client);

	///У�������Ϣ
	int safe_check(Client *client, int cmd);

	///��¼����
	int handler_login_table(Client *client);

	///�ǳ�����
	int handle_logout_table(int tid);

	///���ʹ�����Ϣ��client
	int send_error(Client *client, int cmd, int error_code);

	///��Ȩ
	int check_skey(Client *client);

	///�������
	int add_player(Client *client);

	//ɾ��client
	void del_client(Client *client);

	///ɾ�����
	int del_player(Player *player);

	///����
	int change_table(Player *player);

	// ��鷿��
	int check_room(Client *client, TableConfig tblConfig);

private:
	//int     init_table();
	int     init_accept();

private:
	int checkPacket(Json::Value val);
};

#endif
