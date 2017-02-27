#ifndef __PROTO_H__
#define __PROTO_H__

enum client_command
{
    CLIENT_LOGIN                = 1001,   
    CLIENT_LOGOUT               = 1002,
    CLIENT_READY                = 1003,	 
};

enum server_command
{
    SERVER_RESPOND              = 2000,         //客户端请求回复
    SERVER_LOGIN                = 2001,
    SERVER_LOGOUT               = 2002,
};

#endif
