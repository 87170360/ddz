#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
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


#include "XtBeautyServer.h"
//#include "XtBeautyClient.h"
#include "XtBeautyPlayer.h"

#include "daemonize.h"
#include "XtLog.h"


int main(int argc,char** argv)
{
	int is_daemonize=0;
	std::string conf_file;

	int oc;
	char ic;

	while((oc=getopt(argc,argv,"Df:"))!=-1)
	{
		switch(oc)
		{
			case 'D':
				is_daemonize=1;
				break;
			case 'f':
				conf_file=std::string(optarg);
				break;
			case '?':
				ic=(char) optopt;
				printf("invalid \'%c\'\n",ic);
				break;
			case ':':
				printf("lack option arg\n");
				break;
		}
	}

	if(conf_file=="")
	{
		printf("no config file\n");
		return -1;
	}

	if(is_daemonize)
	{
		printf("begin daemonize\n");
		daemonize();
	}

	signal(SIGPIPE,SIG_IGN);

	struct ev_loop* loop = ev_loop_new(0);




	XtBeautyServer* server=new XtBeautyServer;



	
	if(server->start(loop,conf_file)<0)
	{
		printf("start server failed\n");
		goto error;
	}
	else 
	{
		printf("start server success\n");
	}


	ev_loop(loop,0);

	delete server;

	ev_loop_destroy(loop);



	return 0;

error:
	delete server;
	return -1;

}

















