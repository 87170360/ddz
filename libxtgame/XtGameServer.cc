#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>


#include "XtBuffer.h"
#include "XtNetMacros.h"
#include "XtGameServer.h"
#include "XtGamePlayer.h"
#include "XtGameClient.h"
#include "XtGameLogic.h"
#include "XtRedisClient.h"
#include "XtRedisSubscribe.h"
#include "XtJsonPacket.h"
#include "XtSqlClient.h"
#include "daemonize.h"

#include "XtLog.h"


static void XtGameServer_onClientConnect(struct ev_loop* loop,struct ev_io* w,int revents)
{
	if(EV_ERROR& revents)
	{
		XT_LOG_ERROR("got invalid event\n");
		return;
	}

	XtGameServer*  server=(XtGameServer*) w->data;
	server->clientConnect();
}








XtGameServer::XtGameServer()
{
	m_evLoop=NULL;

	m_start=false;
	m_listenFd=-1;


	m_game=NULL;

	m_cacheRc=NULL;
	m_logRc=NULL;
}



XtGameServer::~XtGameServer()
{

}



///接受连接
void XtGameServer::clientConnect()
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int fd = accept(m_listenFd, (struct sockaddr *) &client_addr, &client_len);
	if (fd < 0) 
	{
		XT_LOG_ERROR("accept error[%s]\n", strerror(errno));
		return;
	}

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);



	XtGameClient* client = onCreateClient();
	client->connectStart(fd);



	addClient(client);
	XT_LOG_INFO("client fd=%d connect\n",fd);

}

int XtGameServer::start(struct ev_loop* loop,const std::string& conf_file)
{
	m_evLoop=loop;
	if(this->parseConfig(conf_file)<0)
	{
		printf("parseConfig failed\n");
		return -1;
	}
	else 
	{
		printf("parseConfig success\n");
	}

	if(this->startLog()<0)
	{
		printf("start log error\n");
		return -1;
	}
	else 
	{
		printf("start log success\n");
	}



	if(this->singleInstance()<0)
	{
		printf("single_instance_running falied\n");
		return -1;
	}
	else 
	{
		printf("single_instance_running success\n");
	}

	if(this->startRedis()<0)
	{
		printf("start redis failed\n");
		return -1;
	}
	else 
	{
		printf("start redis success\n");

	}

	if(this->startSql()<0)
	{
		printf("start sql failed\n");
		return -1;
	}
	else 
	{
		printf("start sql success\n");
	}


	if(this->startListen()<0)
	{
		printf("start listen falied\n");
		return -1;
	}
	else 
	{
		printf("start listen success\n");
	}

	if(this->startGame()<0)
	{
		printf("start game failed\n");
	}
	else 
	{
		printf("start game success\n");
	}
	m_start=true;

	return 0;
}





int XtGameServer::parseConfig(const std::string& conf_file)
{
	std::ifstream in(conf_file.c_str(), std::ifstream::binary); 
	if (!in) 
	{
		printf("init file no found.\n");
		return -1;
	}

	Json::Reader reader;
	bool ret = reader.parse(in, m_conf);
	if (!ret) 
	{
		in.close();
		printf("init file parser.\n");
		return -1;
	}
	in.close();
	return 0;
}

int XtGameServer::singleInstance()
{
	int ret=0;
	int pid=getpid();

	char buf[1024];

	sprintf(buf,"pid/%d.pid",pid);

	ret=single_instance_running(m_conf.get("pid_file",buf).asString().c_str());

	return ret;
}

int XtGameServer::startLog()
{
	int pid=getpid();
	char buf[1024];
	sprintf(buf,"log/%d.log",pid);

	XtLog::getDefaultLog()->start(m_conf["log"].get("log_file", buf).asString(),
			m_conf["log"].get("level", 4).asInt(),
			m_conf["log"].get("console", 0).asInt(),
			m_conf["log"].get("rotate", 1).asInt(),
			m_conf["log"].get("max_size", 1073741824).asInt(),
			m_conf["log"].get("max_file", 50).asInt());

	return 0;
}

int XtGameServer::startRedis()
{

	int ret;

	int size= m_conf["main-db"].size();
	m_mainsRc.resize(size);

	for (int i = 0; i < size; i++)
	{
		m_mainsRc[i] = new XtRedisClient();
		m_mainsRc[i]->init(
				m_conf["main-db"][i]["host"].asString(), 
				m_conf["main-db"][i]["port"].asInt(), 
				1000, 
				m_conf["main-db"][i]["pass"].asString());

		ret=m_mainsRc[i]->connectRedis();

		if (ret < 0)
		{
			XT_LOG_ERROR("main db redis error\n");
			return -1;      
		}
	}


	m_cacheRc=new XtRedisClient();

	m_cacheRc->init(
			m_conf["cache-db"]["host"].asString(),
			m_conf["cache-db"]["port"].asInt(), 
			1000, 
			m_conf["cache-db"]["pass"].asString());

	ret=m_cacheRc->connectRedis();

	if(ret<0)
	{
		XT_LOG_ERROR("cache db redis error\n");
		return -1;
	}

	m_speakerRc=new XtRedisClient();
	m_speakerRc->init(
			m_conf["speaker-db"]["host"].asString(),
			m_conf["speaker-db"]["port"].asInt(),
			1000,
			m_conf["speaker-db"]["pass"].asString());

	ret=m_speakerRc->connectRedis();
	if(ret<0)
	{
		XT_LOG_ERROR("speaker db redis error");
		return -1;
	}
	return 0;
}



///---启动监听服务
int XtGameServer::startListen()
{

	int fd;
	XT_LOG_INFO("Listening on %s:%d\n",
			m_conf["game"]["host"].asString().c_str(),
			m_conf["game"]["port"].asInt());



	struct sockaddr_in addr;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) 
	{
		XT_LOG_ERROR("socket failed: %s\n",strerror(errno));
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_conf["game"]["port"].asInt());
	addr.sin_addr.s_addr = inet_addr(m_conf["game"]["host"].asString().c_str());
	if (addr.sin_addr.s_addr == INADDR_NONE) 
	{
		XT_LOG_ERROR("game::init_accept Incorrect ip address!\n");
		close(fd);
		return -1;
	}



	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) 
	{
		XT_LOG_ERROR("setsockopt failed: %s\n",  strerror(errno));
		close(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) 
	{
		XT_LOG_ERROR("bind failed: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
	listen(fd, 100);

	m_listenFd=fd;
	m_evAccept.data = this;
	ev_io_init(&m_evAccept,XtGameServer_onClientConnect, fd, EV_READ);   ///设置读回调
	ev_io_start(m_evLoop,&m_evAccept);


	XT_LOG_INFO("listen ok\n");

	return 0;

}

int XtGameServer::startGame()
{
	m_game=onCreateGame();
	m_game->configGame(&m_conf);
	m_game->start(this,m_evLoop);

	return 0;
}


void XtGameServer::unicast(XtGamePlayer* player,XtJsonPacket* packet)
{
	int uid=player->getUid();

	std::map<int,XtGameClient*>::iterator iter =m_loginClient.find(uid);
	if(iter!=m_loginClient.end())
	{
		XtGameClient* client=iter->second;
		client->send(packet->m_str.c_str(),packet->m_str.length());
	}
	else 
	{
		XT_LOG_WARN("player(%d) if offline now\n",uid);
	}
}


void XtGameServer::broadcast(XtGamePlayer* player,XtJsonPacket* packet)
{
	int uid=-1;
	if(player)
	{
		uid=player->getUid();
	}

	for(std::map<int,XtGameClient*>::iterator iter=m_loginClient.begin();iter!=m_loginClient.end();++iter)
	{
		if(iter->first!=uid)
		{
			iter->second->send(packet->m_str.c_str(),packet->m_str.length());
		}
	}
}






void XtGameServer::onReciveClientCmd(XtGameClient* client,XtBuffer* buffer)
{
	if(!m_start)
	{
		XT_LOG_WARN("server closed\n");
		return;
	}
	XtJsonPacket packet;
	int ret=packet.parse((char*)buffer->dpos(),buffer->nbytes());

	XT_LOG_INFO("recive client(%d) data(%d):%s\n",client->getClientFd(),packet.m_str.length(),packet.m_str.c_str());

	if(ret<0)
	{
		XT_LOG_ERROR("client(%d,%d) send error packet.\n",client->getClientFd(),client->getUid());
		client->closeConnect();
		return;
	}


	int fd=client->getClientFd();
	if(!packet.m_val["cmd"].isNumeric())
	{
		XT_LOG_ERROR("client(%d) cmd not numberic\n",fd);
		client->closeConnect();
		return;
	}


	int cmd=packet.m_val["cmd"].asInt();

	XtGamePlayer* player=(XtGamePlayer*) client->getPlayer();

	if(cmd==XT_CMD_LOGIN_C)
	{
		if(player)
		{
			XT_LOG_ERROR("client already login uid=%d\n",player->getUid());
			client->closeConnect();
			return;
		}

		handleClientLogin(client,&packet);
		return;
	}

	if(cmd==XT_CMD_LOGOUT_C)
	{
		handleClientLogout(client,&packet);
		return;
	}




#ifdef  XT_DEBUG 
	if(cmd==XT_CMD_SERVER_SHUT_DOWN)
	{
		shutDown();
		return;
	}
#endif /*XT_DEBUG*/



	if(player==NULL)
	{
		XT_LOG_ERROR("client(%d) not login error cmd(%d)\n",fd,cmd);
		return ;
	}

	m_game->onReciveClientCmd(player,&packet);
}



int XtGameServer::shutDown()
{
	closeClient();
	closeGame();
	closeListen();
	closeRedis();
	closeLog();
	m_start=false;
	return 0;
}


int XtGameServer::closeClient()
{
	std::map<int,XtGameClient*>::iterator iter;

	for(iter=m_allClient.begin();iter!=m_allClient.end();++iter)
	{
		delete iter->second ;
	}

	m_allClient.clear();
	m_loginClient.clear();

	return 0;
}



int XtGameServer::closeGame()
{
	m_game->shutDown();
	delete m_game;
	m_game=NULL;
	return 0;
}




int XtGameServer::closeListen()
{
	ev_io_stop(m_evLoop,&m_evAccept);
	close(m_listenFd);
	return 0;
}

int XtGameServer::closeRedis()
{
	int main_size=m_mainsRc.size();
	for(int i=0;i<main_size;i++)
	{
		if(m_mainsRc[i])
		{
			m_mainsRc[i]->deinit();
			delete m_mainsRc[i];
		}
	}


	if(m_cacheRc)
	{
		m_cacheRc->deinit();
		delete m_cacheRc;
	}

	if(m_logRc)
	{
		m_logRc->deinit();
		delete m_logRc;
	}

	return 0;
}

int XtGameServer::closeLog()
{
	XtLog::getDefaultLog()->stop();
	XtLog::deleteDefaultLog();
	return 0;
}



void XtGameServer::addClient(XtGameClient* client)
{

	m_allClient[client->getClientFd()]=client;

}

void XtGameServer::removeClient(XtGameClient* client)
{
	int uid=client->getUid();
	XtGamePlayer* player=(XtGamePlayer*)client->getPlayer();

	if(uid>0)
	{
		assert(player);
		player->setClient(NULL);

		m_game->playerLogout(player);
		m_loginClient.erase(uid);
	}

	int fd=client->getClientFd();
	m_allClient.erase(fd);

	delete client;

}


XtRedisClient* XtGameServer::getDataRedis(int uid)
{
	return  m_mainsRc[uid%m_mainsRc.size()];
}

XtRedisClient* XtGameServer::getCacheRedis()
{
	return m_cacheRc;
}


void XtGameServer::handleClientLogin(XtGameClient* client,XtJsonPacket* packet)
{


	int uid=packet->m_val["uid"].asInt();
	std::string skey=packet->m_val["skey"].asString();

	if(uid<0)
	{
		XT_LOG_ERROR("checkout uid(%d) error\n",uid);
		client->closeConnect();
		return;
	}


	int i=uid%m_mainsRc.size();
	int ret=m_mainsRc[i]->doCommand("hget u:%d skey",uid);

	if(ret<0)
	{
		XT_LOG_ERROR("check uid(%d) skey error\n",uid);
		client->closeConnect();
		return;
	}


	if(uid>XT_VALID_CLIENT_UID_MIN)
	{
		if(m_mainsRc[i]->getReply()->str&&skey.compare(m_mainsRc[i]->getReply()->str)!=0)
		{
			XT_LOG_ERROR("check skey error,client[%s],server[%s]\n",skey.c_str(), m_mainsRc[i]->getReply()->str);
			client->closeConnect();
			return;
		}
	}



	std::map<int,XtGameClient*>::iterator  iter=m_loginClient.find(uid);

	if(iter!=m_loginClient.end())
	{
		XT_LOG_ERROR("two client login one uid\n");
		removeClient(iter->second);
	}

	client->setUid(uid);

	XtGamePlayer* player=m_game->getPlayer(uid);
	client->setPlayer(player);

	m_loginClient[uid]=client;

	m_game->playerLogin(player);

}

void XtGameServer::handleClientLogout(XtGameClient* client,XtJsonPacket* packet)
{
	removeClient(client);
}


void XtGameServer::sendFlow(int flow_id,int vid,int uid,int alter_type,int alter_value,int cur_value)
{

	if(uid<XT_VALID_CLIENT_UID_MIN)
	{
		return;
	}

	time_t ts=time(NULL);	

	char buf[1024];
	sprintf(buf,"insert flow(uid,type,vid,alter_type,alter_value,current_value,ts) value(%d,%d,%d,%d,%d,%d,%ld)",uid,flow_id,vid,alter_type,alter_value,cur_value,ts);

	//printf("flowsql:%s\n",buf);

	int ret=m_sqlClient->query(buf);
	if(ret<0)
	{
		XT_LOG_ERROR("send bet flow failed uid(%d) alert_value(%d) cur_value(%d).\n",uid,alter_value,cur_value);
	}

}

void XtGameServer::sendSpeaker(int cmd,int uid,const char* name,const char* content)
{
	XtJsonPacket packet;

	packet.m_val["cmd"]=cmd;
	packet.m_val["uid"]=uid;
	packet.m_val["name"]=name;
	packet.m_val["str"]=content;

	packet.pack();


	m_speakerRc->doCommand("PUBLISH zjh_speaker %s",packet.m_str.c_str());

}



int XtGameServer::startSql()
{
	int ret;
	m_sqlClient=new XtSqlClient();

	ret=m_sqlClient->connect(m_conf["sql-db"]["host"].asString().c_str(),
							m_conf["sql-db"]["port"].asInt(),
							m_conf["sql-db"]["user"].asString().c_str(),
							m_conf["sql-db"]["pass"].asString().c_str(),
							m_conf["sql-db"]["dbname"].asString().c_str());
	if(ret!=0)
	{
		XT_LOG_ERROR("connect sql error\n");
		return -1;
	}

	return 0;
}



















