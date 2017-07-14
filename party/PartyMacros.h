#ifndef _PARTY_MACROS_H_
#define _PARTY_MACROS_H_ 

//timer
const int PARTY_TIMER_START            = 30;
const int PARTY_TIMER_END              = 30;
const int PARTY_TIMER_PREPARE          = 5;

const int PARTY_TIMER_UPDATE           = 1;
//history num
const int HISTORYNUM                   = 8;

static double g_party_ming       = 300000000;
static double g_party_ming2      = g_party_ming * 2;
static double g_party_an         = 5000000000;
static double g_party_an2        = g_party_an * 2;
//税率，小于1
const static double g_tax              = 0.08;            
//排行榜数量
const static int    g_topNum           = 6;
#define VALID_CLIENT_UID_MIN 10000
//罗盘格子数
const static int    g_grid             = 24;

const static int    g_second_hour      = 3600;

enum PARTY_CMD
{
    PA_LOGIN_C      = 1001, //登录
    PA_START_B      = 1002, //开始
    PA_END_B        = 1003, //结束(全体)
    PA_LOGIN_S      = 4000, 
    PA_BET_C        = 1005, //下注
    PA_BET_S        = 1006,
    PA_UPDATE_B     = 1007, //更新 
	PA_CHAT_C       = 1008, //聊天
	PA_CHAT_B       = 1009,
    PA_LOGOUT_C     = 1010, //登出
    PA_LOGOUT_S     = 1011,
    PA_CANCLE_C     = 1012, //撤注
    PA_CANCLE_S     = 1013,
    PA_FOLLOW_C     = 1014, //跟注
    PA_FOLLOW_S     = 1015,
    PA_AWARD_B      = 1016, //玩家奖励
    PA_AUTO_C       = 1017, //自动续押
    PA_AUTO_S       = 1018,
    PA_PREPARE_B    = 1019, //准备
};

enum PARTY_STATE
{
	STATE_START     =   1,
	STATE_END       =   2,
	STATE_PREPARE   =   3,
};

enum PARTY_AWARD_ID
{
    AWARD_MIN       = -1,        //top
    AWARD_1         = 0,         //红兔子
    AWARD_2         = 1,         //黄兔子
    AWARD_3         = 2,         //绿兔子
    AWARD_4         = 3,         //红猴子
    AWARD_5         = 4,         //黄猴子
    AWARD_6         = 5,         //绿猴子
    AWARD_7         = 6,         //红熊猫
    AWARD_8         = 7,         //黄熊猫
    AWARD_9         = 8,         //绿熊猫
    AWARD_10        = 9,         //红狮子
    AWARD_11        = 10,        //黄狮子
    AWARD_12        = 11,        //绿狮子
    AWARD_13_1      = 12,        //兔子
    AWARD_13_2      = 13,        //猴子
    AWARD_13_3      = 14,        //熊猫
    AWARD_13_4      = 15,        //狮子
    AWARD_14_1      = 16,        //红色
    AWARD_14_2      = 17,        //黄色
    AWARD_14_3      = 18,        //绿色
    AWARD_MAX       = 19,        //botton
};


enum AWARD_TYPE
{
    SINGLE          = 0,         //单项
    THREE           = 1,         //大三元
    FOUR            = 2,         //大四喜
    LAMP            = 3,         //送灯
};

enum ANIMAL_TYPE
{
    RABBIT          = 0,         //兔子
    MONKEY          = 1,         //猴子
    PANDA           = 2,         //熊猫
    LION            = 3,         //狮子
};

enum COLOR_TYPE
{
    RED             = 0,
    YELLOW          = 1,
    GREEN           = 2,
};

enum PARTY_ODDS_TYPE
{
    ODD_MIN         = -1,
    ODD_1           = 0,
    ODD_2           = 1,
    ODD_3           = 2,
    ODD_MAX         = 3,
};

enum PARTY_WEIGHT_TYPE
{
    WEIGHT_1        = 0,
    WEIGHT_2        = 1,
    WEIGHT_3        = 2,
    WEIGHT_MAX      = 3,
};

enum PARTY_RETURN
{
   RET_SUCCESS      = 0, 
   RET_LACKMONEY    = 1,
   RET_STATE        = 2,
   RET_NOBET        = 3,
   RET_NEGATIVE     = 4, 
   RET_BET          = 5,        //下注类型错误
   RET_REPEAT       = 6,        //重复跟注 
   RET_LIMIT        = 7,        //单局下注上限
};

#endif /*_PARTY_MACROS_H_*/
