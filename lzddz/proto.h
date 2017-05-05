#ifndef __PROTO_H__
#define __PROTO_H__

enum CLIENT_COMMAND
{
    CLIENT_LOGIN                = 1001,         //登录 uid, zid
    CLIENT_PREPARE              = 1002,         //准备 
    CLIENT_CALL                 = 1003,         //响应叫地主  act: true, false
    CLIENT_GRAB                 = 1004,         //响应抢地主  act: true, false
    CLIENT_DOUBLE               = 1005,         //农民加倍 double: true, false
    CLIENT_OUT                  = 1006,         //出牌 不出: keep: true, false, 牌: card
    CLIENT_LOGOUT               = 1007,         //退出
    CLIENT_CHANGE               = 1008,         //换桌
    CLIENT_VIEW                 = 1009,         //查看信息 uid 
    CLIENT_ENTRUST              = 1010,         //托管 开启或者关闭 active: true, false
    CLIENT_CHAT                 = 1011,         //聊天 字符串content, 表情id: chatid
    CLIENT_MOTION               = 1012,         //玩家互动 目标id：target_id, 互动id: type, 
};

enum SERVER_COMMAND
{
    SERVER_RESPOND              = 2000,         //其他回复, 消息id:msgid, code:错误码
    SERVER_LOGIN                = 2001,         //其他玩家登录  
    SERVER_CARD_1               = 2002,         //发牌17张, 17张牌:card, 癞子点数:face
    SERVER_CALL                 = 2003,         //通知叫地主: 操作者id:cur_id, 叫分倒计时:time, 发牌时间:show_time 
    SERVER_CALL_RSP             = 2004,         //广播响应叫地主, 操作者id: cur_id
    SERVER_GRAB                 = 2005,         //通知抢地主, 操作者id: cur_id
    SERVER_GRAB_RSP             = 2006,         //广播响应抢地主, 操作者id: cur_id
    SERVER_RESULT_GRAB          = 2007,         //抢地主结果,开始加倍：当期加倍情况:count, 地主id:lord, 加倍倒计时:time, 3底牌:card, 
    SERVER_DOUBLE               = 2008,         //通知加倍情况: 总加倍情况:count, 操作者id:pre_id, 是否加倍: double
    SERVER_RESULT_DOUBLE        = 2009,         //加倍结果,发底牌,通知地主出牌: 总倍数:count, 当前操作者(地主)id:cur_id,出牌倒计时:time
    SERVER_AGAIN_OUT            = 2010,         //通知下一个出牌, 上轮不出: keep = true, false, 上轮牌: card, 当前操作者id:cur_id, 上一轮操作者id:pre_id, 上轮牌出牌人out_id 出牌倒计时:time,牌数:num
    SERVER_END                  = 2011,         //牌局结束, info{uid, name, 是否地主isLord, 底分score, 倍数double, 炸弹数bomb}
    SERVER_REPREPARE            = 2012,         //通知机器人重新准备
    SERVER_KICK                 = 2013,         //踢人离场
    SERVER_TIME                 = 2014,         //定时器时间秒:time
    SERVER_ENTRUST              = 2015,         //托管 玩家uid, 开始或者关闭active: true, false
    SERVER_CHAT                 = 2016,         //聊天 字符串content, uid, chatid
    SERVER_ALLOWANCE            = 2017,         //破产补助, 增加金币数量 money
    SERVER_MOTION               = 2018,         //玩家互动, 发起人id: src_id, 目标id: target_id, 互动id: type, 价格:price
    SERVER_ENTRUST_OUT          = 2019,         //托管出牌 牌型: card, 不出: keep
    SERVER_ENTRUST_CALL         = 2020,         //托管叫分 score
    SERVER_ENTRUST_DOUBLE       = 2021,         //托管加倍 double 
};

enum ERROR_CODE
{
    CODE_SUCCESS                = 0,            //成功 
    CODE_SKEY                   = 1,            //skey错误
    CODE_RELOGIN                = 2,            //重连错误，牌桌没有这个玩家
    CODE_MONEY                  = 3,            //金币不足
    CODE_PREPARE                = 4,            //重复准备
    CODE_STATE                  = 5,            //游戏状态不对
    CODE_SEAT                   = 6,            //座位错误
    CODE_NOTIFY                 = 7,            //未通知提前操作,或者已经自动处理
    CODE_CURRENT                = 8,            //未轮到你操作
    CODE_SCORE                  = 9,            //叫分错误
    CODE_LORD                   = 10,           //地主不能加倍
    CODE_DOUBLE                 = 11,           //重复加倍
    CODE_CARD                   = 12,           //牌型错误
    CODE_NOEXIST                = 13,           //用户不存在
    CODE_KEEP                   = 14,           //不出牌，但有牌
    CODE_COMPARE                = 15,           //比较牌型错误
    CODE_ENTRUST                = 16,           //已经托管
    CODE_OUT_ENTRUST            = 17,           //出牌阶段才可以托管
    CODE_REPEAT_ENTRUST         = 18,           //重复开启或者关闭托管
};  

//游戏阶段
enum STATE
{
    STATE_NULL                  = 0,
    STATE_PREPARE               = 1,            //准备
    STATE_CALL                  = 2,            //叫地主
    STATE_GRAB                  = 3,            //地主
    STATE_DOUBLE                = 4,            //加倍
    STATE_OUT                   = 5,            //出牌
    STATE_END                   = 6,            //结算
    STATE_MAX                   = 7,            //
};

static const char* DESC_STATE[STATE_MAX] = 
{
    "STATE_NULL",
    "STATE_PREPARE",
    "STATE_CALL",
    "STATE_GRAB",
    "STATE_DOUBLE",
    "STATE_OUT",
    "STATE_END"
};

//当前座位状态
enum OP_STATE
{
    OP_NULL                        = 0,            //NULL
    OP_PREPARE_WAIT                = 1,            //等待准备
    OP_PREPARE_REDAY               = 2,            //已准备
    OP_CALL_WAIT                   = 3,            //等待叫地主通知
    OP_CALL_NOTIFY                 = 4,            //已通知叫地主
    OP_CALL_RECEIVE                = 5,            //已经响应叫地主
    OP_GRAB_NOTIFY                 = 7,            //已通知抢地主
    OP_GRAB_RECEIVE                = 8,            //已经响应抢地主
    OP_DOUBLE_NOTIFY               = 9,            //已经通知
    OP_DOUBLE_RECEIVE              = 10,           //已经响应
    OP_OUT_WAIT                    = 11,           //等待出牌
    OP_GAME_END                    = 12,           //结算中
    OP_MAX                         = 13,           //MAX
};

static const char* DESC_OP[OP_MAX] = 
{
    "OP_NULL",
    "OP_PREPARE_WAIT",
    "OP_PREPARE_REDAY",
    "OP_CALL_WAIT",
    "OP_CALL_NOTIFY",
    "OP_CALL_RECEIVE",
    "OP_GRAB_WAIT",
    "OP_GRAB_NOTIFY",
    "OP_GRAB_RECEIVE",
    "OP_DOUBLE_NOTIFY",
    "OP_DOUBLE_RECEIVE",
    "OP_OUT_WAIT",
    "OP_GAME_END",
};

#endif
