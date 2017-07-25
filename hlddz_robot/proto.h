#ifndef __PROTO_H__
#define __PROTO_H__

enum CLIENT_COMMAND
{
    CLIENT_LOGIN                = 1001,         //登录 uid, zid
    CLIENT_PREPARE              = 1002,         //准备 
    CLIENT_CALL                 = 1003,         //叫分 score = 0,1,2,3
    //CLIENT_DOUBLE               = 1004,         //农民加倍 double: true, false
    CLIENT_OUT                  = 1005,         //出牌 不出: keep: true, false, 牌: card
    CLIENT_LOGOUT               = 1006,         //退出
    CLIENT_CHANGE               = 1007,         //换桌
    CLIENT_VIEW                 = 1008,         //查看信息 uid 
    CLIENT_ENTRUST              = 1009,         //托管 开启或者关闭 active: true, false
    CLIENT_CHAT                 = 1010,         //聊天 字符串content, 表情id: chatid
    CLIENT_MOTION               = 1011,         //玩家互动 目标id：target_id, 互动id: type, 
    CLIENT_IDLE                 = 1012,         //机器人空闲
    CLIENT_RECORD               = 1013,         //使用记牌器
    CLIENT_FEE                  = 1014,         //领取奖励
};

enum SERVER_COMMAND
{
    SERVER_RESPOND              = 2000,         //其他回复, 消息id:msgid, code:错误码
    SERVER_LOGIN                = 2001,         //其他玩家登录  
    SERVER_CARD_1               = 2002,         //第一次发牌17张,开始叫分:当前操作者id:cur_id, 叫分倒计时:time, 发牌时间:show_time, 17张牌:card
    SERVER_AGAIN_CALL           = 2003,         //通知下一个叫分: 上次叫分:score, 当前操作者id:cur_id,上一个操作者id:pre_id, 叫分倒计时:time, 倍数:count, 叫分倍数: callcount
    SERVER_RESULT_CALL          = 2004,         //叫分结果,地主开始出牌： 最终分数:score, 地主id:lord, 加倍倒计时:time, 3底牌:card, 倍数:count, 叫分倍数: callcount
    //SERVER_DOUBLE               = 2005,         //通知加倍情况: 总加倍情况:count, 操作者id:pre_id, 是否加倍: double
    //SERVER_RESULT_DOUBLE        = 2006,         //加倍结果,发底牌,通知地主出牌: 总倍数:count, 当前操作者(地主)id:cur_id,出牌倒计时:time
    SERVER_AGAIN_OUT            = 2007,         //通知下一个出牌, 上轮不出:keep = true,false,上轮牌:card,当前操作者id:cur_id,上一轮操作者id:pre_id,上轮牌出牌人out_id 出牌倒计时:time,牌数:num, 倍数:count
    SERVER_END                  = 2008,         //牌局结束, info{uid, name, 是否地主isLord, 底分score, 倍数double, 炸弹数bomb, 兑换券coupon}, 
    SERVER_REPREPARE            = 2009,         //通知机器人重新准备
    SERVER_KICK                 = 2010,         //踢人离场
    SERVER_TIME                 = 2011,         //定时器时间秒:time
    SERVER_ENTRUST              = 2012,         //托管 玩家uid, 开始或者关闭active: true, false
    SERVER_CHAT                 = 2013,         //聊天 字符串content, uid, chatid
    SERVER_ALLOWANCE            = 2014,         //破产补助, 增加金币数量 money
    SERVER_MOTION               = 2015,         //玩家互动, 发起人id: src_id, 目标id: target_id, 互动id: type, 价格:price
    SERVER_ENTRUST_OUT          = 2016,         //托管出牌 牌型: card, 不出: keep
    SERVER_ENTRUST_CALL         = 2017,         //托管叫分 score
    SERVER_ENTRUST_DOUBLE       = 2018,         //托管加倍 double 
    SERVER_LOGOUT               = 2019,         //玩家退出 玩家id uid
    SERVER_PREPARE              = 2020,         //准备 uid
    SERVER_RECORED              = 2021,         //记牌器信息, 格式从3到大王，各个牌的数量 {info:[0,0,0,...]}
    SERVER_TAX                  = 2022,         //扣除台费  tax:台费, 各人的当前金币{uid, money} 
    SERVER_FEE                  = 2023,         //可领取奖励 type类型(0 金币,1 话费券), num 数量
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
    CODE_CARD_EXIST             = 19,           //手牌不存在
    CODE_RECORED                = 20,           //记牌器不存在
};  

//游戏阶段
enum STATE
{
    STATE_NULL                  = 0,
    STATE_PREPARE               = 1,            //准备
    STATE_CALL                  = 2,            //叫分
    STATE_DOUBLE                = 3,            //加倍(废)
    STATE_OUT                   = 4,            //出牌
    STATE_END                   = 5,            //结算
    STATE_MAX                   = 6,            //
};

static const char* DESC_STATE[STATE_MAX] = 
{
    "STATE_NULL",
    "STATE_PREPARE",
    "STATE_CALL",
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
    OP_CALL_WAIT                   = 3,            //等待叫分通知
    OP_CALL_NOTIFY                 = 4,            //已通知
    OP_CALL_RECEIVE                = 5,            //已经叫分
    OP_DOUBLE_NOTIFY               = 6,            //已经通知(废)
    OP_DOUBLE_RECEIVE              = 7,            //已经响应(废)
    OP_OUT_WAIT                    = 8,            //等待出牌
    OP_GAME_END                    = 9,            //结算中
    OP_MAX                         = 10,           //MAX
};

static const char* DESC_OP[OP_MAX] = 
{
    "OP_NULL",
    "OP_PREPARE_WAIT",
    "OP_PREPARE_REDAY",
    "OP_CALL_WAIT",
    "OP_CALL_NOTIFY",
    "OP_CALL_RECEIVE",
    "OP_DOUBLE_NOTIFY",
    "OP_DOUBLE_RECEIVE",
    "OP_OUT_WAIT",
    "OP_GAME_END"
};

#endif
