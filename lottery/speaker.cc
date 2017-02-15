#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <ev.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "speaker.h"
#include "log.h"
#include "daemonize.h"
#include "server.h"

static int parse_arg(int argc, char **argv);
static int init_conf();
static void dump_conf();
static int set_rlimit(int n);
static int init_redis();

Speaker speaker;
Log xt_log;

int main(int argc, char *argv[])
{
	int ret = 0;

	ret = parse_arg(argc, argv);
	if (ret < 0) {
		xt_log.fatal("File: %s Func: %s Line: %d => parse_arg.\n", __FILE__, __FUNCTION__, __LINE__);
		exit(1);
	}

	ret = init_conf();
	if (ret < 0) {
		xt_log.fatal("File: %s Func: %s Line: %d => init_conf.\n", __FILE__, __FUNCTION__, __LINE__);
		exit(1);
	}
	dump_conf();

	if (speaker.is_daemonize == 1) {
		daemonize();
	}

	ret = single_instance_running(speaker.conf.get("pid_file", "conf/lottery.pid").asString().c_str());
	if (ret < 0) {
		xt_log.fatal("File: %s Func: %s Line: %d => single_instance_running.\n", __FILE__, __FUNCTION__, __LINE__);
		exit(1);
	}

	xt_log.start(speaker.conf["log"].get("log_file", "log/lottery.log").asString(),
			speaker.conf["log"].get("level", 4).asInt(),
			speaker.conf["log"].get("console", 0).asInt(),
			speaker.conf["log"].get("rotate", 1).asInt(),
			speaker.conf["log"].get("max_size", 1073741824).asInt(),
			speaker.conf["log"].get("max_file", 50).asInt());

	set_rlimit(10240);

	ret = init_redis();
	if (ret < 0) {
		xt_log.fatal("init redis failed.\n");
		exit(1);
	}

	struct ev_loop *loop = ev_default_loop(0);
	speaker.loop = loop;

	Server *server = new (std::nothrow) Server();
	speaker.server = server;
	speaker.server->start();

	ev_loop(loop, 0);

	return 0;
}

static int parse_arg(int argc, char **argv)
{
	int flag = 0;
	int oc;
	char ic;

	speaker.is_daemonize = 0;
	while((oc = getopt(argc, argv, "Df:")) != -1) {
		switch (oc) {
			case 'D':
				speaker.is_daemonize = 1;
				break;
			case 'f':
				flag = 1;
				speaker.conf_file = string(optarg);
				break;
			case '?':
				ic = (char)optopt;
				printf("invalid \'%c\'\n", ic);
				break;
			case ':':
				printf("lack option arg\n");
				break;
			default:
				break;
		}
	}

	if (flag == 0) {
		return -1;
	}

	return 0;
}

static int init_conf()
{
	std::ifstream in(speaker.conf_file.c_str(), std::ifstream::binary);
	if (!in) {
		printf("Cannot open conf file: %s\n", speaker.conf_file.c_str());
		return -1;
	}

	Json::Reader reader;
	bool ret = reader.parse(in, speaker.conf);
	if (!ret) {
		in.close();
		return -1;
	}

	in.close();
	return 0;
}

static void dump_conf()
{
	std::cout << "pid_file: " << speaker.conf.get("pid_file", "conf/lottery.pid").asString() << endl;
	std::cout << "xt_log:log_file: " << speaker.conf["xt_log"].get("log_file", "xt_log/speaker.xt_log").asString() << endl;
	std::cout << "xt_log:level: " << speaker.conf["xt_log"].get("level", 4).asInt() << endl;
	std::cout << "xt_log:console: " << speaker.conf["xt_log"].get("console", 0).asInt() << endl;
	std::cout << "xt_log:rotate: " << speaker.conf["xt_log"].get("rotate", 1).asInt() << endl;
	std::cout << "xt_log:max_size: " << speaker.conf["xt_log"].get("max_size", 1073741824).asInt() << endl;
	std::cout << "xt_log:max_file: " << speaker.conf["xt_log"].get("max_file", 50).asInt() << endl;
	std::cout << "server:host: " << speaker.conf["server"]["host"].asString() << endl;
	std::cout << "server:port: " << speaker.conf["server"]["port"].asInt() << endl;
}

static int set_rlimit(int n)
{
	struct rlimit rt;

	rt.rlim_max = rt.rlim_cur = n;
	if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
		xt_log.error("File: %s Func: %s Line: %d => setrlimit %s.\n",
				__FILE__, __FUNCTION__, __LINE__, strerror(errno));
		perror("setrlimit");
		return -1;
	}

	return 0;
}

static int init_redis()
{
	int ret;
	int i;

	speaker.main_size = speaker.conf["main-db"].size();
	for (i = 0; i < speaker.main_size; ++i) {
		speaker.main_rc[i] = new RedisClient();
		ret = speaker.main_rc[i]->init(speaker.conf["main-db"][i]["host"].asString(),
				speaker.conf["main-db"][i]["port"].asInt(), 1000, speaker.conf["main-db"][i]["pass"].asString());
		if (ret < 0) {
			xt_log.error("main db redis error.\n");
			return -1;
		}
	}

	// init eventlog_rc
	speaker.eventlog_rc = new RedisClient();
	ret = speaker.eventlog_rc->init(speaker.conf["eventlog-db"]["host"].asString(),
			speaker.conf["eventlog-db"]["port"].asInt(), 1000, speaker.conf["eventlog-db"]["pass"].asString());
	if (ret < 0) {
		xt_log.error("eventlog db redis error.\n");
		return -1;
	}
/*
	// init sub
	speaker.sub_size = speaker.conf["sub-db"].size();
	for (i = 0; i < speaker.sub_size; ++i) {
		speaker.sub_rc[i] = new RedisSub();
		ret = speaker.sub_rc[i]->init(speaker.conf["sub-db"][i]["host"].asString(),
					speaker.conf["sub-db"][i]["port"].asInt(), speaker.conf["sub-db"][i]["pass"].asString());
		if (ret < 0) {
			xt_log.error("sub db redis error.\n");
			return -1;
		}
	}

	// init pub
	speaker.pub_size = speaker.conf["pub-db"].size();
	for (i = 0; i < speaker.pub_size; ++i) {
		speaker.pub_rc[i] = new RedisClient();
		ret = speaker.pub_rc[i]->init(speaker.conf["pub-db"][i]["host"].asString(),
						speaker.conf["pub-db"][i]["port"].asInt(), 1000, speaker.conf["pub-db"][i]["pass"].asString());
		if (ret < 0) {
			xt_log.error("main db redis error.\n");
			return -1;
		}
	}
*/
	return 0;
}
