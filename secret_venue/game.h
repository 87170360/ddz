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


//坐位
struct TableConfig
{
	int         status;          // 房间状态
	int		    tid ;            // 桌子ID（也是房间ID）
	int         vid ;            // 场馆ID
	int         create_uid;      // 创建者uid
	std::string room_name  ;     // 房间名称
	std::string password ;  // 房间密码
	int         max_player; // 房间最多人数
	int         fee;             // 抽水
	int         min_money;       // 最低携带
	int	        max_money ;      // 最高携带
	int         base_money;      // 底注
	int         game_rule;       // 游戏玩法规则
	int         lose_exp;        // 输的经验值
	int         win_exp;	     // 赢的经验值
};


///---游戏相关(封装Table, Client, Player)
class Game
{
public:	
	int getTableInfo(int tableID, TableConfig& tblConfig);
	std::map<int, NormalTable*>     normal_tables;      // 随机庄家
	std::map<int, GrabTable*>       grab_tables;        // 看牌抢庄

	std::map<int, Client*> 		fd_client;         // 所有终端
	std::map<int, Player*>      offline_players;   // 离线玩家
	std::map<int, Player*>      online_players;    // 在线玩家

	Client *robot_client;						   // 机器人客户端

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

	// 检查房间
	int check_room(Client *client, TableConfig tblConfig);

private:
	//int     init_table();
	int     init_accept();

private:
	int checkPacket(Json::Value val);
};

#endif
