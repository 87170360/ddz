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

	// 对局流水入库
	void sendBetFlow(int vid,int uid,int alert_value,int cur_value);
	// 奖金流水入库
	void sendLotteryFlow(int vid,int uid,int alert_value,int cur_value);
	// 发布消息
	void sendSpeaker(int cmd,int uid,const char* name,const char* content);

	// 刷新redis库的玩家人数
	int refreshPlayerNuToRedis();

	XtSqlClient* getSqlClient(){return m_sqlClient;}
	RedisClient* getCacheRedis(){return m_cacheRc;}

	// 玩家下注入库
	void savePlayerBet(int vid,int uid,int bet_num,int win_flag);

	// 领取下注宝箱
	int getBetLottery(int vid, int uid);

	// 查询下注宝箱
	vector<BetBox> queryBetLottery(int vid, int uid);


public: 
	//处理客户端上行协议数据
	void reciveClientCmd(AllKillClient* client,Jpacket& package);

	//接受客户端连接
	void clientConnect();
	//客户端断开
	void clientClose(AllKillClient* client);

	//客户端登录
	void clientLogin(AllKillClient* client,Jpacket& package);
	//下注
	void clientBet(AllKillClient* client,Jpacket& package);
	//申请上庄
	void clientAskRole(AllKillClient* client,Jpacket& package);
	//申请下庄
	void clientUnRole(AllKillClient* client,Jpacket& package);
	//玩家登出 
	void clientLogOut(AllKillClient* client,Jpacket& package);

	//聊天
	void clientChat(AllKillClient* client,Jpacket& package);

	//发表情
	void clientFace(AllKillClient* client,Jpacket& package);

	//坐下
	void clientSitDown(AllKillClient* client,Jpacket& package);

	//站起
	void clientStandUp(AllKillClient* client,Jpacket& package);

	// 更新玩家信息
	void clientUpdatePlayer(AllKillClient* client,Jpacket& package);

	// 一等获得者列表
	void clientLotterFirstPlayers(AllKillClient* client);

	// 领取下注宝箱(返水)
	void clientGetBetLotter(AllKillClient* client);

	void unicast(AllKillPlayer* player,const std::string& data);
	void broadcast(AllKillPlayer* player,const std::string& data);

public:
	/*配置文件的JSON*/
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



