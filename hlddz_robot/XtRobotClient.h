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
#include "XtCard.h"
#include "XtShuffleDeck.h"


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
		static void tfFirstOut(struct ev_loop* loop, struct ev_timer* w, int events);
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
	    void vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key);
        void map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key);
        //void json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key);
        void json_array_to_vector(std::vector<XtCard> &cards, Json::Value &val, string key);

        void handleRespond(Json::Value& msg); 
        void handleCall(Json::Value& msg); 
        void handleAgainCall(Json::Value& msg); 
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
        void sendFirstCard(void);
        void sendChange(void);
        void sendIdle(void);
        void reset(void);
        //是否队友
        bool isAlliance(void) const;
        //是否跟牌
        bool isFollow(void);
        int getAllianceFollowFace(void) const;
        int getAlliancePassFace(void) const;
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
        std::vector<XtCard>         m_card;                   //底牌
		ev_timer                    m_showTimer;              //叫分延时定时器
		ev_timer                    m_firstOutTimer;          //第一次出牌延时定时器
        ev_timer                    m_outTimer;               //跟牌定时
        ev_timer                    m_changeTimer;            //换桌定时
        ev_timer                    m_idleTimer;              //空闲太久换桌
        XtShuffleDeck               m_deck;                   //牌库

        vector<XtCard>              m_lastCard;               //上轮牌
        int                         m_outid;                  //上轮出牌者id
        int                         m_tid;                    //牌桌id
        int                         m_lord;                   //地主uid
        ///////////////////////////////////////
		struct ev_loop* m_evloop;

		ev_io m_evWrite;
		ev_io m_evRead;

		int m_serverfd;
		int m_uid;                                            //自己uid

		int m_state;
		int m_curHeaderLen;

		char m_headerBuf[sizeof(struct Header)];
		Header* m_header;

		std::string m_body;
		Jpacket m_packet;
		std::list<XtBuffer*> m_writeQueue;

};

#endif /*_XT_ROBOT_CLIENT_H_*/
