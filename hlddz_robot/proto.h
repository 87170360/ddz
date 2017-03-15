#ifndef __PROTO_H__
#define __PROTO_H__

enum CLIENT_COMMAND
{
    CLIENT_LOGIN                = 1001,         //登录   
    CLIENT_PREPARE              = 1002,         //准备 
    CLIENT_CALL                 = 1003,         //叫分 score = 0,1,2,3
    CLIENT_FARMER               = 1004,         //农民加倍
    CLIENT_OUT                  = 1005,         //出牌
};

enum SERVER_COMMAND
{
    SERVER_RESPOND              = 2000,         //客户端请求回复
    SERVER_LOGIN                = 2001,         //玩家登录
    SERVER_CARD_1               = 2002,         //第一次发牌17张,开始叫分:当前操作者id:cur_id
    SERVER_AGAIN_CALL           = 2003,         //继续叫分,通知下一个叫分: 上次叫分:score, 当前操作者id:cur_id,上一个操作者id:pre_id 
    SERVER_RESULT_CALL          = 2004,         //叫分结果： 最终分数:score，地主id:lord
    SERVER_START_FARMER         = 2005,         //开始农民加倍
    SERVER_FARMER_INFO          = 2006,         //农民加倍结果
    SERVER_CARD_2               = 2007,         //第二次发牌3张底牌
    SERVER_START_OUT            = 2008,         //开始出牌
    SERVER_OUT_INFO             = 2009,         //出牌信息
    SERVER_END                  = 2010,         //牌局结束
};

enum ERROR_CODE
{
    CODE_SUCCESS                = 0,            //成功 
    CODE_SKEY                   = 1,            //skey错误
};  

enum STATE
{
    STATE_WAIT                  = 1,            //等待
    STATE_CALL                  = 2,            //叫分
    STATE_DOUBLE                = 3,            //加倍
    STATE_CARD                  = 4,            //出牌
    STATE_END                   = 5,            //结算
};

//当前座位状态
enum OP_STATE
{
    CALL_WAIT                   = 1,            //等待叫分通知
    CALL_NOTIFY                 = 2,            //已叫分通知
    DOUBLE_WAIT                 = 3,            //等待通知
    DOUBLE_NOTIFY               = 4,            //已通知
    CARD_WAIT                   = 5,            //等待通知
    CARD_NOTIFY                 = 6,            //已通知
};

#endif
