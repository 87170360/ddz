#ifndef __LOTTERY_PROTO_H__
#define __LOTTERY_PROTO_H__

enum client_command {
	CLIENT_LOTTERY_LOGIN = 1001,  /* lottery login */
	CLIENT_LOTTERY_ACTION = 1002, /* lottery action */
	CLIENT_LOTTERY_BETTING = 1003, /* lottery betting */
};

enum server_command 
{
	SERVER_LOGIN_SUCC = 4001, /* player login succ */
	SERVER_LOGIN_ERR = 4002, /* player login err */

	SERVER_ACTION_SUCC = 4003,
	SERVER_ACTION_ERR = 4004,

	SERVER_BETTING_SUCC = 4005,
	SERVER_BETTING_ERR = 4006,

	SERVER_ACTION_UPDATE = 4007,
	SERVER_LOTTERY_STOP = 4008,
	SERVER_LOTTERY_START = 4009,
	SERVER_LOTTERY_WIN = 4010,

};


#endif
