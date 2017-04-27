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
#include "lzddz.h"
#include "game.h"
#include "log.h"

static int parse_arg(int argc, char **argv);
static int init_conf();
static void dump_conf();
static int set_rlimit(int n);
static int init_redis();

LZDDZ lzddz;
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
	
    if (lzddz.is_daemonize == 1)
        daemonize();
    signal(SIGPIPE, SIG_IGN);
    
    ret = single_instance_running(lzddz.conf.get("pid_file", "conf/zjhsvr.pid").asString().c_str());
    if (ret < 0) {
        xt_log.fatal("File: %s Func: %s Line: %d => single_instance_running.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }

    xt_log.start(lzddz.conf["log"].get("log_file", "log/zjhsvr.log").asString(),
				lzddz.conf["log"].get("level", 4).asInt(),
				lzddz.conf["log"].get("console", 0).asInt(),
				lzddz.conf["log"].get("rotate", 1).asInt(),
				lzddz.conf["log"].get("max_size", 1073741824).asInt(),
				lzddz.conf["log"].get("max_file", 50).asInt());

	set_rlimit(10240);

    ret = init_redis();
    if (ret < 0) //connect redis
    {
        xt_log.fatal("init redis failed\n");
		exit(1);
    }
	
    struct ev_loop *loop = ev_default_loop(0);
    lzddz.loop = loop;
	
    lzddz.game = new (std::nothrow) Game();
    lzddz.game->start();
	
    ev_loop(loop, 0);
    
    return 0;
}

static int parse_arg(int argc, char **argv)
{
    int flag = 0;
    int oc; /* option chacb. */
    char ic; /* invalid chacb. */
    
    lzddz.is_daemonize = 0;
    while((oc = getopt(argc, argv, "Df:")) != -1) {
        switch(oc) {
            case 'D':
                lzddz.is_daemonize = 1;
                break;
            case 'f':
                flag = 1;
                lzddz.conf_file = string(optarg);
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
    std::ifstream in(lzddz.conf_file.c_str(), std::ifstream::binary); 
    if (!in) {
		std::cout << "init file no found." << endl;
		return -1;
	}
	
	Json::Reader reader;
	bool ret = reader.parse(in, lzddz.conf);
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
        << lzddz.conf.get("pid_file", "conf/zjhsvr.pid").asString() << endl;
	
	std::cout << "log:log_file: "
        << lzddz.conf["log"].get("log_file", "log/zjhsvr.log").asString()
        << endl;
	std::cout << "log:level: "
        << lzddz.conf["log"].get("level", 4).asInt() << endl;
	std::cout << "log:console: "
        << lzddz.conf["log"].get("console", 0).asInt() << endl;
	std::cout << "log:rotate: "
        << lzddz.conf["log"].get("rotate", 1).asInt() << endl;
	std::cout << "log:max_size: "
        << lzddz.conf["log"].get("max_size", 1073741824).asInt() << endl;
	std::cout << "log:max_file: "
        << lzddz.conf["log"].get("max_file", 50).asInt() << endl;

	std::cout << "game:host: "
        << lzddz.conf["game"]["host"].asString() << endl;
	std::cout << "game:port: "
        << lzddz.conf["game"]["port"].asInt() << endl;
	
	lzddz.main_size = lzddz.conf["main-db"].size();
	for (int i = 0; i < lzddz.main_size; i++)
	{
		std::cout << "[" << i << "]main-db:host: "
	        << lzddz.conf["main-db"][i]["host"].asString() << endl;
		std::cout << "[" << i << "]main-db:port: "
	        << lzddz.conf["main-db"][i]["port"].asInt() << endl;
		std::cout << "[" << i << "]main-db:pass: "
	        << lzddz.conf["main-db"][i]["pass"].asString() << endl;	
	}
			
	std::cout << "tables:begin: "
        << lzddz.conf["tables"]["begin"].asInt() << endl;
	std::cout << "tables:end: "
        << lzddz.conf["tables"]["end"].asInt() << endl;
	std::cout << "tables:min_money: "
        << lzddz.conf["tables"]["min_money"].asInt() << endl;
	std::cout << "tables:max_money: "
        << lzddz.conf["tables"]["max_money"].asInt() << endl;
	std::cout << "tables:base_money: "
        << lzddz.conf["tables"]["base_money"].asInt() << endl;
	std::cout << "tables:min_round: "
        << lzddz.conf["tables"]["min_round"].asInt() << endl;
	std::cout << "tables:max_round: "
        << lzddz.conf["tables"]["max_round"].asInt() << endl;
	std::cout << "tables:vid: "
        << lzddz.conf["tables"]["vid"].asInt() << endl;
	std::cout << "tables:zid: "
        << lzddz.conf["tables"]["zid"].asInt() << endl;
	std::cout << "tables:type: "
        << lzddz.conf["tables"]["type"].asInt() << endl;
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
	
    lzddz.main_size = lzddz.conf["main-db"].size();
    for (int i = 0; i < lzddz.main_size; i++)
    {
        lzddz.main_rc[i] = new RedisClient();
        ret = lzddz.main_rc[i]->init(lzddz.conf["main-db"][i]["host"].asString()
                    , lzddz.conf["main-db"][i]["port"].asInt(), 1000, lzddz.conf["main-db"][i]["pass"].asString());
        if (ret < 0)
        {
            xt_log.error("main db redis error\n");
            return -1;      
        }
    }

    lzddz.eventlog_rc = new RedisClient();
    ret = lzddz.eventlog_rc->init(lzddz.conf["eventlog-db"]["host"].asString(),
    		lzddz.conf["eventlog-db"]["port"].asInt(), 1000, lzddz.conf["eventlog-db"]["pass"].asString());
    if (ret < 0) {
    	xt_log.error("eventlog db redis error.\n");
    	return -1;
    }

	lzddz.cache_rc =new RedisClient();

    ret = lzddz.cache_rc->init(lzddz.conf["cache-db"]["host"].asString(),
    		lzddz.conf["cache-db"]["port"].asInt(), 1000, lzddz.conf["cache-db"]["pass"].asString());

	if(ret<0)
	{
		xt_log.error("cache db redis error");
		return -1;
	}
    return 0;
}

