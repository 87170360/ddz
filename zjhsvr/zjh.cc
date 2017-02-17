#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ev.h>

#include "daemonize.h"
#include "zjh.h"
#include "game.h"
#include "log.h"

static int parse_arg(int argc, char **argv);
static int init_conf();
static void dump_conf();
static int set_rlimit(int n);
static int init_redis();

ZJH zjh;
Log xt_log;

int main(int argc, char **argv)
{
    int ret;
	
    ret = parse_arg(argc, argv);
    if (ret < 0) {
        xt_log.fatal("File: %s Func: %s Line: %d => parse_arg.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }
    ret = init_conf();
    if (ret < 0) {
        xt_log.fatal("File: %s Func: %s Line: %d => init_conf.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }
    
	dump_conf();
	
    if (zjh.is_daemonize == 1)
        daemonize();
    signal(SIGPIPE, SIG_IGN);
    
    ret = single_instance_running(zjh.conf.get("pid_file", "conf/zjhsvr.pid").asString().c_str());
    if (ret < 0) {
        xt_log.fatal("File: %s Func: %s Line: %d => single_instance_running.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }

    xt_log.start(zjh.conf["log"].get("log_file", "log/zjhsvr.log").asString(),
				zjh.conf["log"].get("level", 4).asInt(),
				zjh.conf["log"].get("console", 0).asInt(),
				zjh.conf["log"].get("rotate", 1).asInt(),
				zjh.conf["log"].get("max_size", 1073741824).asInt(),
				zjh.conf["log"].get("max_file", 50).asInt());

	set_rlimit(10240);

    ret = init_redis();
    if (ret < 0) //connect redis
    {
        xt_log.fatal("init redis failed\n");
		exit(1);
    }
	
    struct ev_loop *loop = ev_default_loop(0);
    zjh.loop = loop;
	
    zjh.game = new (std::nothrow) Game();
    zjh.game->start();
	
    ev_loop(loop, 0);
    
    return 0;
}

static int parse_arg(int argc, char **argv)
{
    int flag = 0;
    int oc; /* option chacb. */
    char ic; /* invalid chacb. */
    
    zjh.is_daemonize = 0;
    while((oc = getopt(argc, argv, "Df:")) != -1) {
        switch(oc) {
            case 'D':
                zjh.is_daemonize = 1;
                break;
            case 'f':
                flag = 1;
                zjh.conf_file = string(optarg);
                break;
            case '?':
                ic = (char)optopt;
                printf("invalid \'%c\'\n", ic);
                break;
            case ':':
                printf("lack option arg\n");
                break;
        }
    }
    
    if (flag == 0)
        return -1;
        
    return 0;
}

static int init_conf()
{
    std::ifstream in(zjh.conf_file.c_str(), std::ifstream::binary); 
    if (!in) {
		std::cout << "init file no found." << endl;
		return -1;
	}
	
	Json::Reader reader;
	bool ret = reader.parse(in, zjh.conf);
	if (!ret) {
		in.close();
		std::cout << "init file parser." << endl;
		return -1;
	}
	
	in.close();
	return 0;
}

static void dump_conf()
{
	std::cout << "pid_file: "
        << zjh.conf.get("pid_file", "conf/zjhsvr.pid").asString() << endl;
	
	std::cout << "log:log_file: "
        << zjh.conf["log"].get("log_file", "log/zjhsvr.log").asString()
        << endl;
	std::cout << "log:level: "
        << zjh.conf["log"].get("level", 4).asInt() << endl;
	std::cout << "log:console: "
        << zjh.conf["log"].get("console", 0).asInt() << endl;
	std::cout << "log:rotate: "
        << zjh.conf["log"].get("rotate", 1).asInt() << endl;
	std::cout << "log:max_size: "
        << zjh.conf["log"].get("max_size", 1073741824).asInt() << endl;
	std::cout << "log:max_file: "
        << zjh.conf["log"].get("max_file", 50).asInt() << endl;

	std::cout << "game:host: "
        << zjh.conf["game"]["host"].asString() << endl;
	std::cout << "game:port: "
        << zjh.conf["game"]["port"].asInt() << endl;
	
	zjh.main_size = zjh.conf["main-db"].size();
	for (int i = 0; i < zjh.main_size; i++)
	{
		std::cout << "[" << i << "]main-db:host: "
	        << zjh.conf["main-db"][i]["host"].asString() << endl;
		std::cout << "[" << i << "]main-db:port: "
	        << zjh.conf["main-db"][i]["port"].asInt() << endl;
		std::cout << "[" << i << "]main-db:pass: "
	        << zjh.conf["main-db"][i]["pass"].asString() << endl;	
	}
			
	std::cout << "tables:begin: "
        << zjh.conf["tables"]["begin"].asInt() << endl;
	std::cout << "tables:end: "
        << zjh.conf["tables"]["end"].asInt() << endl;
	std::cout << "tables:min_money: "
        << zjh.conf["tables"]["min_money"].asInt() << endl;
	std::cout << "tables:max_money: "
        << zjh.conf["tables"]["max_money"].asInt() << endl;
	std::cout << "tables:base_money: "
        << zjh.conf["tables"]["base_money"].asInt() << endl;
	std::cout << "tables:min_round: "
        << zjh.conf["tables"]["min_round"].asInt() << endl;
	std::cout << "tables:max_round: "
        << zjh.conf["tables"]["max_round"].asInt() << endl;
	std::cout << "tables:vid: "
        << zjh.conf["tables"]["vid"].asInt() << endl;
	std::cout << "tables:zid: "
        << zjh.conf["tables"]["zid"].asInt() << endl;
	std::cout << "tables:type: "
        << zjh.conf["tables"]["type"].asInt() << endl;
}

static int set_rlimit(int n)
{
    struct rlimit rt;
  
    rt.rlim_max = rt.rlim_cur = n;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
        xt_log.error("File: %s Func: %s Line: %d => setrlimit %s.\n",
                            __FILE__, __FUNCTION__, __LINE__, strerror(errno));
        return -1;   
    }
    
    return 0;
}

static int init_redis()
{
	int ret;
	
    zjh.main_size = zjh.conf["main-db"].size();
    for (int i = 0; i < zjh.main_size; i++)
    {
        zjh.main_rc[i] = new RedisClient();
        ret = zjh.main_rc[i]->init(zjh.conf["main-db"][i]["host"].asString()
                    , zjh.conf["main-db"][i]["port"].asInt(), 1000, zjh.conf["main-db"][i]["pass"].asString());
        if (ret < 0)
        {
            xt_log.error("main db redis error\n");
            return -1;      
        }
    }

    // cfc add eventlog redis 20140102
    zjh.eventlog_rc = new RedisClient();
    ret = zjh.eventlog_rc->init(zjh.conf["eventlog-db"]["host"].asString(),
    		zjh.conf["eventlog-db"]["port"].asInt(), 1000, zjh.conf["eventlog-db"]["pass"].asString());
    if (ret < 0) {
    	xt_log.error("eventlog db redis error.\n");
    	return -1;
    }

	zjh.cache_rc =new RedisClient();

    ret = zjh.cache_rc->init(zjh.conf["cache-db"]["host"].asString(),
    		zjh.conf["cache-db"]["port"].asInt(), 1000, zjh.conf["cache-db"]["pass"].asString());

	if(ret<0)
	{
		xt_log.error("cache db redis error");
		return -1;
	}
    return 0;
}


// hmset u:4 user lcl name lcl password 123456 salt 123456 sex 1 money 10000 cooldou 11000 coin 10 avater 1 level 10 expr 2300 skey abc ondays 5 rewarded 1 total_board 1000 total_win 200 create_at 1234567891 login_at 1223333333
