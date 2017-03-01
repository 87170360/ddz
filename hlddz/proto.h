#ifndef __PROTO_H__
#define __PROTO_H__

enum client_command
{
    CLIENT_LOGIN                = 1001,         //登录   
    CLIENT_START                = 1002,         //开始 
    CLIENT_CALL                 = 1003,         //叫分
    CLIENT_FARMER               = 1004,         //农民加倍
    CLIENT_OUT                  = 1005,         //出牌
};

enum server_command
{
    SERVER_RESPOND              = 2000,         //客户端请求回复
    SERVER_PLAYER_INFO          = 2001,         //玩家信息
    SERVER_CARD_1               = 2002,         //第一次发牌17张
    SERVER_START_CALL           = 2003,         //开始叫分
    SERVER_CALL_INFO            = 2004,         //叫分情况
    SERVER_START_FARMER         = 2005,         //开始农民加倍
    SERVER_FARMER_INFO          = 2006,         //农民加倍结果
    SERVER_CARD_2               = 2007,         //第二次发牌3张底牌
    SERVER_START_OUT            = 2008,         //开始出牌
    SERVER_OUT_INFO             = 2009,         //出牌信息
    SERVER_END                  = 2010,         //牌局结束
};

enum error_code
{
    CODE_SUCCESS                = 0,            //成功 
    CODE_SKEY                   = 1,            //skey错误
};  

#endif
