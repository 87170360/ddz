#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ev.h>

#include "AllKillClient.h"
#include "AllKillMacros.h"
#include "log.h"




Log xt_log;

static int g_uid=0;

static void recivie_data(AllKillClient* client,void* data,Jpacket& packet)
{
	if(packet.val["cmd"]==AK_GAME_UPDATE_SB)
	{
		return;
	}

	if(packet.val["cmd"]==AK_GAME_READY_SB)
	{
		if(packet.val["role"]["uid"].asInt()==g_uid)
		{
			if(rand()%3==0)
			{
				Jpacket packet;
				packet.val["cmd"]=AK_ASK_UN_ROLE_C; //下庄
				packet.end();
				client->send(packet.tostring());
			}
		}
		else 
		{
			if(rand()%3==0)
			{
				Jpacket packet;
				packet.val["cmd"]=AK_ASK_ROLE_C;   //上庄
				packet.end();
				client->send(packet.tostring());
			}
		}
	
	}


	printf("recive data from server:%s\n",packet.str.c_str());
}



// 庄家客户端
int main(int argc,char** argv)
{
	if (argc != 4)
	{
		printf("useage %s <ip> <port> <uid>\n",argv[0]);
		exit(0);
	}


	int socket_fd;
	struct sockaddr_in serv_addr;

	socket_fd = socket(AF_INET,SOCK_STREAM,0);
	if (socket_fd == -1)
	{
		printf("create Socket failed\n");
		return -1;
	}

	char* ip=argv[1];
	int port=atoi(argv[2]);

	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_port        = htons(port);
	serv_addr.sin_addr.s_addr = inet_addr(ip);

	memset(&serv_addr.sin_zero, 0, 8);

	if(connect(socket_fd, (struct sockaddr*)&serv_addr,sizeof(struct sockaddr)) == -1)
	{
		printf("connect to server failed\n");
		return -1;
	}

	struct ev_loop* loop=ev_loop_new(0);


	AllKillClient* client=new AllKillClient(loop);
	client->setOnReciveCmdFunc(recivie_data,NULL);
	client->connectStart(socket_fd);


	int uid = atoi(argv[3]);
	Jpacket packet;
	if (uid == -1)
	{
		packet.val["cmd"]=AK_ASK_SERVER_SHUT_DOWN;
	}
	else 
	{
		packet.val["cmd"] = AK_LOGIN_C; //登录
	}
	packet.val["uid"]=uid;
	packet.end();
	client->send(packet.tostring());


	//上庄请求
	g_uid=uid;
	Jpacket ask_packet;
	ask_packet.val["cmd"]=AK_ASK_ROLE_C;
	ask_packet.end();
	client->send(ask_packet.tostring());


	ev_loop(loop,0);
	ev_loop_destroy(loop);

	delete client;

	return 0;
}


