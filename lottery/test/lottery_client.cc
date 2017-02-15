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

#include "XtGameClient.h"
#include "XtJsonPacket.h"
#include "protocol/XtMsgProtocol.h"
#include "XtNetMacros.h"
#include "XtBuffer.h"

#include "XtLog.h"
#include "proto.h"




class BetClient:public XtGameClient 
{
	public:
		BetClient(struct ev_loop* loop)
			:XtGameClient(loop,NULL)
		{
			XtMsgProtocol* protocol=new XtMsgProtocol();

			protocol->setXorEnabled(true);
			char key[]={13,0};
			protocol->setXorKey(key);

			setProtocol(protocol);
		}

		~BetClient()
		{

		}


	public:
		virtual void onRecivePacket(XtBuffer** buffer, int num)
		{
			printf("recive data");

			for(int i=0;i<num;i++)
			{
				XtJsonPacket* packet=new XtJsonPacket;

				int ret=packet->parse((char*)buffer[i]->dpos(),buffer[i]->nbytes());

				printf("recive data from server:%s\n",packet->m_str.c_str());


				int bet_nu=rand()%2+1;
				int card_type=rand()%6+1;

				if(packet->m_val["cmd"]==SERVER_ACTION_UPDATE)
				{
					if(rand()%2==0)
					{
						XtJsonPacket send_packet;
						send_packet.m_val["cmd"]=CLIENT_LOTTERY_BETTING;
						send_packet.m_val["card_type"]=card_type;
						send_packet.m_val["bet_nu"]=bet_nu;
						send_packet.pack();
						this->send(send_packet.m_str.c_str(),send_packet.m_str.length());
					}
				}
			}

		}
		virtual void onClose() 
		{
			fprintf(stderr,"client close\n");
			exit(0);
		}


};





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


	BetClient* client=new BetClient(loop);
	client->connectStart(socket_fd);


	int uid=atoi(argv[3]);

	XtJsonPacket packet;

	packet.m_val["cmd"]=CLIENT_LOTTERY_LOGIN;

	packet.m_val["uid"]=uid;
	packet.pack();


	client->send(packet.m_str.c_str(),packet.m_str.length());


	XtJsonPacket action_packet;

	action_packet.m_val["cmd"]=CLIENT_LOTTERY_ACTION;
	action_packet.m_val["action"]=1;
	action_packet.pack();

	client->send(action_packet.m_str.c_str(),action_packet.m_str.length());



	ev_loop(loop,0);
	ev_loop_destroy(loop);
	delete client;

	return 0;
}





