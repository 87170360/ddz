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


#include "Robots.h"


///机器人服务入口
int main(int argc,char** argv)
{
	int ret;

	Robots* robots=new Robots();
	ret=robots->parseArg(argc,argv);

    if (ret < 0) 
	{
        printf("File: %s Func: %s Line: %d => parse_arg.\n",
                            __FILE__, __FUNCTION__, __LINE__);
        exit(1);
    }

	robots->mainLoop();

	srand(getpid());    ///---设置随机种子

	printf("%s exit\n",argv[0]);

	return 0;
}

