#ifndef _LANDLORD_H_
#define _LANDLORD_H_

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <json/json.h>
#include "redis_client.h"

#define XT_ROBOT_UID_MAX 1000

class Game;

class LZDDZ
{
    public:
        std::string         conf_file;
        int                 is_daemonize;
        Json::Value         conf;
        Game                *game;
        struct ev_loop      *loop;
        RedisClient			*main_rc[20];
        RedisClient         *eventlog_rc;   
        RedisClient 		*cache_rc;
        int					main_size;


};

#endif    /* _LANDLORD_H_ */
