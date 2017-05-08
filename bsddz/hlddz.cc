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
#include "hlddz.h"
#include "game.h"
#include "log.h"

static int parse_arg(int argc, char **argv);
static int init_conf();
static void dump_conf();
static int set_rlimit(int n);
static int init_redis();

HLDDZ hlddz;
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
	
    if (hlddz.is_daemonize == 1)
        daemonize();
    signal(SIGPIPE, SIG_IGN);
    
    ret = single_instance_running(hlddz.conf.get("pid_file", "conf/zjhsvr.pid").asString().c_str());
    if (ret < 0) {
        xt_log.fatal("File: %s Func: %s Line: %d => single_instance_running.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }

    xt_log.start(hlddz.conf["log"].get("log_file", "log/zjhsvr.log").asString(),
				hlddz.conf["log"].get("level", 4).asInt(),
				hlddz.conf["log"].get("console", 0).asInt(),
				hlddz.conf["log"].get("rotate", 1).asInt(),
				hlddz.conf["log"].get("max_size", 1073741824).asInt(),
				hlddz.conf["log"].get("max_file", 50).asInt());

	set_rlimit(10240);

    ret = init_redis();
    if (ret < 0) //connect redis
    {
        xt_log.fatal("init redis failed\n");
		exit(1);
    }
	
    struct ev_loop *loop = ev_default_loop(0);
    hlddz.loop = loop;
	
    hlddz.game = new (std::nothrow) Game();
    hlddz.game->start();
	
    ev_loop(loop, 0);
    
    return 0;
}

static int parse_arg(int argc, char **argv)
{
    int flag = 0;
    int oc; /* option chacb. */
    char ic; /* invalid chacb. */
    
    hlddz.is_daemonize = 0;
    while((oc = getopt(argc, argv, "Df:")) != -1) {
        switch(oc) {
            case 'D':
                hlddz.is_daemonize = 1;
                break;
            case 'f':
                flag = 1;
                hlddz.conf_file = string(optarg);
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
    std::ifstream in(hlddz.conf_file.c_str(), std::ifstream::binary); 
    if (!in) {
		std::cout << "init file no found." << endl;
		return -1;
	}
	
	Json::Reader reader;
	bool ret = reader.parse(in, hlddz.conf);
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
        << hlddz.conf.get("pid_file", "conf/zjhsvr.pid").asString() << endl;
	
	std::cout << "log:log_file: "
        << hlddz.conf["log"].get("log_file", "log/zjhsvr.log").asString()
        << endl;
	std::cout << "log:level: "
        << hlddz.conf["log"].get("level", 4).asInt() << endl;
	std::cout << "log:console: "
        << hlddz.conf["log"].get("console", 0).asInt() << endl;
	std::cout << "log:rotate: "
        << hlddz.conf["log"].get("rotate", 1).asInt() << endl;
	std::cout << "log:max_size: "
        << hlddz.conf["log"].get("max_size", 1073741824).asInt() << endl;
	std::cout << "log:max_file: "
        << hlddz.conf["log"].get("max_file", 50).asInt() << endl;

	std::cout << "game:host: "
        << hlddz.conf["game"]["host"].asString() << endl;
	std::cout << "game:port: "
        << hlddz.conf["game"]["port"].asInt() << endl;
	
	hlddz.main_size = hlddz.conf["main-db"].size();
	for (int i = 0; i < hlddz.main_size; i++)
	{
		std::cout << "[" << i << "]main-db:host: "
	        << hlddz.conf["main-db"][i]["host"].asString() << endl;
		std::cout << "[" << i << "]main-db:port: "
	        << hlddz.conf["main-db"][i]["port"].asInt() << endl;
		std::cout << "[" << i << "]main-db:pass: "
	        << hlddz.conf["main-db"][i]["pass"].asString() << endl;	
	}
			
	std::cout << "tables:begin: "
        << hlddz.conf["tables"]["begin"].asInt() << endl;
	std::cout << "tables:end: "
        << hlddz.conf["tables"]["end"].asInt() << endl;
	std::cout << "tables:min_money: "
        << hlddz.conf["tables"]["min_money"].asInt() << endl;
	std::cout << "tables:max_money: "
        << hlddz.conf["tables"]["max_money"].asInt() << endl;
	std::cout << "tables:base_money: "
        << hlddz.conf["tables"]["base_money"].asInt() << endl;
	std::cout << "tables:min_round: "
        << hlddz.conf["tables"]["min_round"].asInt() << endl;
	std::cout << "tables:max_round: "
        << hlddz.conf["tables"]["max_round"].asInt() << endl;
	std::cout << "tables:vid: "
        << hlddz.conf["tables"]["vid"].asInt() << endl;
	std::cout << "tables:zid: "
        << hlddz.conf["tables"]["zid"].asInt() << endl;
	std::cout << "tables:type: "
        << hlddz.conf["tables"]["type"].asInt() << endl;
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
	
    hlddz.main_size = hlddz.conf["main-db"].size();
    for (int i = 0; i < hlddz.main_size; i++)
    {
        hlddz.main_rc[i] = new RedisClient();
        ret = hlddz.main_rc[i]->init(hlddz.conf["main-db"][i]["host"].asString()
                    , hlddz.conf["main-db"][i]["port"].asInt(), 1000, hlddz.conf["main-db"][i]["pass"].asString());
        if (ret < 0)
        {
            xt_log.error("main db redis error\n");
            return -1;      
        }
    }

    hlddz.eventlog_rc = new RedisClient();
    ret = hlddz.eventlog_rc->init(hlddz.conf["eventlog-db"]["host"].asString(),
    		hlddz.conf["eventlog-db"]["port"].asInt(), 1000, hlddz.conf["eventlog-db"]["pass"].asString());
    if (ret < 0) {
    	xt_log.error("eventlog db redis error.\n");
    	return -1;
    }

	hlddz.cache_rc =new RedisClient();

    ret = hlddz.cache_rc->init(hlddz.conf["cache-db"]["host"].asString(),
    		hlddz.conf["cache-db"]["port"].asInt(), 1000, hlddz.conf["cache-db"]["pass"].asString());

	if(ret<0)
	{
		xt_log.error("cache db redis error");
		return -1;
	}
    return 0;
}

