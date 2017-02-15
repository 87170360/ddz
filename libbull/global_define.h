#ifndef _GLOBAL_DEFINE_
#define _GLOBAL_DEFINE_


//数据类型定义
typedef          char        _tint8;     //有符号 1 字节
typedef unsigned char        _uint8;     //无符号 1 字节
typedef short                _tint16;    //有符号 2 字节
typedef unsigned short       _uint16;    //无符号 2 字节
typedef int                  _tint32;    //有符号 4 字节
typedef unsigned int         _uint32;    //无符号 4 字节
typedef long long            _tint64;    //有符号 8 字节
typedef unsigned long long   _uint64;    //无符号 8 字节
typedef unsigned int         socket_id;  //通信标识   4 字节
typedef _uint8               _svrtype;   //服务器类型 1 字节
typedef _uint64              _svrid;     //服务器ID   8 字节


#define	INVALID_BANKER			0xFF				//无效庄家ID
#define	INVALID_DOUBLE			0					//无操作加倍
#define	INVALID_SECOND_DOUBLE	0xFF				//无操作二次加倍
#define	INVALID_SWITCH			0xFF				//无操作换牌
#define	INVALID_GER_BANK		0xFF				//无操作抢庄
#define	INVALID_USER_ID			0					//无效用户ID

#define	MAX_CHAIR_COUNT			5					//最大椅子数
#define	GAME_PLAYER_COUNT		5					//游戏人数
#define	MAX_DOUBLE_CHOICE		4					//最多加倍选项
#define	MAX_CARD_TYPE			13					//最大牌类型
#define	MAX_CHOICE_CARD			3					//最大选牌数

#define	MAX_HAND_CARD_COUNT		5					//8最大手牌张数
#define	MAX_TABLE_CARD_COUNT	5					//桌面最大牌数目
#define	MAX_SWITCH_CARD_COUNT	5					//最大换牌数
#define	MAX_CHOICE_CARD			3					//选牌数目

#define	MAX_BAIREN_BET_NUM		4					//百人最大下注选项
#define	MAX_GET_BANKER_NUM		10					//抢庄列表
#define	MAX_BAIREN_CARD_COUNT	5					//手牌数
#define	MAX_RECORD_NUM			20					//下注记录



//坐位状态
enum enChairStatus   
{
	STATUS_NULL    = 0,     //空位	
	STATUS_LOOK_ON = 1,     //旁观(入座)
	STATUS_IN_GAME = 2,     //游戏中
	STATUS_STAND_UP= 3,     //站起退出
};

enum enRole
{
	ROLE_NULL=0,    //闲家
	ROLE_BANKER=1,  //庄家

};

//结束类型
enum enEndKind
{
	KIND_NULL = 0,  //无
	KIND_WIN  = 1,  //胜
	KING_LOSE = 2,  //输
};


#endif



