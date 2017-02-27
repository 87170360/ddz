#ifndef _LOG_AGENT_H_
#define _LOG_AGENT_H_

#include <iostream>
#include <fstream>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

class LogAgent {
public:
    LogAgent();
    ~LogAgent();
	int init(string host, int port, string pass);
	void send(std::string &data);
	void deinit();
public:
	int fd;
    struct sockaddr_in addr;
	string passwd;
};

#endif
