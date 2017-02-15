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
#include "charcoding.h"
#include "wordfilter.h"

extern Speaker speaker;
extern Log xt_log;

Server::Server()
{
	fd_client.clear();
	uid_client.clear();
	speaker_money = speaker.conf["user"]["speaker_money"].asInt();
	xt_log.info("server speaker_money[%d].\n", speaker_money);
}

Server::~Server()
{
}

int Server::start()
{
	init_accept();

	return 0;
}


//启动监听服务，接受客户端连接
int Server::init_accept()
{
	xt_log.debug("Listening on %s:%d\n", speaker.conf["server"]["host"].asString().c_str(),
			speaker.conf["server"]["port"].asInt());

	struct sockaddr_in addr;

	_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (_fd < 0) 
	{
		xt_log.error("File[%s] Line[%d]: socket failed: %s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(speaker.conf["server"]["port"].asInt());
	addr.sin_addr.s_addr = inet_addr(speaker.conf["server"]["host"].asString().c_str());
	
	if (addr.sin_addr.s_addr == INADDR_NONE) 
	{
		xt_log.error("server::init_accept Incorrect ip address!\n");
		exit(1);
	}

	int on = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) 
	{
		xt_log.error("File[%s] Line[%d]: setsockopt failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(_fd);
		return -1;
	}

	if (bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
	{
		xt_log.error("File[%s] Line[%d]: bind failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(_fd);
		return -1;
	}

	fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK);

	listen(_fd, 100);

	_ev_accept.data = this;
	ev_io_init(&_ev_accept, Server::accept_cb, _fd, EV_READ);  ///---连接请的回调
	ev_io_start(speaker.loop, &_ev_accept);

	xt_log.debug("listen ok.\n");

	return 0;
}

// 连接到来
void Server::accept_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	if (EV_ERROR & revents) 
	{
		xt_log.error("got invalid event.\n");
		return;
	}

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int fd = accept(w->fd, (struct sockaddr *)&client_addr, &client_len);
	if (fd < 0) 
	{
		xt_log.error("accept error[%s].\n", strerror(errno));
		return;
	}

	// client
	Client *client = new (std::nothrow) Client(fd);
	Server *s = (Server*)w->data;
	if (client) 
	{
		s->fd_client[fd] = client;
	} else 
	{
		close(fd);
	}
}

void Server::del_client(int fd)
{
	if (fd_client.find(fd) == fd_client.end()) 
	{
		xt_log.error("free client err[miss]\n");
		return;
	}

	Client *client = fd_client[fd];
	int _uid = client->uid;
	if (uid_client.find(_uid) != uid_client.end()) 
	{
		uid_client.erase(_uid);
	}

	/*
	 *  player
	 * */

	delete client;
	fd_client.erase(fd);
}

//分发消息处理
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

	if (cmd == CLIENT_LOGIN_REQUEST) 
	{
		xt_log.debug("client xt_log...\n");
		if (add_client(client) < 0) 
		{
			send_client_login_err(client, LOGIN_PROTOCOL_ERR);
			return -1;
		}
		client->is_login = 1;
		return 0;
	}

	if (client->is_login == 0) {
		xt_log.error("client without login.\n");
		return -1;
	}

	switch (cmd) 
	{
	case CLIENT_CHAT:          // 聊天
		client_chat(client);
		break;

	case CLIENT_UPDATE_INFO:    //更新玩家信息
		if (client->init_client()) 
		{
			xt_log.error("client update info failed.\n");
			return -1;
		}
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
	client->name = val["name"].asString();
	client->skey = val["skey"].asString();
	client->index = uid % speaker.main_size;

	// 鉴权
	if (check_skey(client) < 0) 
	{
		xt_log.debug("client login skey error.\n");
		return -1;
	}

	if (client->init_client()) 
	{
		xt_log.debug("client init failed.\n");
		return -1;
	}

	//发送登录成功包
	send_client_login_succ(client);

	uid_client[uid] = client;   // 保存到uid_client map中

	return 0;
}

int Server::check_skey(Client *client)
{
	if (client->uid < 10000) {
		return 0;
	}

#if 1
	int i = client->uid % speaker.main_size;
	int ret = speaker.main_rc[i]->command(" hget u:%d token", client->uid);
	if (ret < 0) 
	{
		xt_log.error("player init error, because get player infomation error.\n");
		return -1;
	}

	xt_log.debug("skey [%s] [%s]\n", client->skey.c_str(), speaker.main_rc[i]->reply->str);
	if (speaker.main_rc[i]->reply->str 
		&& client->skey.compare(speaker.main_rc[i]->reply->str) != 0)
	{
		xt_log.error("skey [%s] [%s]\n", client->skey.c_str(), speaker.main_rc[i]->reply->str);
		return -1;
	}
#endif
	return 0;
}

int Server::send_client_login_succ(Client *client)
{
	Jpacket login_succ;
	login_succ.val["cmd"] = SERVER_LOGIN_SUCC;
	login_succ.val["uid"] = client->uid;
	login_succ.end();

	unicast(client, login_succ.tostring());

	return SERVER_LOGIN_SUCC;
}

int Server::send_client_login_err(Client *client, int error_code)
{
    Jpacket login_err;
    login_err.val["cmd"] = SERVER_LOGIN_ERR;
    login_err.val["err"] = error_code;
    login_err.end();
//    return client->send(login_err.tostring());
    unicast(client, login_err.tostring());
    return 0;
}

int Server::client_chat(Client *client)
{
	// 更新下当前用户金币
	client->hget_money();

	if (client->money < speaker_money) 
	{
		Jpacket chat_err;
		chat_err.val["cmd"] = CHAT_LACK_MONEY;
		chat_err.val["uid"] = client->uid;
		chat_err.val["speaker_money"] = speaker_money;
		chat_err.end();
		unicast(client, chat_err.tostring());
		xt_log.error("client chat money too less uid[%d] money[%d].\n", client->uid, client->money);
		return -1;
	}

	Json::Value &val = client->packet.tojson();

	// cfc add down 6 line 20140320
	std::string in_str = val["str"].toStyledString();
	int ret = check_string(in_str);

	if (ret < 0) 
	{
		xt_log.error("client chat check string error.\n");
		Jpacket chat_err;
		chat_err.val["cmd"] = SERVER_CHAT_ERR;
		chat_err.val["uid"] = client->uid;
		chat_err.end();
		unicast(client, chat_err.tostring());
		return -1;
	}

	Jpacket packet;
	packet.val["cmd"] = SERVER_CHAT;
	packet.val["uid"] = client->uid;
	packet.val["name"] = client->name;
	packet.val["sex"] = client->sex;
	packet.val["vlevel"] = client->vlevel;
	packet.val["str"] = val["str"];
	packet.val["tag"] = val["tag"];
	packet.end();
	broadcast(NULL, packet.tostring());

	// 扣钱
	client->incr_money(1, speaker_money);
	send_client_chat_succ(client);

	// write event xt_log
	client->eventlog_update(speaker_money);
	client->incr_achievement_count(CLIENT_CHAT, 1);

	xt_log.info("client chat succ uid[%d] str:[%d].\n", client->uid, in_str.c_str());

	return 0;
}

int Server::send_client_chat_succ(Client *client)
{
	Jpacket packet;
	packet.val["cmd"] = SERVER_CHAT_SUCC;
	packet.val["uid"] = client->uid;
	packet.val["money"] = client->money;
	packet.end();
	unicast(client, packet.tostring());
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


// 发布消息
void Server::publish(const std::string &pubdata)
{
//	speaker.pub_rc[0]->command("PUBLISH zjh_speaker %s", pubdata.c_str());
	for (int i = 0; i < speaker.pub_size; i++) {
		speaker.pub_rc[i]->command("PUBLISH zjh_speaker %s", pubdata.c_str());
	}
}

// 订阅
void Server::subscribe(const std::string &subdata)
{
	std::string sub = subdata;
	Jpacket packet;
	if (packet.noxor_parse(sub) < 0) 
	{
		xt_log.error("subscribe parse error.\n");
		return;
	}
	packet.end();

	int _cmd = packet.val["cmd"].asInt();
	xt_log.info("subscribe cmd[%d] subdata[%d].\n", _cmd, subdata.c_str());

	if (_cmd == SERVER_SYSTEM_UC) {
		system_message_unicast(packet);
		return;
	} else if (_cmd == SERVER_SYSTEM_ACHI) {
		system_message_unicast(packet);
		return;
	} else if (_cmd == SERVER_SYSTEM_UC_CHAT) {
		system_message_unicast(packet);
		return;
	}

	broadcast(NULL, packet.tostring());
}

void Server::system_message_unicast(Jpacket &jpacket)
{
	xt_log.debug("system message unicast...\n");

	Json::Value &val = jpacket.tojson();
	int len = val["to_uid"].size();     // 发送uid数组

	if (len <= 0) 
	{
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
	for (int i = 0; i < len; i++) 
	{
		int _uid = val["to_uid"][i].asInt();
		if (uid_client.find(_uid) != uid_client.end()) 
		{
			Client *client = uid_client[_uid];
			if (client == NULL)
			{
				xt_log.info("system message unicast uid:%d failed,client error \n",_uid);
				continue;
			}
			xt_log.info("system message unicast uid:%d.\n", _uid);
			unicast(client, jpacket.tostring());
		}
		else 
		{
			xt_log.info("system message unicast uid:%d failed,user not online\n",_uid);
		}
	}
#endif
}

// 转码和过滤关键字 
int Server::check_string(const std::string &in_str)
{
	return 0;   //没有校验？？？
	char *buf = NULL;
	int len = in_str.size();
	int l = 0;
	int r = cc_convert((char *) in_str.c_str(), len, &buf, &l, "utf8", "gbk");

	if (r) 
	{
		xt_log.error("check_string encode error str[%s].\n", in_str.c_str());
		return -1;
	}

	free(buf);

	if (keyword_filter(in_str.c_str())) {
		xt_log.error("check_string keyword filter error str[%s].\n", in_str.c_str());
		return -1;
	}

	return 0;
}
