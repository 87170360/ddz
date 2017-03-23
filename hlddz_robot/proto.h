#ifndef __PROTO_H__
#define __PROTO_H__

enum CLIENT_COMMAND
{
    CLIENT_LOGIN                = 1001,         //登录   
    CLIENT_PREPARE              = 1002,         //准备 
    CLIENT_CALL                 = 1003,         //叫分 score = 0,1,2,3
    CLIENT_DOUBLE               = 1004,         //农民加倍 count= 0, 1
    CLIENT_OUT                  = 1005,         //出牌 不出: keep: true, false, 牌: card
    CLIENT_LOGOUT               = 1006,         //
};

enum SERVER_COMMAND
{
    SERVER_RESPOND              = 2000,         //其他回复
    SERVER_LOGIN                = 2001,         //其他玩家登录  
    SERVER_CARD_1               = 2002,         //第一次发牌17张,开始叫分:当前操作者id:cur_id, 叫分倒计时:time, 17张牌:card
    SERVER_AGAIN_CALL           = 2003,         //通知下一个叫分: 上次叫分:score, 当前操作者id:cur_id,上一个操作者id:pre_id, 叫分倒计时:time
    SERVER_RESULT_CALL          = 2004,         //叫分结果： 最终分数:score, 地主id:lord, 加倍倒计时:time, 加倍操作者id:cur_id, 3底牌:card, 
    SERVER_AGAIN_DOUBLE         = 2005,         //通知下一个农民加倍: 上次加倍情况:count, 当前操作者id:cur_id,上一个操作者id:pre_id, 加倍倒计时:time
    SERVER_RESULT_DOUBLE        = 2006,         //加倍结果,发底牌,通知地主出牌: 总倍数:count, 当前操作者(地主)id:cur_id,出牌倒计时:time
    SERVER_AGAIN_OUT            = 2007,         //通知下一个出牌, 上轮不出: keep = true, false, 上轮牌: card, 上轮出牌者id: out_id, 当前操作者id:cur_id, 上一轮操作者id:pre_id, 出牌倒计时:time
    SERVER_END                  = 2008,         //牌局结束
    SERVER_REPREPARE            = 2009,         //通知机器人重新准备
};

enum ERROR_CODE
{
    CODE_SUCCESS                = 0,            //成功 
    CODE_SKEY                   = 1,            //skey错误
    CODE_RELOGIN                = 1,            //重连错误，牌桌没有这个玩家
};  

//游戏阶段
enum STATE
{
    STATE_PREPARE               = 1,            //准备
    STATE_CALL                  = 2,            //叫分
    STATE_DOUBLE                = 3,            //加倍
    STATE_OUT                   = 4,            //出牌
    STATE_END                   = 5,            //结算
};

//当前座位状态
enum OP_STATE
{
    PREPARE_WAIT                = 1,            //等待准备
    PREPARE_REDAY               = 2,            //已准备
    CALL_WAIT                   = 3,            //等待叫分通知
    CALL_NOTIFY                 = 4,            //已通知
    CALL_RECEIVE                = 5,            //已经叫分
    DOUBLE_WAIT                 = 6,            //等待加倍
    DOUBLE_NOTIFY               = 7,            //已通知
    DOUBLE_NONE                 = 8,            //不参与
    OUT_WAIT                    = 9,            //等待出牌
};

#endif
