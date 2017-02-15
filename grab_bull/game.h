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

///---游戏相关(封装Table, Client, Player)
class Game
{
public:
	std::map<int, Table*>       seven_tables;     ///7号片区桌子（好象没有用到）
	std::map<int, Table*>       six_tables;       ///6号片区桌子（好象没有用到）
	std::map<int, Table*>       five_tables;      ///5号片区桌子 
	std::map<int, Table*>       four_tables;      ///4号片区桌子 
	std::map<int, Table*>       three_tables;     ///3号片区桌子 
	std::map<int, Table*>       two_tables;       ///2号片区桌子  
	std::map<int, Table*>       one_tables;       ///1号片区桌子 
	std::map<int, Table*>       zero_tables;      ///0号片区桌子
	std::map<int, Table*>       all_tables;       ///所有桌子 

    std::map<int, Client*> 		fd_client;         ///====所有终端
	std::map<int, Player*>      offline_players;   ///====离线玩家
	std::map<int, Player*>      online_players;    ///====在线玩家

	Client *robot_client;						   ///====机器人客户端

private:
    ev_io _ev_accept;

	///监听socket
	int _fd;

public:
    Game();
    virtual ~Game();	

	///服务器启动
	int start();

	///接受用户连接
	static void accept_cb(struct ev_loop *loop, struct ev_io *w, int revents);

	///分发消息
    int dispatch(Client *client);

	///校验玩家信息
	int safe_check(Client *client, int cmd);

	///登录桌子
	int handler_login_table(Client *client);
	int login_table(Client *client, std::map<int, Table*> &a, std::map<int, Table*> &b);
	
	///登出桌子
	int handle_logout_table(int tid);

	///发送错误消息到client
	int send_error(Client *client, int cmd, int error_code);
	
	///鉴权
	int check_skey(Client *client);

	///增加玩家
    int add_player(Client *client);

	//删除client
	void del_client(Client *client);

	///删除玩家
    int del_player(Player *player);

	///换桌
    int change_table(Player *player);

private:
	int     init_table();
    int     init_accept();

private:
	int checkPacket(Json::Value val);
};

#endif
