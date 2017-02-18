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
#include "ddz.h"
#include "game.h"
#include "log.h"

static int parse_arg(int argc, char **argv);
static int init_conf();
static void dump_conf();
static int set_rlimit(int n);
static int init_redis();

DDZ ddz;
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
	
    if (ddz.is_daemonize == 1)
        daemonize();
    signal(SIGPIPE, SIG_IGN);
    
    ret = single_instance_running(ddz.conf.get("pid_file", "conf/zjhsvr.pid").asString().c_str());
    if (ret < 0) {
        xt_log.fatal("File: %s Func: %s Line: %d => single_instance_running.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }

    xt_log.start(ddz.conf["log"].get("log_file", "log/zjhsvr.log").asString(),
				ddz.conf["log"].get("level", 4).asInt(),
				ddz.conf["log"].get("console", 0).asInt(),
				ddz.conf["log"].get("rotate", 1).asInt(),
				ddz.conf["log"].get("max_size", 1073741824).asInt(),
				ddz.conf["log"].get("max_file", 50).asInt());

	set_rlimit(10240);

    ret = init_redis();
    if (ret < 0) //connect redis
    {
        xt_log.fatal("init redis failed\n");
		exit(1);
    }
	
    struct ev_loop *loop = ev_default_loop(0);
    ddz.loop = loop;
	
    ddz.game = new (std::nothrow) Game();
    ddz.game->start();
	
    ev_loop(loop, 0);
    
    return 0;
}

static int parse_arg(int argc, char **argv)
{
    int flag = 0;
    int oc; /* option chacb. */
    char ic; /* invalid chacb. */
    
    ddz.is_daemonize = 0;
    while((oc = getopt(argc, argv, "Df:")) != -1) {
        switch(oc) {
            case 'D':
                ddz.is_daemonize = 1;
                break;
            case 'f':
                flag = 1;
                ddz.conf_file = string(optarg);
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
    std::ifstream in(ddz.conf_file.c_str(), std::ifstream::binary); 
    if (!in) {
		std::cout << "init file no found." << endl;
		return -1;
	}
	
	Json::Reader reader;
	bool ret = reader.parse(in, ddz.conf);
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
        << ddz.conf.get("pid_file", "conf/zjhsvr.pid").asString() << endl;
	
	std::cout << "log:log_file: "
        << ddz.conf["log"].get("log_file", "log/zjhsvr.log").asString()
        << endl;
	std::cout << "log:level: "
        << ddz.conf["log"].get("level", 4).asInt() << endl;
	std::cout << "log:console: "
        << ddz.conf["log"].get("console", 0).asInt() << endl;
	std::cout << "log:rotate: "
        << ddz.conf["log"].get("rotate", 1).asInt() << endl;
	std::cout << "log:max_size: "
        << ddz.conf["log"].get("max_size", 1073741824).asInt() << endl;
	std::cout << "log:max_file: "
        << ddz.conf["log"].get("max_file", 50).asInt() << endl;

	std::cout << "game:host: "
        << ddz.conf["game"]["host"].asString() << endl;
	std::cout << "game:port: "
        << ddz.conf["game"]["port"].asInt() << endl;
	
	ddz.main_size = ddz.conf["main-db"].size();
	for (int i = 0; i < ddz.main_size; i++)
	{
		std::cout << "[" << i << "]main-db:host: "
	        << ddz.conf["main-db"][i]["host"].asString() << endl;
		std::cout << "[" << i << "]main-db:port: "
	        << ddz.conf["main-db"][i]["port"].asInt() << endl;
		std::cout << "[" << i << "]main-db:pass: "
	        << ddz.conf["main-db"][i]["pass"].asString() << endl;	
	}
			
	std::cout << "tables:begin: "
        << ddz.conf["tables"]["begin"].asInt() << endl;
	std::cout << "tables:end: "
        << ddz.conf["tables"]["end"].asInt() << endl;
	std::cout << "tables:min_money: "
        << ddz.conf["tables"]["min_money"].asInt() << endl;
	std::cout << "tables:max_money: "
        << ddz.conf["tables"]["max_money"].asInt() << endl;
	std::cout << "tables:base_money: "
        << ddz.conf["tables"]["base_money"].asInt() << endl;
	std::cout << "tables:min_round: "
        << ddz.conf["tables"]["min_round"].asInt() << endl;
	std::cout << "tables:max_round: "
        << ddz.conf["tables"]["max_round"].asInt() << endl;
	std::cout << "tables:vid: "
        << ddz.conf["tables"]["vid"].asInt() << endl;
	std::cout << "tables:zid: "
        << ddz.conf["tables"]["zid"].asInt() << endl;
	std::cout << "tables:type: "
        << ddz.conf["tables"]["type"].asInt() << endl;
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
	
    ddz.main_size = ddz.conf["main-db"].size();
    for (int i = 0; i < ddz.main_size; i++)
    {
        ddz.main_rc[i] = new RedisClient();
        ret = ddz.main_rc[i]->init(ddz.conf["main-db"][i]["host"].asString()
                    , ddz.conf["main-db"][i]["port"].asInt(), 1000, ddz.conf["main-db"][i]["pass"].asString());
        if (ret < 0)
        {
            xt_log.error("main db redis error\n");
            return -1;      
        }
    }

    // cfc add eventlog redis 20140102
    ddz.eventlog_rc = new RedisClient();
    ret = ddz.eventlog_rc->init(ddz.conf["eventlog-db"]["host"].asString(),
    		ddz.conf["eventlog-db"]["port"].asInt(), 1000, ddz.conf["eventlog-db"]["pass"].asString());
    if (ret < 0) {
    	xt_log.error("eventlog db redis error.\n");
    	return -1;
    }

	ddz.cache_rc =new RedisClient();

    ret = ddz.cache_rc->init(ddz.conf["cache-db"]["host"].asString(),
    		ddz.conf["cache-db"]["port"].asInt(), 1000, ddz.conf["cache-db"]["pass"].asString());

	if(ret<0)
	{
		xt_log.error("cache db redis error");
		return -1;
	}
    return 0;
}

