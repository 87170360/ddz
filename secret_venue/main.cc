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
#include "bull.h"
#include "game.h"
#include "log.h"

static int parse_arg(int argc, char **argv);
static int init_conf();
static void dump_conf();
static int set_rlimit(int n);
static int init_redis();

Bull gBull;     //斗牛全局对象
Log xt_log;

int main(int argc, char **argv)
{
    int ret;

    ret = parse_arg(argc, argv);
    if (ret < 0) 
	{
        xt_log.fatal("File: %s Func: %s Line: %d => parse_arg.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }
    ret = init_conf();
    if (ret < 0) 
	{
        xt_log.fatal("File: %s Func: %s Line: %d => init_conf.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }
    
	dump_conf();
	
    if (gBull.is_daemonize == 1)
    {
        daemonize();
    }
        
    signal(SIGPIPE, SIG_IGN);
    
    ret = single_instance_running(gBull.conf.get("pid_file", "conf/normal_bull.pid").asString().c_str());
    if (ret < 0) 
	{
        xt_log.fatal("File: %s Func: %s Line: %d => single_instance_running.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }

    xt_log.start(gBull.conf["log"].get("log_file", "log/secret_venue.log").asString(),
				 gBull.conf["log"].get("level", 4).asInt(),
				 gBull.conf["log"].get("console", 0).asInt(),
				 gBull.conf["log"].get("rotate", 1).asInt(),
				 gBull.conf["log"].get("max_size", 1073741824).asInt(),
				 gBull.conf["log"].get("max_file", 50).asInt());

	set_rlimit(10240);

	///初始redis库
    ret = init_redis();
    if (ret < 0) //connect redis
    {
        xt_log.fatal("init redis failed\n");
		exit(1);
    }
	
    struct ev_loop *loop = ev_default_loop(0);
    gBull.loop = loop;   ///---生成一个全局的事件循环对象
	
    gBull.game = new (std::nothrow) Game();
    gBull.game->start();
	
    ev_loop(loop, 0);
    
    return 0;
}

static int parse_arg(int argc, char **argv)
{
    int flag = 0;
    int oc; /* option chacb. */
    char ic; /* invalid chacb. */
    
    gBull.is_daemonize = 0;     ///初始非守护进程
    while((oc = getopt(argc, argv, "Df:")) != -1) {
        switch(oc) {
            case 'D':
                gBull.is_daemonize = 1;
                break;
            case 'f':
                flag = 1;
                gBull.conf_file = string(optarg);   //保存配置文件名
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
    std::ifstream in(gBull.conf_file.c_str(), std::ifstream::binary); 
    if (!in) {
		std::cout << "init file no found." << endl;
		return -1;
	}
	
	Json::Reader reader;
	bool ret = reader.parse(in, gBull.conf);  //文件格式是否正确
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
        << gBull.conf.get("pid_file", "conf/normal_bull.pid").asString() << endl;
	
	std::cout << "log:log_file: "
        << gBull.conf["log"].get("log_file", "log/normal_bull.log").asString()
        << endl;
	std::cout << "log:level: "
        << gBull.conf["log"].get("level", 4).asInt() << endl;
	std::cout << "log:console: "
        << gBull.conf["log"].get("console", 0).asInt() << endl;
	std::cout << "log:rotate: "
        << gBull.conf["log"].get("rotate", 1).asInt() << endl;
	std::cout << "log:max_size: "
        << gBull.conf["log"].get("max_size", 1073741824).asInt() << endl;
	std::cout << "log:max_file: "
        << gBull.conf["log"].get("max_file", 50).asInt() << endl;

	std::cout << "game:host: "
        << gBull.conf["game"]["host"].asString() << endl;
	std::cout << "game:port: "
        << gBull.conf["game"]["port"].asInt() << endl;
	
	gBull.main_size = gBull.conf["main-db"].size();
	for (int i = 0; i < gBull.main_size; i++)
	{
		std::cout << "[" << i << "]main-db:host: "
	        << gBull.conf["main-db"][i]["host"].asString() << endl;
		std::cout << "[" << i << "]main-db:port: "
	        << gBull.conf["main-db"][i]["port"].asInt() << endl;
		std::cout << "[" << i << "]main-db:pass: "
	        << gBull.conf["main-db"][i]["pass"].asString() << endl;	
	}
			
	std::cout << "tables:begin: "
        << gBull.conf["tables"]["begin"].asInt() << endl;
	std::cout << "tables:end: "
        << gBull.conf["tables"]["end"].asInt() << endl;
	std::cout << "tables:min_money: "
        << gBull.conf["tables"]["min_money"].asInt() << endl;
	std::cout << "tables:max_money: "
        << gBull.conf["tables"]["max_money"].asInt() << endl;
	std::cout << "tables:base_money: "
        << gBull.conf["tables"]["base_money"].asInt() << endl;
	std::cout << "tables:vid: "
        << gBull.conf["tables"]["vid"].asInt() << endl;
	std::cout << "tables:zid: "
        << gBull.conf["tables"]["zid"].asInt() << endl;

}

static int set_rlimit(int n)
{
    struct rlimit rt;
  
    rt.rlim_max = rt.rlim_cur = n;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) 
	{
        xt_log.error("File: %s Func: %s Line: %d => setrlimit %s.\n",
			__FILE__, __FUNCTION__, __LINE__, strerror(errno));
        return -1;   
    }
    
    return 0;
}

static int init_redis()
{
	int ret;
	
    gBull.main_size = gBull.conf["main-db"].size();
    for (int i = 0; i < gBull.main_size; i++)
    {
        gBull.main_rc[i] = new RedisClient();    ///创建并连接上redis库
        ret = gBull.main_rc[i]->init(gBull.conf["main-db"][i]["host"].asString()
                    , gBull.conf["main-db"][i]["port"].asInt(), 1000, gBull.conf["main-db"][i]["pass"].asString());
        if (ret < 0)
        {
            xt_log.error("main db redis error\n");
            return -1;      
        }
    }

    // cfc add eventlog redis 20140102   ///记录事件
    gBull.eventlog_rc = new RedisClient();
    ret = gBull.eventlog_rc->init(gBull.conf["eventlog-db"]["host"].asString(),
    		gBull.conf["eventlog-db"]["port"].asInt(), 1000, gBull.conf["eventlog-db"]["pass"].asString());
    if (ret < 0) {
    	xt_log.error("eventlog db redis error.\n");
    	return -1;
    }

	gBull.cache_rc =new RedisClient();   ///缓存库

    ret = gBull.cache_rc->init(gBull.conf["cache-db"]["host"].asString(),
    		gBull.conf["cache-db"]["port"].asInt(), 1000, gBull.conf["cache-db"]["pass"].asString());

	if (ret < 0)
	{
		xt_log.error("cache db redis error");
		return -1;
	}

	// ++ 私密table的redis
	gBull.table_rc = new RedisClient();
	ret = gBull.table_rc->init(
		gBull.conf["table-db"]["host"].asString(),
		gBull.conf["table-db"]["port"].asInt(), 
		1000, 
		gBull.conf["table-db"]["pass"].asString());

	if (ret < 0)
	{
		xt_log.error("table db redis error");
		return -1;
	}

	// 场
	gBull.venue_rc = new RedisClient();
	ret = gBull.venue_rc->init(
		gBull.conf["venue-db"]["host"].asString(),
		gBull.conf["venue-db"]["port"].asInt(), 
		1000, 
		gBull.conf["venue-db"]["pass"].asString());

	if (ret < 0)
	{
		xt_log.error("venue db redis error");
		return -1;
	}

    return 0;
}

