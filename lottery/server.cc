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

#include "speaker.h"
#include "log.h"
#include "proto.h"
#include "server.h"
#include "client.h"
#include "lottery.h"

extern Speaker speaker;
extern Log xt_log;

Server::Server()
{
	fd_client.clear();
	uid_client.clear();
	lottery_money = speaker.conf["user"]["lottery_money"].asInt();
	xt_log.info("server lottery_money[%d].\n", lottery_money);
}

Server::~Server()
{
	delete lottery;
}

int Server::start()
{
	init_lottery();
	init_accept();

	return 0;
}
///初始化抽奖
int Server::init_lottery()
{
	xt_log.info("init lottery...\n");

	lottery = new (std::nothrow) Lottery();
	if (lottery == NULL) {
		xt_log.error("init lottery new lottery error.\n");
		return -1;
	}

	int ret = lottery->init(this);
	if (ret < 0) {
		xt_log.error("init lottery initialization error.\n");
		return -1;
	}
	lottery->start();

	return 0;
}
///---启动抽奖监听服务
int Server::init_accept()
{
	xt_log.info("Listening on %s:%d\n", speaker.conf["server"]["host"].asString().c_str(),
			speaker.conf["server"]["port"].asInt());

	struct sockaddr_in addr;

	_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (_fd < 0) {
		xt_log.error("File[%s] Line[%d]: socket failed: %s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(speaker.conf["server"]["port"].asInt());
	addr.sin_addr.s_addr = inet_addr(speaker.conf["server"]["host"].asString().c_str());
	if (addr.sin_addr.s_addr == INADDR_NONE) {
		xt_log.error("server::init_accept Incorrect ip address!\n");
		exit(1);
	}

	int on = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		xt_log.error("File[%s] Line[%d]: setsockopt failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(_fd);
		return -1;
	}

	if (bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		xt_log.error("File[%s] Line[%d]: bind failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(_fd);
		return -1;
	}

	fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK);

	listen(_fd, 100);

	_ev_accept.data = this;
	ev_io_init(&_ev_accept, Server::accept_cb, _fd, EV_READ);
	ev_io_start(speaker.loop, &_ev_accept);

	xt_log.debug("listen ok.\n");

	return 0;
}
///---接受连接
void Server::accept_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	if (EV_ERROR & revents) {
		xt_log.error("got invalid event.\n");
		return;
	}

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int fd = accept(w->fd, (struct sockaddr *)&client_addr, &client_len);
	if (fd < 0) {
		xt_log.error("accept error[%s].\n", strerror(errno));
		return;
	}

	// client
	Client *client = new (std::nothrow) Client(fd);
	Server *s = (Server*)w->data;
	if (client) {
		s->fd_client[fd] = client;
	} else {
		close(fd);
	}
}

void Server::del_client(int fd)
{
	if (fd_client.find(fd) == fd_client.end()) {
		xt_log.error("free client err[miss]\n");
		return;
	}

	Client *client = fd_client[fd];
	int _uid = client->uid;
	if (uid_client.find(_uid) != uid_client.end()) {
		uid_client.erase(_uid);
	}
	lottery->handler_del_action(_uid);
	/*
	 *  player
	 * */

	delete client;
	fd_client.erase(fd);
}

int Server::dispatch(Client *client)
{
	client->cmd_type = 0;
	int cmd = client->packet.sefe_check();
	if (cmd < 0) {
		xt_log.error("the cmd format is error.\n");
		return -1;
	}

	/* dispatch */
	xt_log.debug("cmd is: %d.\n", cmd);
	if (cmd == CLIENT_LOTTERY_LOGIN) {
		if (add_client(client) < 0) {
			send_client_login_err(client);
			return -1;
		}
		client->is_login = 1;
		return 0;
	}

	if (client->is_login == 0) {
		xt_log.error("client without login.\n");
		return -1;
	}

	switch (cmd) {
	case CLIENT_LOTTERY_ACTION:
		lottery_action(client);
		break;
	case CLIENT_LOTTERY_BETTING:
		lottery_betting(client);
		break;
	default:
		xt_log.error("invalid command[%d]\n", cmd);
		return -1;
	}

	xt_log.debug("dispatch succ.\n");
	return 0;
}

int Server::add_client(Client *client)
{
	Json::Value &val = client->packet.tojson();
	int uid = val["uid"].asInt();
	client->uid = uid;
	client->index = uid % speaker.main_size;

	if (client->init_client()) {
		xt_log.debug("client init failed.\n");
		return -1;
	}

	lottery->handler_login(client);
	uid_client[uid] = client;

	return 0;
}

int Server::send_client_login_err(Client *client)
{
    Jpacket login_err;
    login_err.val["cmd"] = SERVER_LOGIN_ERR;
    login_err.val["uid"] = client->uid;
    login_err.end();
    unicast(client, login_err.tostring());
    return 0;
}

int Server::broadcast(Client *c, const std::string &packet)
{
	Client *client;
	std::map<int, Client*>::iterator iter;
	iter = fd_client.begin();
	for (; iter != fd_client.end(); iter++) {
		client = iter->second;
		if (client == c || client == NULL) {
			continue;
		}
		client->send(packet);
	}

	return 0;
}

int Server::unicast(Client *c, const std::string &packet)
{
	if (c) {
		return c->send(packet);
	}

	return -1;
}

void Server::system_message_unicast(Jpacket &jpacket)
{
	xt_log.debug("system message unicast...\n");

	Json::Value &val = jpacket.tojson();
	int len = val["to_uid"].size();
	if (len <= 0) {
		xt_log.error("unicast to uid list len is 0.\n");
		return;
	}

#if 0
	int num = 0;
	std::map<int, Client*>::iterator iter;
	for (iter = fd_client.begin(); iter != fd_client.end(); iter++) {
		Client *client = iter->second;
		if (client == NULL)
			continue;
		for (int i = 0; i < len; i++) {
			int _uid = val["to_uid"][i].asInt();
			if (client->uid == _uid) {
				xt_log.info("system message unicast uid:%d.\n", _uid);
				unicast(client, jpacket.tostring());
				num++;
				break;
			}
		}
		if (num == len)
			break;
	}
#else
	for (int i = 0; i < len; i++) {
		int _uid = val["to_uid"][i].asInt();
		if (uid_client.find(_uid) != uid_client.end()) {
			Client *client = uid_client[_uid];
			if (client == NULL)
				continue;
			xt_log.info("system message unicast uid:%d.\n", _uid);
			unicast(client, jpacket.tostring());
		}
	}
#endif
}

void Server::lottery_action(Client *client)
{
	Json::Value &val = client->packet.tojson();
	int _action = val["action"].asInt();

	lottery->handler_action(client, _action);
}

void Server::lottery_betting(Client *client)
{
	client->hget_money();

	Json::Value& val = client->packet.tojson();

	int _card_type = val["card_type"].asInt();

	int bet_nu=val["bet_nu"].asInt();

	int need_money=lottery_money*bet_nu;


	int client_money=client->get_money();


	if(client_money<70000 && client_money<need_money)
	{
		Jpacket betting_err;
		betting_err.val["cmd"] = SERVER_BETTING_ERR;
		betting_err.val["money"] = client_money;
		betting_err.val["need_money"] = need_money;
		betting_err.end();
		unicast(client, betting_err.tostring());
		return;
	}


	int ret = lottery->handler_betting(client, _card_type,bet_nu);

	if (ret < 0) 
	{
		xt_log.error("lottery betting error.\n");
		Jpacket betting_err;
		betting_err.val["cmd"] = SERVER_BETTING_ERR;
		betting_err.val["lstate"] = 0;
		betting_err.end();
		unicast(client, betting_err.tostring());
		return;
	}

	client->incr_money(1, need_money);
	client->eventlog_update(-need_money);


	client->incr_achievement_count(0, bet_nu);

}








