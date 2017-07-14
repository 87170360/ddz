#ifndef _PARTY_SERVER_H_
#define _PARTY_SERVER_H_


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


class PartyClient;
class PartyGame;
class PartyPlayer;

class PartyServer 
{
	public:
		static void onClientConnect(struct ev_loop* loop,struct ev_io* w,int revents);
		static void onClientClose(PartyClient* client,void* data);
		static void onReciveClientCmd(PartyClient* client,void* data,Jpacket& package);

	public:
		PartyServer();
		~PartyServer();

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

		void sendPlayerResultFlow(int uid, int alert_value, int cur_value);
		void sendSpeaker(int cmd, int uid, const char* name, const char* content);

		int refreshPlayerNuToRedis();

		XtSqlClient* getSqlClient(){return m_sqlClient;}
		RedisClient* getCacheRedis(){return m_cacheRc;}
		
	public: 
		void reciveClientCmd(PartyClient* client,Jpacket& package);

		void clientConnect();
		void clientClose(PartyClient* client);
		void clientLogin(PartyClient* client,Jpacket& package);
		void clientBet(PartyClient* client,Jpacket& package);
		void clientAskRole(PartyClient* client,Jpacket& package);
		void clientUnRole(PartyClient* client,Jpacket& package);
		void clientLogOut(PartyClient* client,Jpacket& package);
		void clientChat(PartyClient* client,Jpacket& package);

		void unicast(PartyPlayer* player,const std::string& data);
		void broadcast(PartyPlayer* player,const std::string& data);

	private:
		struct ev_loop* m_evLoop;
		ev_io m_evAccept;

		/* socket fd */
		int m_listenFd;

		Json::Value m_conf;

		/* client info */
		std::map<int,PartyClient*> m_allClient;
		std::map<int,PartyClient*> m_loginClient;

		/* game */
		PartyGame* m_game;

		/* redis */
		RedisClient* m_mainRc[20];
		int m_mainSize;

		RedisClient* m_cacheRc;
		RedisClient* m_speakerRc;
		XtSqlClient* m_sqlClient;
};

#endif /*_PARTY_SERVER_H_*/
