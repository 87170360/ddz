#include "Robots.h"




int Robots::parseArg(int argc,char** argv)
{
    int flag = 0;
    int oc; /* option chacb. */
    char ic; /* invalid chacb. */
    
    while((oc = getopt(argc, argv, "Df:")) != -1) {
        switch(oc) {
            case 'f':
                flag = 1;
                m_configFileName= std::string(optarg);
                break;
            case '?':
                ic = (char)optopt;
                printf("invalid \'%c\'\n", ic);
                break;
            case ':':
                printf("lack option arg\n");
                break;
        }
    }
    
    if (flag == 0)
        return -1;

    std::ifstream in(m_configFileName.c_str(), std::ifstream::binary); 
    if (!in) 
	{
		std::cout << "init file no found." << std::endl;
		return -1;
	}
	
	Json::Reader reader;
	bool ret = reader.parse(in, m_conf);
	if (!ret) 
	{
		in.close();
		std::cout << "init file parser." << std::endl;
		return -1;
	}
	in.close();
        
    return 0;
}



void Robots::mainLoop()
{
	int uid_start = m_conf["uid_start"].asInt();   // uid开始号
	int uid_num   = m_conf["uid_num"].asInt();     // 机器人个数

	std::string server_ip=m_conf["server"].asString();
	int port=m_conf["port"].asInt();

	printf("uid_start=%d,uid_num=%d,server_ip=%s,port=%d\n",uid_start,uid_num,server_ip.c_str(),port);
	m_evloop=ev_default_loop(0);

	m_robotClient.resize(uid_num);

	for (int i = 0; i < uid_num; i++)
	{
		printf("init robotes client %d\n",i+uid_start);
		XtRobotClient* client=new XtRobotClient(m_evloop);     // 传入事件循环对象
		client->connectToServer( server_ip.c_str(), port, uid_start+i);
		m_robotClient[i]=client;
	}

	ev_loop(m_evloop,0);
}















