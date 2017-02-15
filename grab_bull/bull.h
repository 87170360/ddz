#ifndef _LANDLORD_H_
#define _LANDLORD_H_

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <json/json.h>
#include "redis_client.h"


///���������id������1000
#define XT_ROBOT_UID_MAX 1000


class Game;

class ZJH
{
	public:
		std::string       conf_file;      ///�����ļ���
		int               is_daemonize;   ///�Ƿ�תΪ�ػ�����
		Json::Value       conf;           ///�����ļ�JSON
		Game              *game;          ///��Ϸ�������
		struct ev_loop    *loop;          ///�¼�ѭ������

		RedisClient		  *main_rc[20];   ///��redis
		int				  main_size;      ///��redis�����

		RedisClient       *eventlog_rc;   //cfc add  20140102
		RedisClient 	  *cache_rc;      ///����redis
};

#endif    /* _LANDLORD_H_ */
