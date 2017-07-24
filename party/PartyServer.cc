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

#include "log.h"
#include "daemonize.h"
#include "PartyServer.h"
#include "PartyClient.h"
#include "PartyPlayer.h"
#include "PartyGame.h"
#include "PartyMacros.h"

extern Log xt_log;
static std::map<int, string> g_cmd;

void PartyServer::onClientConnect(struct ev_loop* loop,struct ev_io* w,int revents)
{
	if (EV_ERROR & revents) 
	{
		xt_log.error("got invalid event\n");
		return;
	}

	PartyServer* s=(PartyServer*) w->data;
	s->clientConnect();
}

void PartyServer::onClientClose(PartyClient* client,void* data)
{
	PartyServer* s=(PartyServer*) data;
	s->clientClose(client);
}

void PartyServer::onReciveClientCmd(PartyClient* client,void* data,Jpacket& package)
{
	PartyServer* s=(PartyServer*) data;
	s->reciveClientCmd(client,package);
}

PartyServer::PartyServer()
{
	m_evLoop=NULL;
	m_listenFd=-1;
	m_game=NULL;

	for(int i=0;i<20;i++)
	{
		m_mainRc[i]=NULL;
	}

	m_mainSize=0;
	m_cacheRc=NULL;
	m_sqlClient=NULL;

    g_cmd[PA_START_B]      = "PA_START_B";
    g_cmd[PA_END_B]        = "PA_END_B";
    g_cmd[PA_LOGIN_C]      = "PA_LOGIN_C";
    g_cmd[PA_LOGIN_S]      = "PA_LOGIN_S";
    g_cmd[PA_BET_C]        = "PA_BET_C";
    g_cmd[PA_BET_S]        = "PA_BET_S";
    g_cmd[PA_UPDATE_B]     = "PA_UPDATE_B";
    g_cmd[PA_CHAT_C]       = "PA_CHAT_C";
    g_cmd[PA_CHAT_B]       = "PA_CHAT_B";
    g_cmd[PA_LOGOUT_C]     = "PA_LOGOUT_C";
    g_cmd[PA_LOGOUT_S]     = "PA_LOGOUT_S";
    g_cmd[PA_CANCLE_C]     = "PA_CANCLE_C";
    g_cmd[PA_CANCLE_S]     = "PA_CANCLE_S";
    g_cmd[PA_FOLLOW_C]     = "PA_FOLLOW_C";
    g_cmd[PA_FOLLOW_S]     = "PA_FOLLOW_S";
    g_cmd[PA_AWARD_B]      = "PA_AWARD_B";
    g_cmd[PA_AUTO_S]       = "PA_AUTO_S";
    g_cmd[PA_AUTO_C]       = "PA_AUTO_C";
}

PartyServer::~PartyServer()
{

}

void PartyServer::clientConnect()
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int fd = accept(m_listenFd, (struct sockaddr *) &client_addr, &client_len);
	if (fd < 0) 
	{
		xt_log.error("accept error[%s]\n", strerror(errno));
		return;
	}

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

	PartyClient* client = new (std::nothrow) PartyClient(m_evLoop);

	client->setOnReciveCmdFunc(PartyServer::onReciveClientCmd,this);
	client->setOnCloseFunc(PartyServer::onClientClose,this);

	m_allClient[fd]=client;
	xt_log.info("client fd=%d connect\n",fd);

	client->connectStart(fd);
}

void PartyServer::clientClose(PartyClient* client)
{
	int uid=client->getUid();
	PartyPlayer* player=(PartyPlayer*)client->getUserData();
	if(uid>0)
	{
		m_game->playerLogout(player);
		m_loginClient.erase(uid);
	}

	int fd=client->getClientFd();
	m_allClient.erase(fd);

	delete client;
}

void PartyServer::reciveClientCmd(PartyClient* client,Jpacket& package)
{
	int fd=client->getClientFd();

	if(!package.val["cmd"].isNumeric()) 
	{
		xt_log.error("client(%d) cmd not numberic\n",fd);
		client->closeConnect();
		return;
	}

	int cmd =package.val["cmd"].asInt();
	//xt_log.info("receive %s from client(%d) %s", g_cmd[cmd].c_str(), client->getClientFd(), package.tostring().c_str());

	PartyPlayer* player=(PartyPlayer*)client->getUserData();

	if(cmd == PA_LOGIN_C)
	{
		if(player)
		{
			xt_log.error("client already login uid=%d\n",player->getUid());
			client->closeConnect();
			return;
		}

		clientLogin(client,package);
		return ;
	}

#ifdef AK_DEBUG 
/*
	if(cmd==AK_ASK_SERVER_SHUT_DOWN)
	{
		shutDown();
		return ;
	}
*/
#endif 

	if(player==NULL)
	{
		xt_log.error("client(%d) not login error cmd(%d)\n",fd,cmd);
		return ;
	}

	switch(cmd)
	{
		case PA_BET_C:
			clientBet(client,package);
			break;
        case PA_CANCLE_C:
            {
	            PartyPlayer* player=(PartyPlayer*)client->getUserData();
                m_game->playerCancle(player, package);
            }
            break;
        case PA_FOLLOW_C:
            {
	            PartyPlayer* player=(PartyPlayer*)client->getUserData();
                m_game->playerFollow(player, package);
            }
            break;
		case PA_LOGOUT_C:
			clientLogOut(client, package);
			break;

		case PA_CHAT_C:
			clientChat(client, package);
			break;
		case PA_AUTO_C:
            {
	            PartyPlayer* player=(PartyPlayer*)client->getUserData();
                m_game->playerAuto(player, package);
            }
			break;
		default:
			xt_log.error("client(%d) unkown cmd(%d)\n", fd, cmd);
	}
}

void PartyServer::clientLogin(PartyClient* client,Jpacket& package) 
{
	int uid=package.val["uid"].asInt();
	std::string skey=package.val["skey"].asString();
	if(uid<0)
	{
		xt_log.error("check uid(%d) error\n",uid);
		client->closeConnect();
		return ;
	}

	int i=uid%m_mainSize;
	int ret=m_mainRc[i]->command("hget u:%d skey",uid);
	if(ret<0)
	{
		xt_log.error("check uid(%d) skey error\n",uid);
		client->closeConnect();
		return;
	}

	if(uid<VALID_CLIENT_UID_MIN)
	{
		if(m_mainRc[i]->reply->str&&skey.compare(m_mainRc[i]->reply->str)!=0)
		{
			xt_log.error("check skey error,client[%s],server[%s]\n",skey.c_str(),
					m_mainRc[i]->reply->str);
			client->closeConnect();
			return;
		}
	}

	client->setUid(uid);

	PartyPlayer* player=m_game->getPlayer(uid);
	client->setUserData(player);

	std::map<int,PartyClient*>::iterator client_iter=m_loginClient.find(uid);


	/* if client logined */
	if(client_iter!=m_loginClient.end())
	{
		xt_log.warn("double client uid(%d)\n",uid);
	
		int fd=client_iter->second->getClientFd();
		m_allClient.erase(fd);

		delete client_iter->second;
		m_loginClient.erase(client_iter);
	}


	m_loginClient[uid]=client;
	m_game->playerLogin(player);
}


void PartyServer::clientBet(PartyClient* client,Jpacket& package)
{
	PartyPlayer* player=(PartyPlayer*)client->getUserData();
	m_game->playerBet(player,package);
}

void PartyServer::clientAskRole(PartyClient* client,Jpacket& package)
{
	//PartyPlayer* player=(PartyPlayer*)client->getUserData();
	//m_game->playerAskRole(player,package);
}


void PartyServer::clientUnRole(PartyClient* client,Jpacket& package)
{
	//PartyPlayer* player=(PartyPlayer*)client->getUserData();
	//m_game->playerUnRole(player,package);
}


void PartyServer::clientLogOut(PartyClient* client,Jpacket& package)
{
	clientClose(client);
}

void PartyServer::clientChat(PartyClient* client,Jpacket& package)
{
	PartyPlayer* player=(PartyPlayer*)client->getUserData();

	if (player->checkBanned())
	{
		xt_log.info("client(%d) been banned",client->getClientFd());
		return;
	}
	
	Jpacket packet;
	packet.val["cmd"]           = PA_CHAT_B;
	packet.val["uid"]           = player->getUid();
	//packet.val["avatar"]        = player->getAvatar();
	packet.val["name"]          = player->getName();
	packet.val["sex"]           = player->getSex();
	packet.val["vlevel"]        = player->getVlevel();
	packet.val["content"]       = package.val["content"];
	packet.end();

	broadcast(NULL,packet.tostring());
}

int PartyServer::start(struct ev_loop* loop,const std::string& conf_file)
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


	return 0;
}

int PartyServer::parseConfig(const std::string& conf_file)
{
	std::ifstream in(conf_file.c_str(), std::ifstream::binary); 
	if (!in) 
	{
		std::cout << "init file no found." << endl;
		return -1;
	}

	Json::Reader reader;
	bool ret = reader.parse(in, m_conf);
	if (!ret) 
	{
		in.close();
		std::cout << "init file parser." << endl;
		return -1;
	}

	in.close();
	return 0;
}

int PartyServer::startListen()
{
	int fd;
	xt_log.info("Listening on %s:%d\n",
			m_conf["game"]["host"].asString().c_str(),
			m_conf["game"]["port"].asInt());

	struct sockaddr_in addr;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) 
	{
		xt_log.error("File[%s] Line[%d]: socket failed: %s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_conf["game"]["port"].asInt());
	addr.sin_addr.s_addr = inet_addr(m_conf["game"]["host"].asString().c_str());
	if (addr.sin_addr.s_addr == INADDR_NONE) 
	{
		xt_log.error("game::init_accept Incorrect ip address!");
		close(fd);
		return -1;
	}



	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) 
	{
		xt_log.error("File[%s] Line[%d]: setsockopt failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) 
	{
		xt_log.error("File[%s] Line[%d]: bind failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(fd);
		return -1;
	}

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
	listen(fd, 10000);

	m_listenFd=fd;
	m_evAccept.data = this;
	ev_io_init(&m_evAccept,PartyServer::onClientConnect, fd, EV_READ);
	ev_io_start(m_evLoop,&m_evAccept);


	xt_log.info("listen ok\n");

	return 0;
}

int PartyServer::startGame()
{
	m_game=new PartyGame();
	m_game->configGame(m_conf);
	m_game->start(this,m_evLoop);
	return 0;
}


int PartyServer::singleInstance()
{
	int ret;
	ret=single_instance_running(m_conf.get("pid_file","conf/allkill.pid").asString().c_str());
	return ret;

}



int PartyServer::startLog()
{
	xt_log.start(m_conf["log"].get("log_file", "log/zjhsvr.log").asString(),
			m_conf["log"].get("level", 4).asInt(),
			m_conf["log"].get("console", 0).asInt(),
			m_conf["log"].get("rotate", 1).asInt(),
			m_conf["log"].get("max_size", 1073741824).asInt(),
			m_conf["log"].get("max_file", 50).asInt());

	return 0;
}

int PartyServer::startRedis()
{
	int ret;

	m_mainSize= m_conf["main-db"].size();

	for (int i = 0; i < m_mainSize; i++)
	{
		m_mainRc[i] = new RedisClient();
		ret = m_mainRc[i]->init(m_conf["main-db"][i]["host"].asString()
				, m_conf["main-db"][i]["port"].asInt(), 1000, m_conf["main-db"][i]["pass"].asString());
		if (ret < 0)
		{
			xt_log.error("main db redis error\n");
			return -1;      
		}
	}

	m_cacheRc=new RedisClient();

	ret = m_cacheRc->init(m_conf["cache-db"]["host"].asString(),
			m_conf["cache-db"]["port"].asInt(), 1000, m_conf["cache-db"]["pass"].asString());

	if(ret<0)
	{
		xt_log.error("cache db redis error\n");
		return -1;
	}


	m_speakerRc=new RedisClient();
	ret=m_speakerRc->init(m_conf["speaker-db"]["host"].asString(),
			m_conf["speaker-db"]["port"].asInt(),
			1000,
			m_conf["speaker-db"]["pass"].asString());

	if(ret<0)
	{
		xt_log.error("speaker db redis error");
		return -1;
	}


	return 0;
}





int PartyServer::startSql()
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
		xt_log.error("connect sql error\n");
		return -1;
	}

	return 0;
}



RedisClient* PartyServer::getDataRedis(int uid)
{
	return m_mainRc[uid%m_mainSize];
}


void __xorfunc(std::string &nString)
{
	const int KEY = 13;
	int strLen = (nString.length());
	char *cString = (char*)(nString.c_str());
	
	for (int i = 0; i < strLen; i++)
	{
		*(cString + i) = (*(cString + i) ^ KEY);
	}
}

void PartyServer::unicast(PartyPlayer* player,const std::string& data)
{
	int uid=player->getUid();
	std::map<int,PartyClient*>::iterator iter =m_loginClient.find(uid);
	if(iter!=m_loginClient.end())
	{
		PartyClient* client=iter->second;
		client->send(data);
	}
	else 
	{
		xt_log.error("player(%d) is offline now\n",uid);
	}
}

void PartyServer::broadcast(PartyPlayer* player,const std::string& data)
{
	int uid=-1;
	if(player)
	{
		uid=player->getUid();
	}

	for(std::map<int,PartyClient*>::iterator iter=m_loginClient.begin();iter!=m_loginClient.end();++iter)
	{
		if(iter->first!=uid)
		{
			iter->second->send(data);
		}
	}
}

int PartyServer::shutDown()
{
	closeClient();
	closeGame();
	closeListen();
	closeSql();
	closeRedis();
	closeLog();
	return 0;
}

int PartyServer::closeClient()
{
	std::map<int,PartyClient*>::iterator iter;

	for(iter=m_allClient.begin();iter!=m_allClient.end();++iter)
	{
		delete iter->second;
	}

	m_allClient.clear();
	m_loginClient.clear();

	return 0;
}

int PartyServer::closeGame()
{
	m_game->shutDown();
	delete m_game;
	m_game=NULL;
	return 0;
}


int PartyServer::closeListen()
{
	ev_io_stop(m_evLoop,&m_evAccept);
	close(m_listenFd);
	return 0;
}


int PartyServer::closeSql()
{
	if(m_sqlClient)
	{
		delete m_sqlClient;
		m_sqlClient=NULL;
	}

	return 0;
}


int PartyServer::closeRedis()
{
	for(int i=0;i<m_mainSize;i++)
	{
		if(m_mainRc[i])
		{
			m_mainRc[i]->deinit();
		}

	}

	if(m_cacheRc)
	{
		m_cacheRc->deinit();
	}
	return 0;
}


int PartyServer::closeLog()
{
	xt_log.stop();
	return 0;
}


int PartyServer::refreshPlayerNuToRedis()
{
	int connect_nu=m_loginClient.size();

	int ret=m_cacheRc->command("hset gameinfo %d %d", m_conf["game"]["port"].asInt(),connect_nu);
	if(ret<0)
	{
		xt_log.error("cache redis error( set gameinfo).\n");
	}
	return ret;
}

void PartyServer::sendPlayerResultFlow(int uid, int alert_value, int value)
{
	if(uid<VALID_CLIENT_UID_MIN)
	{
		return;
	}
	time_t ts=time(NULL);	
			
    int type_id = m_conf["info"].get("type", 501).asInt();
    int v_id = m_conf["info"].get("vid", 601).asInt();

	char buf[1024];
	sprintf(buf,"insert flow(uid,type,vid,alter_type,alter_value,current_value,ts) value(%d,%d,%d,2,%d,%d,%ld)",uid,type_id,v_id,alert_value,value,ts);

    //printf("party server sendBetFlow: %s\n",buf);

	int ret=m_sqlClient->query(buf);
	if(ret<0)
	{
		//xt_log.error("send bet flow failed uid(%d) alert_value(%d) value(%d).\n",uid,alert_value,value);
        xt_log.error("party server sendBetFlow failed: %s\n",buf);
	}
}

void PartyServer::sendSpeaker(int cmd,int uid,const char* name,const char* content)
{
	Jpacket packet;

	packet.val["cmd"]=cmd;
	packet.val["uid"]=uid;
	packet.val["name"]=name;
	packet.val["str"]=content;

	packet.end();

	//printf("speaker=%s \n",packet.val.toStyledString().c_str());

	m_speakerRc->command("PUBLISH zjh_speaker %s",packet.val.toStyledString().c_str());
}
