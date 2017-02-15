#ifndef _LANDLORD_H_
#define _LANDLORD_H_

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <json/json.h>
#include "redis_client.h"


///机器人最大id不超过1000
#define XT_ROBOT_UID_MAX 1000


class Game;

class ZJH
{
	public:
		std::string       conf_file;      ///配置文件名
		int               is_daemonize;   ///是否转为守护进程
		Json::Value       conf;           ///配置文件JSON
		Game              *game;          ///游戏处理对象
		struct ev_loop    *loop;          ///事件循环对象

		RedisClient		  *main_rc[20];   ///主redis
		int				  main_size;      ///主redis库个数

		RedisClient       *eventlog_rc;   //cfc add  20140102
		RedisClient 	  *cache_rc;      ///缓存redis
};

#endif    /* _LANDLORD_H_ */
