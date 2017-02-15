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

class Client;
class Player;
class Table;

///---��Ϸ���(��װTable, Client, Player)
class Game
{
public:
	std::map<int, Table*>       seven_tables;     ///7��Ƭ�����ӣ�����û���õ���
	std::map<int, Table*>       six_tables;       ///6��Ƭ�����ӣ�����û���õ���
	std::map<int, Table*>       five_tables;      ///5��Ƭ������ 
	std::map<int, Table*>       four_tables;      ///4��Ƭ������ 
	std::map<int, Table*>       three_tables;     ///3��Ƭ������ 
	std::map<int, Table*>       two_tables;       ///2��Ƭ������  
	std::map<int, Table*>       one_tables;       ///1��Ƭ������ 
	std::map<int, Table*>       zero_tables;      ///0��Ƭ������
	std::map<int, Table*>       all_tables;       ///�������� 

    std::map<int, Client*> 		fd_client;         ///====�����ն�
	std::map<int, Player*>      offline_players;   ///====�������
	std::map<int, Player*>      online_players;    ///====�������

	Client *robot_client;						   ///====�����˿ͻ���

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
	int login_table(Client *client, std::map<int, Table*> &a, std::map<int, Table*> &b);
	
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

private:
	int     init_table();
    int     init_accept();

private:
	int checkPacket(Json::Value val);
};

#endif
