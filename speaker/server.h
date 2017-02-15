#ifndef _SERVER_H_
#define _SERVER_H_

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

#include <json/json.h>

#include "jpacket.h"

class Client;

class Server {
public:
	std::map<int, Client*>  fd_client;      // 所有连接的客户
	std::map<int, Client*>  uid_client;

	int speaker_money;

private:
	ev_io   _ev_accept;
	int     _fd;
public:
	Server();
	virtual ~Server();
	int start();
	static void accept_cb(struct ev_loop *loop, struct ev_io *w, int revents);
	void del_client(int fd);
	int dispatch(Client *client);
	int add_client(Client *client);
	int check_skey(Client *client);
	int send_client_login_succ(Client *client);
	int send_client_login_err(Client *client, int error_code);
	int client_chat(Client *client);
	int send_client_chat_succ(Client *client);
	int broadcast(Client *c, const std::string &packet);
	int unicast(Client *c, const std::string &packet);
	void publish(const std::string &pubdata);
	void subscribe(const std::string &subdata);
	void system_message_unicast(Jpacket &jpacket);

private:
	int init_accept();
	int check_string(const std::string &in_str);
};


#endif   /*_SERVER_H_*/
