#ifndef _ALL_KILL_SERVER_H_
#define _ALL_KILL_SERVER_H_


#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <ev.h>

#include <json/json.h>
#include "redis_client.h"
#include "jpacket.h"
#include "XtSqlClient.h"
#include <vector>

class AllKillClient;
class AllKillGame;
class AllKillPlayer;
class BetBox;

using namespace std;

class AllKillServer 
{
public:
	bool isCanSpeak(AllKillPlayer* player);
public:
	static void onClientConnect(struct ev_loop* loop,struct ev_io* w,int revents);
	static void onClientClose(AllKillClient* client,void* data);
	static void onReciveClientCmd(AllKillClient* client,void* data,Jpacket& package);

public:
	AllKillServer();
	~AllKillServer();

public:
	int start(struct ev_loop* loop,const std::string& conf);

	int parseConfig(const std::string& conf_file);
	int singleInstance();
	int startLog();
	int startRedis();
	int startSql();
	int startListen();
	int startGame();


public:
	int shutDown();
	int closeClient();
	int closeGame();
	int closeListen();
	int closeSql();
	int closeRedis();
	int closeLog();


public:
	RedisClient* getDataRedis(int uid);

	// �Ծ���ˮ���
	void sendBetFlow(int vid,int uid,int alert_value,int cur_value);
	// ������ˮ���
	void sendLotteryFlow(int vid,int uid,int alert_value,int cur_value);
	// ������Ϣ
	void sendSpeaker(int cmd,int uid,const char* name,const char* content);

	// ˢ��redis����������
	int refreshPlayerNuToRedis();

	XtSqlClient* getSqlClient(){return m_sqlClient;}
	RedisClient* getCacheRedis(){return m_cacheRc;}

	// �����ע���
	void savePlayerBet(int vid,int uid,int bet_num,int win_flag);

	// ��ȡ��ע����
	int getBetLottery(int vid, int uid);

	// ��ѯ��ע����
	vector<BetBox> queryBetLottery(int vid, int uid);


public: 
	//����ͻ�������Э������
	void reciveClientCmd(AllKillClient* client,Jpacket& package);

	//���ܿͻ�������
	void clientConnect();
	//�ͻ��˶Ͽ�
	void clientClose(AllKillClient* client);

	//�ͻ��˵�¼
	void clientLogin(AllKillClient* client,Jpacket& package);
	//��ע
	void clientBet(AllKillClient* client,Jpacket& package);
	//������ׯ
	void clientAskRole(AllKillClient* client,Jpacket& package);
	//������ׯ
	void clientUnRole(AllKillClient* client,Jpacket& package);
	//��ҵǳ� 
	void clientLogOut(AllKillClient* client,Jpacket& package);

	//����
	void clientChat(AllKillClient* client,Jpacket& package);

	//������
	void clientFace(AllKillClient* client,Jpacket& package);

	//����
	void clientSitDown(AllKillClient* client,Jpacket& package);

	//վ��
	void clientStandUp(AllKillClient* client,Jpacket& package);

	// ���������Ϣ
	void clientUpdatePlayer(AllKillClient* client,Jpacket& package);

	// һ�Ȼ�����б�
	void clientLotterFirstPlayers(AllKillClient* client);

	// ��ȡ��ע����(��ˮ)
	void clientGetBetLotter(AllKillClient* client);

	void unicast(AllKillPlayer* player,const std::string& data);
	void broadcast(AllKillPlayer* player,const std::string& data);

public:
	/*�����ļ���JSON*/
	Json::Value m_conf;

private:
	struct ev_loop* m_evLoop;
	ev_io m_evAccept;


	/* socket fd */
	int m_listenFd;


	/* client info */
	std::map<int,AllKillClient*> m_allClient;
	std::map<int,AllKillClient*> m_loginClient;

	/* game */
	AllKillGame* m_game;


	/* redis */
	RedisClient* m_mainRc[20];
	int m_mainSize;

	RedisClient* m_cacheRc;
	RedisClient* m_speakerRc;


	XtSqlClient* m_sqlClient;
};



#endif /*_ALL_KILL_SERVER_H_*/



