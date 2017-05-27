#ifndef _XT_ROBOT_CLIENT_H_
#define _XT_ROBOT_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <list>
#include <set>

#include <ev.h>
#include <json/json.h>

#include "jpacket.h"
#include "XtBuffer.h"
#include "card.h"
#include "shuffledeck.h"


enum XtParseState
{
    XT_PARSE_HEADER    	= 0,
    XT_PARSE_BODY      	= 1,
    XT_PARSE_ERR       	= 2,
    XT_PARSE_CROSS_DOMAIN	= 3,
};

#ifndef XT_MAX_BUF_LEN
#   define XT_MAX_BUF_LEN (1024*8)
#endif

#ifndef XT_DEF_BUF_LEN
#   define XT_DEF_BUF_LEN (1024*8)
#endif

class XtRobotClient 
{
	public:
		static void onReadData(struct ev_loop *loop, struct ev_io *w, int revents);
		static void onWriteData(struct ev_loop *loop, struct ev_io *w, int revents);
        //showTimer 定时器回调函数
		static void tfShow(struct ev_loop* loop, struct ev_timer* w, int events);
		static void tfOut(struct ev_loop* loop, struct ev_timer* w, int events);
		static void tfChange(struct ev_loop* loop, struct ev_timer* w, int events);
		static void tfIdle(struct ev_loop* loop, struct ev_timer* w, int events);

	public:
		int send(const char *buf, unsigned int len);
		int send(const std::string &res);
		unsigned int safeWriten(const char *buf, unsigned int len);

	public:
		int onReciveCmd(Jpacket& cmd);

//////////////////////////////////////////////////////////////////////////////////////////////
	    void vector_to_json_array(std::vector<Card> &cards, Jpacket &packet, string key);
        void map_to_json_array(std::map<int, Card> &cards, Jpacket &packet, string key);
        //void json_array_to_vector(std::vector<Card> &cards, Jpacket &packet, string key);
        void json_array_to_vector(std::vector<Card> &cards, Json::Value &val, string key);
        void jsonArrayToVector(std::vector<int> &change, Json::Value &val, string key);

        void handleRespond(Json::Value& msg); 
        void handleCard(Json::Value& msg); 
        void handleCall(Json::Value& msg); 
        void handleGrab(Json::Value& msg); 
        void handleResultGrab(Json::Value& msg); 
        void handleDouble(Json::Value& msg); 
        void handleOut(Json::Value& msg);
        void handleAgainOut(Json::Value& msg);
        void handleReprepare(Json::Value& msg);
        void handleEnd(Json::Value& msg);
        void handleKick(Json::Value& msg);
        void handleTime(Json::Value& msg);
        void handleLogin(Json::Value& msg);
        void handlePrepare(Json::Value& msg);

        void sendCall(void);
        void sendCard(void);
        void sendChange(void);
        void sendIdle(void);
//////////////////////////////////////////////////////////////////////////////////////////////

	public:

		void doLogin();
		void sendLoginPackage();
	public:
		int connectToServer(const char* ip,int port,int uid);
		int closeConnect();

	public:
		XtRobotClient(struct ev_loop* evloop);
		~XtRobotClient();

	private:
        ///////////////////////////////////////
        std::vector<Card>           m_card;                   //底牌
		ev_timer                    m_showTimer;              //第一次出牌延时定时器
        ev_timer                    m_outTimer;               //出牌定时
        ev_timer                    m_changeTimer;            //换桌定时
        ev_timer                    m_idleTimer;              //空闲
        Shuffledeck                 m_deck;                   //牌库

        vector<Card>                m_lastCard;               //上轮牌
        int                         m_outid;                  //上轮出牌者id
        int                         m_tid;                    //牌桌id
        std::set<int>               m_playerlist;             //玩家id队列
        ///////////////////////////////////////
		struct ev_loop* m_evloop;

		ev_io m_evWrite;
		ev_io m_evRead;

		int m_serverfd;
		/*game info */
		int m_uid;

		/* parse data */
		int m_state;
		int m_curHeaderLen;

		char m_headerBuf[sizeof(struct Header)];
		Header* m_header;

		std::string m_body;
		Jpacket m_packet;
		std::list<XtBuffer*> m_writeQueue;

};

#endif /*_XT_ROBOT_CLIENT_H_*/
