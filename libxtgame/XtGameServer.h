#ifndef _XT_GAME_SERVER_H_
#define _XT_GAME_SERVER_H_


#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <ev.h>
#include <json/json.h>

class XtGameLogic;
class XtJsonPacket;
class XtGameClient;
class XtRedisClient;
class XtBuffer;
class XtGamePlayer;
class XtSqlClient;


#define XT_VALID_CLIENT_UID_MIN 10000

class XtGameServer 
{
	public:
		XtGameServer();
		virtual ~XtGameServer();

	public:
		virtual int start(struct ev_loop* loop,const std::string& conf);
		virtual int shutDown();

	public:
		virtual XtGameLogic* onCreateGame()=0;
		virtual XtGameClient* onCreateClient()=0;

	public:
		void addClient(XtGameClient* client);
		void removeClient(XtGameClient* client);

		XtRedisClient* getDataRedis(int uid);
		XtRedisClient* getCacheRedis();


	public:
		virtual void onReciveClientCmd(XtGameClient* client,XtBuffer* buffer);

	public:
		void clientConnect();

	public:
		void unicast(XtGamePlayer* player,XtJsonPacket* packet);
		void broadcast(XtGamePlayer* player,XtJsonPacket* packet);

	public:
		void handleClientLogin(XtGameClient* client,XtJsonPacket* packet);
		void handleClientLogout(XtGameClient* client,XtJsonPacket* packet);

	public:
		void sendFlow(int type,int vid,int uid,int alter_type,int alter_value,int cur_value); 
		void sendSpeaker(int cmd,int uid,const char* name,const char* content);



	protected:

		int parseConfig(const std::string& conf_file);
		int singleInstance();
		int startLog();
		int startRedis();
		int startSql();
		int startListen();
		int startGame();

		int closeClient();
		int closeGame();
		int closeListen();
		int closeRedis();
		int closeLog();

	protected:
		struct ev_loop* m_evLoop;
		ev_io m_evAccept;


		bool m_start;

		/* socket fd */
		int m_listenFd;

		/* config */
		Json::Value m_conf;

		/* client */
		std::map<int,XtGameClient*> m_allClient;
		std::map<int,XtGameClient*> m_loginClient;

		/* game */
		XtGameLogic* m_game;

		/* redis */
		std::vector<XtRedisClient*> m_mainsRc;
		XtRedisClient* m_cacheRc;
		XtRedisClient* m_logRc;
		XtRedisClient* m_speakerRc;

		XtSqlClient* m_sqlClient;

};
#endif /*_XT_GAME_SERVER_H_*/


