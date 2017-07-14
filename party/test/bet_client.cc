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

static int g_bet_nu[] = {50,60,70,80,90,100};
static int g_bet_nu_size = sizeof(g_bet_nu) / sizeof(int);

static void server_close(AllKillClient* client,void* data)
{
	fprintf(stderr,"client close\n");
	exit(0);
}


static void recivie_data(AllKillClient* client,void* data,Jpacket& packet)
{

	printf("recive data from server:%s\n",packet.val.toStyledString().c_str());

	int seat_id=AK_SEAT_ID_START+rand()%(AK_SEAT_ID_NU);
	int bet_nu = g_bet_nu[rand() % g_bet_nu_size];

	if(packet.val["cmd"]==AK_GAME_UPDATE_SB)
	{
		if(rand()%5==0)
		{
			Jpacket packet;
			packet.val["cmd"]=AK_PLAYER_BET_C;
			packet.val["seat_id"]=seat_id;
			packet.val["bet_nu"]=bet_nu;
			packet.end();
			client->send(packet.tostring());
		}
	}
}



int main(int argc,char** argv)
{
	if(argc != 4)
	{
		printf("useage %s <ip> <port> <uid>\n",argv[0]);
		exit(0);
	}

	srand(time(NULL)+getpid());

	int socket_fd;
	struct sockaddr_in serv_addr;

	socket_fd=socket(AF_INET,SOCK_STREAM,0);
	if(socket_fd==-1)
	{
		printf("create Socket failed\n");
		return -1;
	}

	char* ip=argv[1];
	int port=atoi(argv[2]);

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(port);
	serv_addr.sin_addr.s_addr=inet_addr(ip);

	memset(&serv_addr.sin_zero,0,8);

	if(connect(socket_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))==-1)
	{
		printf("connect to server failed\n");
		return -1;
	}

	struct ev_loop* loop=ev_loop_new(0);


	AllKillClient* client=new AllKillClient(loop);
	client->setOnReciveCmdFunc(recivie_data,NULL);
	client->setOnCloseFunc(server_close,NULL);
	client->connectStart(socket_fd);


	int uid=atoi(argv[3]);
	Jpacket packet;
	if(uid==-1)
	{
		packet.val["cmd"]=AK_ASK_SERVER_SHUT_DOWN;
	}
	else 
	{
		packet.val["cmd"]=AK_LOGIN_C;
	}
	packet.val["uid"]=uid;
	packet.end();
	client->send(packet.tostring());




	ev_loop(loop,0);
	ev_loop_destroy(loop);
	delete client;

	return 0;
}


