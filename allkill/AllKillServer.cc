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

#include "AllKillServer.h"
#include "AllKillClient.h"
#include "AllKillPlayer.h"
#include "AllKillGame.h"


#include "AllKillMacros.h"
//#include <mysql/mysql.h>

extern Log xt_log;

//���ӵ���
void AllKillServer::onClientConnect(struct ev_loop* loop,struct ev_io* w,int revents)
{
	if (EV_ERROR & revents) 
	{
		xt_log.error("got invalid event\n");
		return;
	}

	AllKillServer* s = (AllKillServer*)w->data;
	s->clientConnect();
}

void AllKillServer::onClientClose(AllKillClient* client,void* data)
{
	AllKillServer* s=(AllKillServer*) data;
	s->clientClose(client);
}

//�յ��ͻ���Э������
void AllKillServer::onReciveClientCmd(AllKillClient* client,void* data,Jpacket& package)
{
	AllKillServer* s = (AllKillServer*) data;
	s->reciveClientCmd(client,package);
}


AllKillServer::AllKillServer()
{
	m_evLoop=NULL;
	m_listenFd=-1;
	m_game=NULL;

	for (int i = 0; i < 20; i++)
	{
		m_mainRc[i]=NULL;
	}

	m_mainSize=0;
	m_cacheRc=NULL;
	m_sqlClient=NULL;
}


AllKillServer::~AllKillServer()
{

}



//���ܿͻ�������
void AllKillServer::clientConnect()
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

	// ����client��
	AllKillClient* client = new (std::nothrow)AllKillClient(m_evLoop);

	// ����client�Ļص�����
	client->setOnReciveCmdFunc(AllKillServer::onReciveClientCmd, this);
	client->setOnCloseFunc(AllKillServer::onClientClose, this);

	m_allClient[fd] = client;    // ����m_allClient�б���
	xt_log.info("client fd=%d connect\n", fd);

	client->connectStart(fd);

}

// ��ҵǳ� 
void AllKillServer::clientClose(AllKillClient* client)
{
	int uid = client->getUid();
	AllKillPlayer* player = (AllKillPlayer*)client->getUserData();
	if (uid > 0)
	{
		m_game->playerLogout(player);
		m_loginClient.erase(uid);
	}

	int fd = client->getClientFd();
	m_allClient.erase(fd);

	delete client;
}

// ����ͻ�������
void AllKillServer::reciveClientCmd(AllKillClient* client,Jpacket& package)
{

	int fd = client->getClientFd();

	if (!package.val["cmd"].isNumeric()) 
	{
		xt_log.error("client(%d) cmd not numberic\n",fd);
		client->closeConnect();
		return;
	}

	xt_log.info("Func[%s] recive from client(%d) %s \n",
		__FUNCTION__, client->getClientFd(),package.tostring().c_str());

	int cmd = package.val["cmd"].asInt();

	AllKillPlayer* player = (AllKillPlayer*)client->getUserData();

	if (AK_LOGIN_C == cmd)
	{
		if (player != NULL)  // �Ѿ���¼����ֱ�ӶϿ�֮ǰ�����ӣ�������
		{
			xt_log.error("client already login uid=%d\n", player->getUid());
			client->closeConnect();
			return;
		}

		clientLogin(client,package);
		return ;
	}

#ifdef AK_DEBUG 

	if(cmd==AK_ASK_SERVER_SHUT_DOWN)
	{
		shutDown();
		return ;
	}
#endif 

	if (NULL == player)
	{
		xt_log.error("client(%d) not login error cmd(%d)\n", fd, cmd);
		return ;
	}


	switch (cmd)
	{
		case AK_PLAYER_BET_C:   // ѹע
			clientBet(client,package);
			break;

		case AK_ASK_ROLE_C:    // ������ׯ
			clientAskRole(client,package);
			break;

		case AK_ASK_UN_ROLE_C:  // ������ׯ
			clientUnRole(client,package);
			break;

		case AK_LOGOUT_C:      // �ǳ�
			clientLogOut(client,package);
			break;

		case AK_CHAT_C:       // ����������Ϣ
			clientChat(client,package);
			break;

		case AK_FACE_C:      // ������
			clientFace(client,package);
			break;

		case AK_SIT_DOWN_C:  // ����λ��
			clientSitDown(client,package);
			break;

		case AK_STAND_UP_C:  // �뿪λ��
			clientStandUp(client,package);
			break;

		case AK_PLAYER_INFO_C:  // ��ѯ�����Ϣ��Ϣ
			clientUpdatePlayer(client,package);
			break;

		case AK_LOTTERY_FIRST_C:  // �񽱵�һ���б�
			clientLotterFirstPlayers(client);
			break;

		case AK_BET_LOTTERY_C:
			clientGetBetLotter(client);
			break;

		default:
			xt_log.error("client(%d) unkown cmd(%d)\n", fd, cmd);
	}
}

// �ͻ��˵�¼
void AllKillServer::clientLogin(AllKillClient* client,Jpacket& package) 
{
	int uid = package.val["uid"].asInt();
	std::string skey = package.val["skey"].asString();  ///�Ự��Կ


	if (uid < 0)
	{
		xt_log.error("check uid(%d) error\n", uid);
		client->closeConnect();
		return ;
	}


	//��ȡ�����Ϣ
	int i = uid % m_mainSize;    //�����Ϣ����redis��
	int ret = m_mainRc[i]->command("hget u:%d skey",uid);
	if (ret < 0)
	{
		xt_log.error("check uid(%d) skey error\n",uid);
		client->closeConnect();
		return;
	}

	//У��Ự��Կ
	if (uid > VALID_CLIENT_UID_MIN)
	{
		if (m_mainRc[i]->reply->str
			&&skey.compare(m_mainRc[i]->reply->str) != 0)
		{
			//У��ʧ�ܣ��˳�
			xt_log.error("check skey error,client_skey[%s], server_skey[%s]\n",
				skey.c_str(), m_mainRc[i]->reply->str);
			client->closeConnect();
			return;
		}
	}


	client->setUid(uid);

	//������Ҷ���
	AllKillPlayer* player = m_game->getPlayer(uid);  
	client->setUserData(player);

	std::map<int, AllKillClient*>::iterator client_iter = m_loginClient.find(uid);

	/* if client logined */
	if(client_iter!=m_loginClient.end())
	{
		xt_log.warn("double client uid(%d)\n",uid);
	
		int fd = client_iter->second->getClientFd();
		m_allClient.erase(fd);

		delete client_iter->second;
		m_loginClient.erase(client_iter);
	}


	m_loginClient[uid]=client;	// ������m_loginClient

	// ������Ϸ
	m_game->playerLogin(player);
}

// ѹע
void AllKillServer::clientBet(AllKillClient* client,Jpacket& package)
{
	AllKillPlayer* player=(AllKillPlayer*)client->getUserData();
	m_game->playerBet(player,package);
}

// ������ׯ
void AllKillServer::clientAskRole(AllKillClient* client,Jpacket& package)
{
	AllKillPlayer* player=(AllKillPlayer*)client->getUserData();
	m_game->playerAskRole(player,package);
}

// ������ׯ
void AllKillServer::clientUnRole(AllKillClient* client,Jpacket& package)
{
	AllKillPlayer* player=(AllKillPlayer*)client->getUserData();
	m_game->playerUnRole(player,package);
}

// ��ҵǳ� 
void AllKillServer::clientLogOut(AllKillClient* client,Jpacket& package)
{
	clientClose(client);
}

// �㲥�������
void AllKillServer::clientChat(AllKillClient* client,Jpacket& package)
{
	AllKillPlayer* player=(AllKillPlayer*)client->getUserData();

	// ���ж��ܷ�˵��
	if (isCanSpeak(player))
	{
		Jpacket packet;
		packet.val["cmd"]     = AK_CHAT_SB;
		packet.val["uid"]     = player->getUid();
		packet.val["avatar"]  = player->getAvatar();
		packet.val["name"]    = player->getName();
		packet.val["sex"]     = player->getSex();

		packet.val["content"] =package.val["content"];
		packet.end();

		broadcast(NULL,packet.tostring());
	}

}

// �㲥����
void AllKillServer::clientFace(AllKillClient* client,Jpacket& package)
{
	AllKillPlayer* player=(AllKillPlayer*)client->getUserData();

	//����id
	Json::Value &val = package.tojson();
	int faceid = val["faceid"].asInt();

	Jpacket packet;
	packet.val["cmd"]    = AK_FACE_SB;
	packet.val["uid"]    = player->getUid();
	packet.val["seatid"] = player->m_seatId;
	packet.val["faceid"] = faceid;
	packet.end();
	broadcast(NULL, packet.tostring());
}




int AllKillServer::start(struct ev_loop* loop, const std::string& conf_file)
{
	m_evLoop=loop;

	if (this->parseConfig(conf_file) < 0)
	{
		printf("parseConfig failed\n");
		return -1;
		
	}
	else 
	{
		printf("parseConfig success\n");
	}

	if (this->startLog() < 0)
	{
		printf("start log error\n");
		return -1;
	}
	else 
	{
		printf("start log success\n");
	}


	if (this->singleInstance() < 0)
	{
		printf("single_instance_running falied\n");
		return -1;
	}
	else 
	{
		printf("single_instance_running success\n");
	}

	if (this->startRedis() < 0)
	{
		printf("start redis failed\n");
		return -1;
	}
	else 
	{
		printf("start redis success\n");

	}

	if (this->startSql() < 0)
	{
		printf("start sql failed\n");
		return -1;
	}
	else 
	{
		printf("start sql success\n");
	}


	if (this->startListen() < 0)
	{
		printf("start listen falied\n");
		return -1;
	}
	else 
	{
		printf("start listen success\n");
	}

	if (this->startGame() < 0)
	{
		printf("start game failed\n");
	}
	else 
	{
		printf("start game success\n");
	}


	return 0;
}






int AllKillServer::parseConfig(const std::string& conf_file)
{
	std::ifstream in(conf_file.c_str(), std::ifstream::binary); 
	if (!in) 
	{
		std::cout << "init file no found." << endl;
		return -1;
	}

	Json::Reader reader;
	bool ret = reader.parse(in, m_conf);  //����JSONֵ��m_conf
	if (!ret) 
	{//�ļ���ʽ����
		in.close();
		std::cout << "init file parser." << endl;
		return -1;
	}

	in.close();
	return 0;

}


int AllKillServer::startListen()
{
	int fd;
	xt_log.info("Listening on %s:%d\n",
			m_conf["game"]["host"].asString().c_str(),
			m_conf["game"]["port"].asInt());

	struct sockaddr_in addr;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) 
	{
		xt_log.error("File[%s] Line[%d]: socket failed: %s\n", 
			__FILE__, __LINE__, strerror(errno));
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_conf["game"]["port"].asInt());
	addr.sin_addr.s_addr = inet_addr(m_conf["game"]["host"].asString().c_str());

	if (INADDR_NONE == addr.sin_addr.s_addr) 
	{
		xt_log.error("File[%s] Line[%d]:Incorrect ip address!\n", __FILE__, __LINE__);
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

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
	{
		xt_log.error("File[%s] Line[%d]: bind failed: %s\n", __FILE__, __LINE__, strerror(errno));
		close(fd);
		return -1;
	}

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
	listen(fd, 100);

	m_listenFd = fd;
	m_evAccept.data = this;
	ev_io_init(&m_evAccept, AllKillServer::onClientConnect, fd, EV_READ);
	ev_io_start(m_evLoop, &m_evAccept);


	xt_log.info("listen ok\n");

	return 0;
}

//��ʼ��Ϸ
int AllKillServer::startGame()
{
	m_game = new AllKillGame();
	m_game->configGame(m_conf);
	m_game->start(this,m_evLoop);
	return 0;
}


int AllKillServer::singleInstance()
{
	int ret;
	ret = single_instance_running(m_conf.get("pid_file","conf/allkill.pid").asString().c_str());
	return ret;
}



int AllKillServer::startLog()
{
	//��Ŀ¼�����ڣ������ֱ���˳�
	xt_log.start(m_conf["log"].get("log_file", "log/zjhsvr.log").asString(),
			m_conf["log"].get("level", 4).asInt(),
			m_conf["log"].get("console", 0).asInt(),
			m_conf["log"].get("rotate", 1).asInt(),
			m_conf["log"].get("max_size", 1073741824).asInt(),
			m_conf["log"].get("max_file", 50).asInt());

	return 0;
}

// redis�� 
int AllKillServer::startRedis()
{
	int ret;
	// ����
	m_mainSize = m_conf["main-db"].size();

	for (int i = 0; i < m_mainSize; i++)
	{
		m_mainRc[i] = new RedisClient();

		ret = m_mainRc[i]->init(m_conf["main-db"][i]["host"].asString(), 
			m_conf["main-db"][i]["port"].asInt(), 
			1000, 
			m_conf["main-db"][i]["pass"].asString());

		if (ret < 0)
		{
			xt_log.error("main db redis error\n");
			return -1;      
		}
	}

	// "cache-db"
	m_cacheRc = new RedisClient();

	ret = m_cacheRc->init(m_conf["cache-db"]["host"].asString(),
			m_conf["cache-db"]["port"].asInt(), 
			1000, 
			m_conf["cache-db"]["pass"].asString());

	if (ret < 0)
	{
		xt_log.error("cache db redis error\n");
		return -1;
	}

	// "speaker-db" ��Ϣ����/����
	m_speakerRc = new RedisClient();
	ret = m_speakerRc->init(m_conf["speaker-db"]["host"].asString(),
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



int AllKillServer::startSql()
{
	int ret;
	m_sqlClient = new XtSqlClient();

	ret = m_sqlClient->connect(m_conf["sql-db"]["host"].asString().c_str(),
							m_conf["sql-db"]["port"].asInt(),
							m_conf["sql-db"]["user"].asString().c_str(),
							m_conf["sql-db"]["pass"].asString().c_str(),
							m_conf["sql-db"]["dbname"].asString().c_str());
	if (ret != 0)
	{
		xt_log.error("connect sql error\n");
		return -1;
	}

	return 0;
}



RedisClient* AllKillServer::getDataRedis(int uid)
{
	return m_mainRc[uid%m_mainSize];
}


//void __xorfunc(std::string &nString)
//{
//	const int KEY = 13;
//	int strLen = (nString.length());
//	char *cString = (char*)(nString.c_str());
//	
//	for (int i = 0; i < strLen; i++)
//	{
//		*(cString + i) = (*(cString + i) ^ KEY);
//	}
//}

// ����
void AllKillServer::unicast(AllKillPlayer* player,const std::string& data)
{
	// У�������
	if (NULL == player)
	{
		xt_log.error("Func[%s] player is null\n",__FUNCTION__);
		return;
	}

	int uid = player->getUid();
	std::map<int,AllKillClient*>::iterator iter =m_loginClient.find(uid);
	if (iter != m_loginClient.end())
	{
		AllKillClient* client=iter->second;
		client->send(data);
	}
	else 
	{
		xt_log.error("player(%d) is offline now\n",uid);
	}

}

// �㲥
void AllKillServer::broadcast(AllKillPlayer* player,const std::string& data)
{
	int uid = -1;
	if (player != NULL)
	{
		uid = player->getUid();
	}

	for (std::map<int, AllKillClient*>::iterator iter = m_loginClient.begin();
		iter != m_loginClient.end(); ++iter)
	{
		if(iter->first != uid 
			&& iter->second != NULL)
		{
			iter->second->send(data);
		}
	}
}



int AllKillServer::shutDown()
{
	closeClient();
	closeGame();
	closeListen();
	closeSql();
	closeRedis();
	closeLog();
	return 0;
}

int AllKillServer::closeClient()
{
	std::map<int,AllKillClient*>::iterator iter;

	for(iter=m_allClient.begin();iter!=m_allClient.end();++iter)
	{
		delete iter->second;
	}

	m_allClient.clear();
	m_loginClient.clear();

	return 0;
}

int AllKillServer::closeGame()
{
	m_game->shutDown();
	delete m_game;
	m_game=NULL;
	return 0;
}


int AllKillServer::closeListen()
{
	ev_io_stop(m_evLoop, &m_evAccept);
	close(m_listenFd);
	return 0;
}


int AllKillServer::closeSql()
{
	if(m_sqlClient)
	{
		delete m_sqlClient;
		m_sqlClient=NULL;
	}

	return 0;
}


int AllKillServer::closeRedis()
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


int AllKillServer::closeLog()
{
	xt_log.stop();
	return 0;
}

// ˢ�����������
int AllKillServer::refreshPlayerNuToRedis()
{
	int connect_nu = m_loginClient.size();

	int ret=m_cacheRc->command("hset gameinfo %d %d", m_conf["game"]["port"].asInt(),connect_nu);
	if(ret<0)
	{
		xt_log.error("cache redis error( set gameinfo).\n");
	}
	return ret;
}


// �Ծ���ˮ���
void AllKillServer::sendBetFlow(int vid, int uid, int alert_value, int value)
{
	if (uid < VALID_CLIENT_UID_MIN)
	{
		//���ü�¼�����˶Ծ���Ϣ
		return;
	}

	time_t ts=time(NULL);	

	char buf[1024];
	sprintf(buf,
		"INSERT INTO flow(uid,type,vid,alter_type,alter_value,current_value,CREATE_TIME) VALUES(%d,%d,%d,2,%d,%d,%ld)",
		uid,AK_BET_FLOW_ID,vid,alert_value,value,ts);

	//printf("flowsql:%s\n",buf);

	int ret=m_sqlClient->query(buf);
	if(ret<0)
	{
		xt_log.error("Func[%s] send bet flow failed uid(%d) alert_value(%d) value(%d) sql=[%s].\n",
			__FUNCTION__, uid,alert_value,value, buf);
	}

}

//������ˮ���
void AllKillServer::sendLotteryFlow(int vid,int uid,int alert_value,int value)
{
	if(uid<VALID_CLIENT_UID_MIN)
	{
		return;
	}

	time_t ts=time(NULL);	

	char buf[1024];
	sprintf(buf,"insert flow(uid,type,vid,alter_type,alter_value,current_value,ts) value(%d,%d,%d,2,%d,%d,%ld)",
		                      uid,AK_ROTTLE_FLOW_ID,vid,alert_value,value,ts);

	//printf("flowsql:%s\n",buf);

	int ret=m_sqlClient->query(buf);
	if(ret<0)
	{
		xt_log.error("send bet flow failed uid(%d) alert_value(%d) value(%d).\n",uid,alert_value,value);
	}

}

//������Ϣ
void AllKillServer::sendSpeaker(int cmd,int uid,const char* name,const char* content)
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


//����
void AllKillServer::clientSitDown(AllKillClient* client, Jpacket& package)
{
	AllKillPlayer* player=(AllKillPlayer*)client->getUserData();
	int seat_id =package.val["seat_id"].asInt();
	m_game->playerSitDown(player, seat_id);
}



// վ��
void AllKillServer::clientStandUp(AllKillClient* client,Jpacket& package)
{
	AllKillPlayer* player=(AllKillPlayer*)client->getUserData();
	int seat_id =package.val["seat_id"].asInt();

	m_game->playerStandUp(player, seat_id);
}


// ���������Ϣ
void AllKillServer::clientUpdatePlayer(AllKillClient* client,Jpacket& package)
{
	AllKillPlayer* player=(AllKillPlayer*)client->getUserData();

	int uid =package.val["uid"].asInt();

	RedisClient* redisClient = getDataRedis(uid);

	int ret = redisClient->command("hgetall hu:%d", uid);

	if (ret >= 0 && redisClient->is_array_return_ok() >= 0) 
	{
		//��ѯ�ɹ�
		std::string name = redisClient->get_value_as_string("nickName");
		int sex = redisClient->get_value_as_int("sex");
		std::string	avatar = redisClient->get_value_as_string("avatar");
		int exp = redisClient->get_value_as_int("exp");
		int rmb = redisClient->get_value_as_int("rmb");
		int money = redisClient->get_value_as_int("money");
		int coin = redisClient->get_value_as_int("coin");
		int total_board = redisClient->get_value_as_int("totalBoard");
		int total_win = redisClient->get_value_as_int("totalWin");
		int pcount = redisClient->get_value_as_int("playCount");
		int vlevel = redisClient->get_value_as_int("vlevel");
		std::string	ps = redisClient->get_value_as_string("ps");
		int max_win_money = redisClient->get_value_as_int("maxWinMoney");
		int best_board = redisClient->get_value_as_int("bestBoard");
		std::string	best_board_detail = redisClient->get_value_as_string("bestBoardDetail");

		Jpacket packet;
		packet.val["cmd"] = AK_PLAYER_INFO_SUCC_SU;
		packet.val["uid"]               = uid;       
		packet.val["name"]              = name;
		packet.val["sex"]               = sex;
		packet.val["avatar"]            = avatar;
		packet.val["exp"]               = exp;
		packet.val["rmb"]               = rmb; 
		packet.val["money"]             = money;
		packet.val["coin"]              = coin;
		packet.val["total_board"]       = total_board;
		packet.val["total_win"]         = total_win;
		packet.val["pcount"]            = pcount;
		packet.val["vlevel"]            = vlevel;
		packet.val["ps"]                = ps;  
		packet.val["max_win_money"]     = max_win_money;  
		packet.val["best_board"]        = best_board ;    
		packet.val["best_board_detail"] = best_board_detail;
		packet.end();

		unicast(player, packet.tostring());
		xt_log.info("Func[%s] query player infomsg[%s]", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	} 
	else 
	{
		//��ѯʧ��
		Jpacket packet;
		packet.val["cmd"] = AK_PLAYER_INFO_ERR_SU;		
		packet.val["ret"] = -1 ;    
		packet.val["msg"] = "";
		packet.end();

		unicast(player, packet.tostring());

		xt_log.error("Func[%s] query player info msg[%s]", 
			__FUNCTION__, packet.val.toStyledString().c_str());
	}

}

// һ�Ȼ�����б�
void AllKillServer::clientLotterFirstPlayers(AllKillClient* client)
{
	AllKillPlayer* player = (AllKillPlayer*)client->getUserData();
	m_game->sendLotterFirstPlayers(player);
}


// ��ȡ��ע����(��ˮ)
void AllKillServer::clientGetBetLotter(AllKillClient* client)
{
	AllKillPlayer* player = (AllKillPlayer*)client->getUserData();

	int money = getBetLottery(player->getVid(), player->getUid());
	
	Jpacket packet;

	packet.val["cmd"] = AK_BET_LOTTERY_SU;
	if (money > 0)
	{
		packet.val["ret"] = 0;
		packet.val["lottery_money"] = money;
	}
	else
	{
		packet.val["ret"] = -1;
		packet.val["lottery_money"] = 0;
	}
	packet.end();

	unicast(player, packet.tostring());

	xt_log.error("Func[%s] ��ע����=[%s]", 
		__FUNCTION__, packet.val.toStyledString().c_str());

}



// �����ע���
void AllKillServer::savePlayerBet(int vid,int uid,int bet_num,int win_flag)
{
	if (uid < VALID_CLIENT_UID_MIN)
	{
		//���ü�¼�����˶Ծ���Ϣ
		return;
	}

	time_t ts=time(NULL);	

	char buf[1024] = {0};
	sprintf(buf,
		"insert bet_flow(vid,uid,bet_num,bet_time,win_flag) value(%d,%d,%d,%d,%ld)",
		 vid,uid,bet_num,win_flag,ts);

	//printf("flowsql:%s\n",buf);

	int ret = m_sqlClient->query(buf);
	if (ret < 0)
	{
		xt_log.error("Func[%s] error sql=%s.\n", __FUNCTION__, buf);
	}

}



// ��ȡ��ע����
int AllKillServer::getBetLottery(int vid, int uid)
{

	int lotteryMoney = 0;
	char buf[1024] = {0};
	sprintf(buf,
	        "SELECT SUM(lottery_money) FROM bet_lottery_daily WHERE get_flag=0 AND vid=%d AND uid=%d AND to_days(lottery_time)=to_days(now())",
            vid,uid);

	vector<MYSQL_ROW> vecRow = m_sqlClient->GetRecord(buf);
	if (vecRow.size() > 0)
	{
		lotteryMoney = atoi(vecRow[0][0]);
	}
	
	// ������ȡ���Ϊ1
	memset(buf, 0, sizeof(buf));
	sprintf(buf,
		"UPDATE bet_lottery_daily SET get_flag=1 WHERE vid=%d AND uid=%d AND to_days(lottery_time)=to_days(now())",
		vid,uid);
	
	int ret = m_sqlClient->query(buf);
	if (ret < 0)
	{
		xt_log.error("Func[%s] error sql=%s.\n", __FUNCTION__, buf);
	}
	return lotteryMoney;

}


// ��ѯ������ע����
vector<BetBox> AllKillServer::queryBetLottery(int vid, int uid)
{
	std::vector<BetBox> vecBetBox;

	char buf[1024] = {0};

	sprintf(buf,
		"SELECT box_type,lottery_money,get_flag FROM bet_lottery_daily WHERE vid=%d AND uid=%d AND to_days(lottery_time)=to_days(now())",
		vid, uid);

	vector<MYSQL_ROW> vecRow = m_sqlClient->GetRecord(buf);
    for (unsigned int i =0; i < vecRow.size(); i++)
    {
		BetBox betBox;
		betBox.boxType      = atoi(vecRow[i][0]);
		betBox.lotteryMoney = atoi(vecRow[i][1]);
		betBox.getFlag      = atoi(vecRow[i][2]);

		vecBetBox.push_back(betBox);
    }
	return vecBetBox;

}


// ����˵����
bool AllKillServer::isCanSpeak(AllKillPlayer* player)
{
	// �����û�����
	if (player->getBanned() > 0)
	{
		return false;
	}

	//û�н��ԣ��������ϴη��Ե�ʱ���ֵ
	unsigned long long  now_speak_time = time(0);
	int time_inteval = now_speak_time -  player->last_speak_time;

	// ���Լ��С�ڵ��ޣ����Դ���
	if (time_inteval < SPEAK_MIN_INTERVAL)
	{
		player->mQuickSpeakTimes++;   // �ۼƿ��ٷ��Դ���

		//����10�Σ���Ϊ�Ƕ���ˢ�����ˣ����Դ���
		if (player->mQuickSpeakTimes > SPEAK_MIN_COUNT)
		{
			xt_log.info("����uid[%d]", player->getUid());
			player->setBanned(1);
			return false;
		}

		xt_log.warn("Func[%s] ���ڶ���ˢ��... uid[%d] name[%s] time_inteval[%d] mQuickSpeakTimes[%d]\n", 
			__FUNCTION__, player->getUid(), player->getName(), time_inteval, player->mQuickSpeakTimes);
	} 
	else 
	{
		player->mQuickSpeakTimes = 0;  //���ÿ��ٷ��Դ���
	}

	// �����ϴη���ʱ��
	player->last_speak_time = now_speak_time;

	return true;
}