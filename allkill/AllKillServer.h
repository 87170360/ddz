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


class AllKillClient;
class AllKillGame;
class AllKillPlayer;

class AllKillServer 
{
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

		void sendBetFlow(int vid,int uid,int alert_value,int cur_value);
		void sendRottleFlow(int vid,int uid,int alert_value,int cur_value);
		void sendSpeaker(int cmd,int uid,const char* name,const char* content);
        void saveBetNum(int uid, int num);

		int refreshPlayerNuToRedis();

		XtSqlClient* getSqlClient(){return m_sqlClient;}
		RedisClient* getCacheRedis(){return m_cacheRc;}

        //校验消息格式
        bool checkMsg(const Jpacket& package);
		
	public: 
		void reciveClientCmd(AllKillClient* client,Jpacket& package);

		void clientConnect();
		void clientClose(AllKillClient* client);
		void clientLogin(AllKillClient* client,Jpacket& package);
		void clientBet(AllKillClient* client,Jpacket& package);
		void clientAskRole(AllKillClient* client,Jpacket& package);
		void clientUnRole(AllKillClient* client,Jpacket& package);
		void clientLogOut(AllKillClient* client,Jpacket& package);
		void clientChat(AllKillClient* client,Jpacket& package);
		void clientDesk(AllKillClient* client,Jpacket& package);

		void unicast(AllKillPlayer* player,const std::string& data);
		void broadcast(AllKillPlayer* player,const std::string& data);

	private:
		struct ev_loop* m_evLoop;
		ev_io m_evAccept;

		/* socket fd */
		int m_listenFd;
		Json::Value m_conf;

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



