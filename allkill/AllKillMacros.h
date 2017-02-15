#ifndef _ALL_KILL_MACROS_H_
#define _ALL_KILL_MACROS_H_ 


// 玩家发言最短间隔(以防恶意刷屏)
#define SPEAK_MIN_INTERVAL   5 
// 最短发言持续次数
#define SPEAK_MIN_COUNT      10

enum E_AllKillGameStatus
{
	AK_Ready   = 0,
	AK_Start   = 1,
	AK_EndGame = 2
};

//上行指令
enum E_AllKillClientCmd
{
	AK_LOGIN_C         = 3000,
	AK_LOGOUT_C        = 3001,
	AK_SIT_DOWN_C      = 3002,
	AK_STAND_UP_C      = 3003,
	AK_PLAYER_BET_C    = 3004,
	AK_ASK_ROLE_C      = 3005,
	AK_ASK_UN_ROLE_C   = 3006,
	AK_CHAT_C          = 3007,
	AK_PLAYER_INFO_C   = 3008,
	AK_FACE_C          = 3009,
	AK_LOTTERY_FIRST_C = 3010,
	AK_BET_LOTTERY_C   = 3011,
};

enum E_AllKillServerCmd 
{	
	AK_LOGIN_SU                = 6000,
	AK_LOGIN_SUCC_SB           = 6001,
	AK_GAME_INFO_SU            = 6002,
    AK_SIT_DOWN_SU             = 6003,
	AK_SIT_DOWN_SB             = 6004,
	AK_STAND_UP_SU             = 6005,
	AK_STAND_UP_SB             = 6006,
	AK_LOGOUT_SU               = 6007,
	//AK_LOGOUT_ERR_SU           = 6008,	
	AK_GAME_READY_SB           = 6009,
	AK_GAME_START_SB           = 6010,
	AK_GAME_END_SB             = 6011,
	AK_GAME_UPDATE_SB          = 6012,	
	AK_PLAYER_BET_RESULT_SU    = 6013,	
	AK_ASK_ROLE_RESULT_SU      = 6014,	
	AK_ASK_UN_ROLE_RESULT_SU   = 6015,
	AK_ASK_ROLE_LIST_CHANGE_SB = 6016,
	AK_PLAYER_BET_REWARD_SU    = 6017,	
	AK_CHAT_SB                 = 6018,
    AK_PLAYER_INFO_SUCC_SU     = 6019,
	AK_PLAYER_INFO_ERR_SU      = 6020,
	AK_PLAYER_BET_SB           = 6021,
	AK_FACE_SB                 = 6022,
	AK_LOGOUT_SB               = 6023,
	AK_LOTTERY_FIRST_SU        = 6024,
	AK_BET_LOTTERY_SU          = 6025,

	AK_ASK_SERVER_SHUT_DOWN    = -1,
};


enum E_AllKillGameErr 
{
	AK_BET_NU_ERR       = -1,   // 下注金额错误
	AK_STATUS_ERR       = -2,   // 游戏状态错误
	AK_SEAT_ID_ERR      = -3,   // 位置ID错误
	AK_MONEY_NOT_ENOUGH = -4,   // 金币不够
	AK_NOT_IN_ROLE_LIST = -5,   // 不在上庄列表
	AK_ROLE_BET_ERR     = -6,
};


/* ak seat info */
#define AK_SEAT_ID_START 1
#define AK_SEAT_ID_END	 4
#define AK_SEAT_ID_NU    4

/* ak game timer */
#define AK_READY_TIME       1        // 准备时间
#define AK_START_TIME       15       // 开始时间
#define AK_END_GAME_TIME    10       // 结束时间

#define AK_UPDATE_TIME      1           // 更新时间
#define AK_LOTTERY_OPEN_TIME 5          // 开奖时间
#define AK_SET_NEXT_ROLE_BEFORE_TIME 3  // 设置下一庄家时间


#define VALID_CLIENT_UID_MIN 1000        //无效uid最小值 


/* default role info */
#define AK_DEFAULT_ROLE_NAME "系统女王"
#define AK_DEFAULT_ROLE_MONEY 99999999
#define AK_DEFAULT_ROLE_AVATAR "image_0.png"
#define AK_DEFAULT_ROLE_UID 0




#define AK_BET_FLOW_ID    301      // 下注流水标识
#define AK_ROTTLE_FLOW_ID 302      // 开奖流水标识


#define AK_HAND_CARD_COUNT 5       //5手牌 （1庄家 + 4位置）

#endif /*_ALL_KILL_MACROS_H_*/

