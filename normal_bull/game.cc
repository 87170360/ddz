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

#include "bull.h"
#include "log.h"
#include "game.h"
#include "proto.h"
#include "client.h"
#include "player.h"
#include "table.h"

extern Bull gBull;
extern Log xt_log;

Game::Game()
{
	robot_client = NULL;
}

Game::~Game()
{
}

void dump_game_info(char *tag)
{
#if 0
	static char buf[102400];
	int i = 0;
	i += sprintf(buf, "begin===============%s===============begin\n", tag);
	std::map<int, Table*>::iterator table_it;
	std::map<int, Client*>::iterator client_it;
	std::map<int, Player*>::iterator player_it;

	i += sprintf(buf + i, "[seven_tables][%lu]\n", gBull.game->seven_tables.size());
	for (table_it = gBull.game->seven_tables.begin(); table_it != gBull.game->seven_tables.end(); table_it++)
	{
		Table *table = table_it->second;
		i += sprintf(buf + i, "Tid[%d]state[%d] ", table_it->first, table->state);
		for (player_it = table->playersMap.begin(); player_it != table->playersMap.end(); player_it++)
		{
			Player *player = player_it->second;
			if (player->client)
				i += sprintf(buf + i, "uid[%d]fd[%d] ", player->uid, player->client->fd);
			else
				i += sprintf(buf + i, "uid[%d] ", player->uid);
		}
		i += sprintf(buf + i, "\n");
	}

	i += sprintf(buf + i, "[six_tables][%lu]\n", gBull.game->six_tables.size());
	for (table_it = gBull.game->six_tables.begin(); table_it != gBull.game->six_tables.end(); table_it++)
	{
		Table *table = table_it->second;
		i += sprintf(buf + i, "Tid[%d]state[%d] ", table_it->first, table->state);
		for (player_it = table->playersMap.begin(); player_it != table->playersMap.end(); player_it++)
		{
			Player *player = player_it->second;
			if (player->client)
				i += sprintf(buf + i, "uid[%d]fd[%d] ", player->uid, player->client->fd);
			else
				i += sprintf(buf + i, "uid[%d] ", player->uid);
		}
		i += sprintf(buf + i, "\n");
	}

	i += sprintf(buf + i, "[five_tables][%lu]\n", gBull.game->five_tables.size());
	for (table_it = gBull.game->five_tables.begin(); table_it != gBull.game->five_tables.end(); table_it++)
	{
		Table *table = table_it->second;
		i += sprintf(buf + i, "Tid[%d]state[%d] ", table_it->first, table->state);
		for (player_it = table->playersMap.begin(); player_it != table->playersMap.end(); player_it++)
		{
			Player *player = player_it->second;
			if (player->client)
				i += sprintf(buf + i, "uid[%d]fd[%d] ", player->uid, player->client->fd);
			else
				i += sprintf(buf + i, "uid[%d] ", player->uid);
		}
		i += sprintf(buf + i, "\n");
	}

	i += sprintf(buf + i, "[four_tables][%lu]\n", gBull.game->four_tables.size());
	for (table_it = gBull.game->four_tables.begin(); table_it != gBull.game->four_tables.end(); table_it++)
	{
		Table *table = table_it->second;
		i += sprintf(buf + i, "Tid[%d]state[%d] ", table_it->first, table->state);
		for (player_it = table->playersMap.begin(); player_it != table->playersMap.end(); player_it++)
		{
			Player *player = player_it->second;
			if (player->client)
				i += sprintf(buf + i, "uid[%d]fd[%d] ", player->uid, player->client->fd);
			else
				i += sprintf(buf + i, "uid[%d] ", player->uid);
		}
		i += sprintf(buf + i, "\n");
	}

	i += sprintf(buf + i, "[three_tables][%lu]\n", gBull.game->three_tables.size());
	for (table_it = gBull.game->three_tables.begin(); table_it != gBull.game->three_tables.end(); table_it++)
	{
		Table *table = table_it->second;
		i += sprintf(buf + i, "Tid[%d]state[%d] ", table_it->first, table->state);
		for (player_it = table->playersMap.begin(); player_it != table->playersMap.end(); player_it++)
		{
			Player *player = player_it->second;
			if (player->client)
				i += sprintf(buf + i, "uid[%d]fd[%d] ", player->uid, player->client->fd);
			else
				i += sprintf(buf + i, "uid[%d] ", player->uid);
		}
		i += sprintf(buf + i, "\n");
	}

	i += sprintf(buf + i, "[two_tables][%lu]\n", gBull.game->two_tables.size());
	for (table_it = gBull.game->two_tables.begin(); table_it != gBull.game->two_tables.end(); table_it++)
	{
		Table *table = table_it->second;
		i += sprintf(buf + i, "Tid[%d]state[%d] ", table_it->first, table->state);
		for (player_it = table->playersMap.begin(); player_it != table->playersMap.end(); player_it++)
		{
			Player *player = player_it->second;
			if (player->client)
				i += sprintf(buf + i, "uid[%d]fd[%d] ", player->uid, player->client->fd);
			else
				i += sprintf(buf + i, "uid[%d] ", player->uid);
		}
		i += sprintf(buf + i, "\n");
	}

	i += sprintf(buf + i, "[one_tables][%lu]\n", gBull.game->one_tables.size());
	for (table_it = gBull.game->one_tables.begin(); table_it != gBull.game->one_tables.end(); table_it++)
	{
		Table *table = table_it->second;
		i += sprintf(buf + i, "Tid[%d]state[%d] ", table_it->first, table->state);
		for (player_it = table->playersMap.begin(); player_it != table->playersMap.end(); player_it++)
		{
			Player *player = player_it->second;
			if (player->client)
				i += sprintf(buf + i, "uid[%d]fd[%d] ", player->uid, player->client->fd);
			else
				i += sprintf(buf + i, "uid[%d] ", player->uid);
		}
		i += sprintf(buf + i, "\n");
	}

	i += sprintf(buf + i, "[fd_client][%lu]\n", gBull.game->fd_client.size());
	for (client_it = gBull.game->fd_client.begin(); client_it != gBull.game->fd_client.end(); client_it++)
	{
		i += sprintf(buf + i, "fd[%d] ", client_it->first);
	}
	i += sprintf(buf + i, "\n");

	i += sprintf(buf + i, "[offline_players][%lu]\n", gBull.game->offline_players.size());
	for (player_it = gBull.game->offline_players.begin(); player_it != gBull.game->offline_players.end(); player_it++)
	{
		i += sprintf(buf + i, "uid[%d] ", player_it->first);
	}
	i += sprintf(buf + i, "\n");

	i += sprintf(buf + i, "[online_players][%lu]\n", gBull.game->online_players.size());
	for (player_it = gBull.game->online_players.begin(); player_it != gBull.game->online_players.end(); player_it++)
	{
		i += sprintf(buf + i, "uid[%d] ", player_it->first);
	}
	i += sprintf(buf + i, "\n");
	i += sprintf(buf + i, "end===============%s===============end\n", tag);
	xt_log.debug("\n%s", buf);
#endif
}

int Game::start()
{
	/* first init table */
	init_table();

	init_accept();

	return 0;
}


/* init table from config
 * todo read config from db or other */

int Game::init_table()
{
	int vid = gBull.conf["tables"]["vid"].asInt();   //场馆id
	int zid = gBull.conf["tables"]["zid"].asInt();

	int min_money = gBull.conf["tables"]["min_money"].asInt();   //上限
	int max_money = gBull.conf["tables"]["max_money"].asInt();   //下限 

	int base_money = gBull.conf["tables"]["base_money"].asInt(); //底注
	int fee = gBull.conf["tables"]["fee"].asInt();               //每局的抽水

	int lose_exp = gBull.conf["tables"]["lose_exp"].asInt();   //经验值
	int win_exp = gBull.conf["tables"]["win_exp"].asInt();     //经验值

	xt_log.info("tables vid[%d] zid[%d] fee[%d] min_money[%d] max_money[%d] base_money[%d] lose_exp[%d] win_exp[%d] \n",
			vid, zid, fee, min_money, max_money, base_money,lose_exp, win_exp);

	///创建桌子[begin, end)
	for (int i = gBull.conf["tables"]["begin"].asInt(); i < gBull.conf["tables"]["end"].asInt(); i++)
	{
		Table *table = new Table();
		if (table->init(i, vid, /*zid, type,*/ min_money, max_money, base_money, fee, lose_exp, win_exp) < 0) 
		{
			xt_log.error("init table tid[%d] init err.\n", i);
			exit(1);
		}
		zero_tables[i] = table;      ///---保存到zero_tables   all_tables
		all_tables[i]  = table;
	}
	xt_log.info("total tables[%d]\n", zero_tables.size());

	return 0;
}

///启动监听服务
int Game::init_accept()
{
	xt_log.info("Listening on %s:%d\n",
			gBull.conf["game"]["host"].asString().c_str(),
			gBull.conf["game"]["port"].asInt());

	struct sockaddr_in addr;

	_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (_fd < 0) 
	{
		xt_log.error("File[%s] Line[%d]: socket failed: %s\n", __FILE__, __LINE__, strerror(errno));
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(gBull.conf["game"]["port"].asInt());
	addr.sin_addr.s_addr = inet_addr(gBull.conf["game"]["host"].asString().c_str());
	if (addr.sin_addr.s_addr == INADDR_NONE) 
	{
		xt_log.error("game::init_accept Incorrect ip address!");
		close(_fd);
		_fd = -1;
		exit(1);
	}

	int on = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) 
	{
		xt_log.error("File[%s] Line[%d]: setsockopt failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(_fd);
		return -1;
	}

	if (bind(_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) 
	{
		xt_log.error("File[%s] Line[%d]: bind failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(_fd);
		return -1;
	}

	fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK);
	listen(_fd, 100);

	_ev_accept.data = this;
	ev_io_init(&_ev_accept, Game::accept_cb, _fd, EV_READ);   ///设置接受连接回调
	ev_io_start(gBull.loop, &_ev_accept);

	xt_log.info("listen ok\n");

	return 0;
}

///接受用户连接
void Game::accept_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	if (EV_ERROR & revents) 
	{
		xt_log.error("got invalid event\n");
		return;
	}

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int fd = accept(w->fd, (struct sockaddr *) &client_addr, &client_len);
	if (fd < 0) 
	{
		xt_log.error("accept error[%s]\n", strerror(errno));
		return;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

	Client *client = new (std::nothrow) Client(fd);
	Game *game = (Game*) (w->data);
	if (client) 
	{
		game->fd_client[fd] = client;        ///保存负责与客户端通讯
	} 
	else 
	{
		close(fd);
	}
}

//删除client
void Game::del_client(Client *client)
{	
	///没有找到此client
	if (fd_client.find(client->fd) == fd_client.end()) 
	{
		xt_log.error("del client free client err[miss].\n");
		return;
	}

	fd_client.erase(client->fd);    ///移除client

	// todo
	if (client->player) 
	{
		Player *player = client->player;

		//client为POSTTION_WAIT,直接删除
		if (client->position == POSITION_WAIT) 
		{
			// todo delete this status   
			if (offline_players.find(player->uid) != offline_players.end()) 
			{
				offline_players.erase(player->uid);
				xt_log.info("del client player uid[%d] offline.\n", player->uid);
			}

			if (online_players.find(player->uid) != online_players.end()) 
			{
				online_players.erase(player->uid);
				xt_log.info("del client player uid[%d] online.\n", player->uid);
			}

			//delete player;   //为什么在这里删除？不应该啊
		} 
		else if (client->position == POSITION_TABLE) 
		{			
			///如果是"POSITION_TABLE", 放入离线玩家列表，启动离线定时器进行删除
			if (online_players.find(player->uid) != online_players.end()) 
			{
				online_players.erase(player->uid);

				offline_players[player->uid] = client->player;
				player->start_offline_timer();   
				player->client = NULL;
				xt_log.debug("del client player uid[%d] online and add this uid to offline\n", player->uid);
			}

			client->player->client = NULL;
		}
	}

	///如果是机器人玩家
	if (client->is_robot_svr) 
	{   
		robot_client = NULL;
	}

	xt_log.info("del client fd[%d].\n", client->fd);
	delete client;
	client = NULL;  ///+++

	dump_game_info("del_client");
}

///处理客户端上行的指令
int Game::dispatch(Client *client)
{
	//打印下收到的消息
	xt_log.info("Func[%s] player msg=%s \n", __FUNCTION__, client->packet.val.toStyledString().c_str());

	client->cmd_type = 0;

	// 校验协议包
	int cmd = checkPacket(client->packet.val);

	if (cmd < 0) 
	{
		xt_log.error("the cmd format is error.\n");
		return -1;
	}

	///系统echo (心跳包)
	if (cmd == SYS_ECHO) {
		Jpacket packet;
		packet.val = client->packet.val;
		packet.end();
		return client->send(packet.tostring());
	}

	///在线玩家数量
	if (cmd == SYS_ONLINE) 
	{
		client->cmd_type = 1;
		Jpacket packet;
		packet.val["cmd"] = SYS_ONLINE;
		packet.val["online"] = (int) (online_players.size() + offline_players.size());  //在线人数
		packet.end();
		return client->send(packet.tostring());
	}

	// cfc add if    ///---为机器人
	if (cmd == CLIENT_ROBOT_REQ) 
	{
		robot_client = client;
		client->is_robot_svr = true;
		Jpacket isrobot;
		isrobot.val["cmd"] = SERVER_ROBOT_SER_UC;
		isrobot.val["str"] = "yes, you're robot.";
		isrobot.end();
		return robot_client->send(isrobot.tostring());
	}

	///玩家登陆
	if (cmd == CLIENT_LOGIN_REQ) 
	{
		if (client->player == NULL) 
		{
			//添加玩家
			int ret = add_player(client);

			if (ret == -1)    ///鉴权失败
			{
				return -1;
			} 
			else if (ret == 1)    /* rebind by offline */
			{
				return 0;
			}
		   	else if (ret == 2)   /* rebind by online */
			{
				return 0;
			}

			//进入桌子
			ret = handler_login_table(client);
			return ret;
		}

		xt_log.error("CLIENT_LOGIN_REQ player must be NULL.\n");
		return -1;
	}

	///校验玩家信息
	if (safe_check(client, cmd) < 0) {
		return -1;
	}

	Player *player = client->player;
	/* dispatch */  ///分发给玩家所在的Table去处理
	switch (cmd) {		
		case CLIENT_GROUP_CARD_REQ: //组牌
			all_tables[player->tid]->handler_group_card(player);		
			break;

		case CLIENT_CHAT_REQ:   // 聊天
			all_tables[player->tid]->handler_chat(player);
			break;

		case CLIENT_FACE_REQ:   // 发图片表情
			all_tables[player->tid]->handler_face(player);
			break;

		case CLIENT_INFO_REQ:   // 更新玩家信息
			all_tables[player->tid]->handler_info(player);
			break;

		case CLIENT_LOGOUT_REQ: // 玩家退出  
			xt_log.info("Func[%s] 玩家退出\n", __FUNCTION__);
			del_player(player);
			break;

		case CLIENT_CHANGE_REQ:   // 换桌
			change_table(player);
			break;

		case CLIENT_TABLE_INFO_REQ:   ///8. 获取桌子信息
			all_tables[player->tid]->send_table_info_uc(player);
			break;

		case CLIENT_EMOTION_REQ:     ///9. 发互动表情
			all_tables[player->tid]->handler_interaction_emotion(player);
			break;

		default:
			xt_log.error("invalid command[%d]\n", cmd);
			return -1;
	}

	// xt_log.debug("dispatch succ\n");
	return 0;
}

///校验玩家信息
int Game::safe_check(Client *client, int cmd)
{
	///不在online列表中
	if (online_players.find(client->uid) == online_players.end()) {
		xt_log.error("safe check uid[%d] must be online player.\n", client->uid);
		return -1;
	}

	Player *player = client->player;
	if (player == NULL) {
		xt_log.error("safe check client player is NULL.\n");
		return -1;
	}

	///没有进入桌子
	if (all_tables.find(player->tid) == all_tables.end())
	{
		xt_log.error("safe_check uid[%d] is not in tid[%d]\n", player->uid, player->tid);
		return -1;
	}

	return 0;
}

///登录桌子
int Game::handler_login_table(Client *client)
{
	Player *player = client->player;

	if (client->position == POSITION_TABLE) {
		xt_log.error("handler_login_table uid[%d] have been in table\n", player->uid);
		return -1;
	}

	int ret = 0;

	///uid > 10000?
	if(client->uid > 10000)
	{
		ret = login_table(client, four_tables, five_tables);
		if (ret == 0)
			return 0;
		else if (ret == -2)
			return -2;

		ret = login_table(client, three_tables, four_tables);
		if (ret == 0)
			return 0;
		else if (ret == -2)
			return -2;
	}
	else 
	{
		if(rand()%20==0)
		{
			ret = login_table(client, three_tables, four_tables);
			if (ret == 0)
				return 0;
			else if (ret == -2)
				return -2;
		}
	}

	ret = login_table(client, two_tables, three_tables);
	if (ret == 0)
		return 0;
	else if (ret == -2)
		return -2;


	ret = login_table(client, one_tables, two_tables);
	if (ret == 0)
		return 0;
	else if (ret == -2)
		return -2;

	ret = login_table(client, zero_tables, one_tables);
	if (ret == 0)
		return 0;
	else if (ret == -2)
		return -2;

	xt_log.error("handler login table no seat.\n");
	return -1;
}

int Game::login_table(Client *client, std::map<int, Table*> &a, std::map<int, Table*> &b)
{
	if (a.size() > 0)   ///要有桌子
	{
		Player *player = client->player;
		map<int, Table*>::iterator it;

		std::vector<Table*> ok_tables;
		for (it = a.begin(); it != a.end(); it++) 
		{
			Table *table = (*it).second;

			if (table->game_state == COMPARE/*END_GAME*/) 
			{
				continue;
			}

			if (player->tid == table->tid)   ///不登陆之前的桌子
			{
				continue;
			}

			if (table->playersMap.find(player->uid) != table->playersMap.end()) 
			{
				xt_log.error("login table uid[%d] is in tid[%d]\n", player->uid, table->tid);
				return -2;
			}

			ok_tables.push_back(table);

			if(ok_tables.size()>20)
			{
				break;
			}
		}


		if(ok_tables.size()==0)
		{
			return -1;
		}

		Table* t=ok_tables[rand()%ok_tables.size()];

		a.erase(t->tid);
		b[t->tid] = t;

		client->set_positon(POSITION_TABLE);
		t->handler_login(client->player);

		return 0;
	}

	return -1;
}

int Game::handle_logout_table(int tid)
{
	map<int, Table*>::iterator it;
	it = all_tables.find(tid);
	if (it == all_tables.end()) {
		return -1;
	}
	Table *table = (*it).second;

	it = five_tables.find(tid);
	if (it != five_tables.end()) {
		five_tables.erase(it);
		four_tables[tid] = table;
		return 0;
	}

	it = four_tables.find(tid);
	if (it != four_tables.end()) {
		four_tables.erase(it);
		three_tables[tid] = table;
		return 0;
	}

	it = three_tables.find(tid);
	if (it != three_tables.end()) {
		three_tables.erase(it);
		two_tables[tid] = table;
		return 0;
	}

	it = two_tables.find(tid);
	if (it != two_tables.end()) {
		two_tables.erase(it);
		one_tables[tid] = table;
		return 0;
	}

	it = one_tables.find(tid);
	if (it != one_tables.end()) {
		one_tables.erase(it);
		zero_tables[tid] = table;
		return 0;
	}

	return -1;
}

///发送错误消息到client
int Game::send_error(Client *client, int cmd, int error_code)
{
	Jpacket error;
	error.val["cmd"] = cmd;
	error.val["err"] = error_code;
	error.end();
	return client->send(error.tostring());
}

///鉴权
int Game::check_skey(Client *client)
{
    ///机器玩家不用鉴权
	if (client->uid < XT_ROBOT_UID_MAX)  
	{
		return 0;
	}

#if 1

	int i = client->uid % gBull.main_size;
	int ret = gBull.main_rc[i]->command("hget hu:%d token", client->uid);
	if (ret < 0) {
		xt_log.error("check skey error, because get player infomation error.\n");
		return -1;
	}

	///从redis库中读取密钥并比较是否相同
	xt_log.debug("skey [%s] [%s]\n", client->skey.c_str(), gBull.main_rc[i]->reply->str);

	if (gBull.main_rc[i]->reply->str 
		&& client->skey.compare(gBull.main_rc[i]->reply->str) != 0) 
	{
		xt_log.error("check skey error, client[%s] server[%s].\n", 
			client->skey.c_str(), gBull.main_rc[i]->reply->str);
		return -1;
	}

#endif	
	return 0;
}

//添加玩家
int Game::add_player(Client *client)
{
	///JSON包
	Json::Value &val = client->packet.tojson();

	int uid = val["uid"].asInt();            //玩家UID
	client->uid  = uid;
	client->skey = val["skey"].asString();   //会话密钥
	client->vid  = val["vid"].asInt();
	client->zid  = val["zid"].asInt();

	//鉴权
	if (check_skey(client) < 0) {  
		Jpacket packet;
		packet.val["cmd"] = SERVER_LOGIN_ERR_UC;
		packet.val["code"] = 505;
		packet.val["msg"] = "skey error";
		packet.end();
		client->send(packet.tostring());
		return -1;
	}

	/* rebind by online */  
	if (online_players.find(uid) != online_players.end()) {

		xt_log.debug("player[%d] rebind by online get info ok\n", uid);

		Player *player = online_players[uid];

		if (all_tables.find(player->tid) == all_tables.end()) {
			xt_log.error("add player rebind by online uid[%d] is not in tid[%d].\n", player->uid, player->tid);
			return -1;
		}

		Client *oldClient = player->client;
		player->set_client(client);          //更新player的通讯Client
		client->set_positon(POSITION_TABLE);

		all_tables[player->tid]->send_login_succ_uc(player);   ///登录成功应答
		all_tables[player->tid]->send_table_info_uc(player);  ///下行房间信息包

		fd_client.erase(oldClient->fd);
		delete oldClient;

		player->stop_offline_timer();

		dump_game_info("rebind by online");

		return 2;
	}

	/* rebind by offline */
	if (offline_players.find(uid) != offline_players.end())
	{
		xt_log.debug("player[%d] rebind by offline get info ok\n", uid);

		Player *player = offline_players[uid];

		if (all_tables.find(player->tid) == all_tables.end())
		{
			xt_log.error("rebind by offline uid[%d] is not in table[%d]\n", player->uid, player->tid);
			return -1;
		}

		offline_players.erase(uid);   ///清除离线
		online_players[uid] = player;

		player->set_client(client);
		client->set_positon(POSITION_TABLE);

		all_tables[player->tid]->send_login_succ_uc(player);  ///---登录成功的应答
		all_tables[player->tid]->send_table_info_uc(player);

		player->stop_offline_timer();

		dump_game_info("rebind by offline");
		return 1;
	}
	
	/* set player info (新进来的玩家)*/
	Player *player = new (std::nothrow) Player();
	if (player == NULL) {
		xt_log.error("new player err");
		return -1;
	}

	player->set_client(client);
	int ret = player->init();

	if (ret < 0) {
		return -1;
	}

	//保存到online玩家列表
	online_players[uid] = player;

	//缓存数据库，更新在线人数(哪个监听端口下，对应多少人)
	if(gBull.cache_rc->command("hset gameinfo %d %d",gBull.conf["game"]["port"].asInt(),fd_client.size())<0)
	{
		xt_log.error("cache redis error(set gameinfo");
	}

	xt_log.info("add player uid[%d] login success.\n", uid);
	return 0;
}


///删除玩家
int Game::del_player(Player *player)
{
	int ret = 0;

	// 1 此玩家有桌子
	if (all_tables.find(player->tid) != all_tables.end()) 
	{
		//登出游戏(若游戏中，则由系统托管，不能删除)
		ret = all_tables[player->tid]->handler_logout(player);
		if (ret < 0) 
		{
			xt_log.error("del player table handler logout\n");
			return -1;
		}

		//删除玩家
		ret = all_tables[player->tid]->del_player(player);
		if (ret < 0) 
		{
			xt_log.error("del player table del player\n");
			return -1;
		}

		///登出桌子
		ret = handle_logout_table(player->tid);
		if (ret < 0) 
		{
			xt_log.error("del player table handle logout table.\n");
			return -1;
		}
	}

	// 2 offline清理
	if (offline_players.find(player->uid) != offline_players.end()) 
	{
		offline_players.erase(player->uid);
		xt_log.info("del player uid[%d] offline.\n", player->uid);
	}

	// 3 online清理
	if (online_players.find(player->uid) != online_players.end()) 
	{
		online_players.erase(player->uid);
		xt_log.info("del player uid[%d] online.\n", player->uid);
	}

	//如果有client
	if (player->client) 
	{
		Client *client = player->client;
		client->position = POSITION_WAIT;
		Client::pre_destroy(client);
		client->player = NULL;
		delete player;
		dump_game_info("del_player");
		return 0;
	}

	xt_log.debug("del player[%p] uid[%d]\n", player, player->uid);
	delete player;

	///更新gameinfo
	if(gBull.cache_rc->command("hset gameinfo %d %d",
		gBull.conf["game"]["port"].asInt(),fd_client.size()))
	{
		xt_log.error("cache redis error(set gameinfo");
	}

	return 0;
}

///换桌
int Game::change_table(Player *player)
{
	xt_log.info("change table uid[%d] money[%d] tid[%d].\n", 
		player->uid, player->money, player->tid);

	int ret = 0;
	if (all_tables.find(player->tid) != all_tables.end()) 
	{
		player->logout_type = 2;   // logout type 2 is change table cfc add 20131220

		ret = all_tables[player->tid]->handler_logout(player);
		if (ret < 0) 
		{
			xt_log.error("change table handler logout error.\n");
			return -1;
		}

		ret = all_tables[player->tid]->del_player(player);
		if (ret < 0) 
		{
			xt_log.error("change table del player error.\n");
			return -1;
		}

		///登出桌子
		ret = handle_logout_table(player->tid);
		if (ret < 0) 
		{
			xt_log.error("change table handle logout table error.\n");
			return -1;
		}

		Client *client = player->client;
		client->position = POSITION_WAIT;
		dump_game_info("handler_login_table begin.");

		ret = handler_login_table(client);   ///换到哪个桌子？

		dump_game_info("handler_login_table end.");
	}
	return 0;
}

//
int Game::checkPacket(Json::Value val)
{
	if (!val["cmd"].isNumeric()) 
		return -2;

	int cmd = val["cmd"].asInt();  //命令字

	switch (cmd)
	{
	case CLIENT_LOGIN_REQ:
		{
			if (val["uid"].isNumeric() && val["skey"].isString())
			{
				break;
			}
			else
			{
				//xt_log.error("Jpacket::sefe_check(): uid or skey format error.\n");
				return -1;
			}
		}
	}

	return cmd;
}