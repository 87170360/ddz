#ifndef __PROTO_H__
#define __PROTO_H__

enum system_command {
	SYS_ECHO = 0001, /* echo */
	SYS_HEART_BIT_CLI = 0002, /* heart bit from client */
	SYS_HEART_BIT_SVR = 0003, /* heart bit to client */
	SYS_KICK_PLAYER = 0004, /* kick player out of table */
	SYS_CLOSE_TABLE = 0005, /* close a table */
	SYS_STOP_SVR = 0006, /* stop a svr */
	SYS_ONLINE = 0007, /* get online */
};

enum client_command {
	CLIENT_LOGIN_REQUEST = 1001, /* player login */
	CLIENT_CHAT = 1002, /* player chat command */
	CLIENT_UPDATE_INFO = 1003,   /* update player info*/
};

enum server_command {
	SERVER_LOGIN_SUCC = 4001, /* player login succ */
	SERVER_LOGIN_ERR = 4002, /* player login err */

	SERVER_CHAT = 4003, /* player chat broadcast */
	SERVER_CHAT_SUCC = 4004, /* player chat broadcast succ*/
	SERVER_CHAT_ERR = 4005, /* player chat broadcast err*/
	SERVER_SYSTEM_CHAT = 4006,   /* system broadcast message */
	SERVER_SYSTEM_UC = 4007,     /* system unicast message */
	SERVER_SYSTEM_ACHI = 4008,  /* system unicast message achievement */
	SERVER_SYSTEM_UC_CHAT = 4009, /* system unicast message */
};

enum system_error_command {
	CHAT_LACK_MONEY = 8001, /* chat need money */
};

enum login_error_command {
	LOGIN_PROTOCOL_ERR = 8001, /* protocol error */
	LOGIN_TABLE_NONEXIST = 8002, /* no such table exist */
	LOGIN_PLAYER_KEY_ERR = 8003, /* player key error */
	LOGIN_TABLE_FULL = 8004, /* table has max players */
	LOGIN_TOO_MUCH_MONEY = 8005, /* out of money */
	LOGIN_LACK_MONEY = 8006, /* need money */
	LOGIN_PLAYER_EXIST = 8007, /* player has been existed */
	LOGIN_NO_FREE_TABLE = 8008, /* no free table */
};

#endif
