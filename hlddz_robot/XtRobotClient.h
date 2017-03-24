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

#include <ev.h>
#include <json/json.h>

#include "jpacket.h"
#include "XtBuffer.h"
#include "XtCard.h"


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

		static void onDoFold(struct ev_loop* loop,struct ev_timer* w,int events);
		static void onDoFollow(struct ev_loop* loop,struct ev_timer* w,int events);
		static void onDoSee(struct ev_loop* loop,struct ev_timer* w,int events);
		static void onDoCompare(struct ev_loop* loop,struct ev_timer* w,int events);
		static void onDoAllIn(struct ev_loop* loop,struct ev_timer* w,int events);

        //showtimer 定时器回调函数
		static void tfShow(struct ev_loop* loop, struct ev_timer* w, int events);

	public:
		int send(const char *buf, unsigned int len);
		int send(const std::string &res);
		unsigned int safeWriten(const char *buf, unsigned int len);

	public:
		int onReciveCmd(Jpacket& cmd);

		void handleGameStart(Json::Value& cmd);
		void handleGameEnd(Json::Value& cmd);
		void handleGameNextBet(Json::Value& cmd);
		void handleBetBc(Json::Value& cmd);
		void handleTableInfo(Json::Value& cmd);
		void handleRobotChange(Json::Value& cmd);

//////////////////////////////////////////////////////////////////////////////////////////////
	    void vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key);
        void map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key);
        void json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key);
        void json_array_to_vector(std::vector<XtCard> &cards, Json::Value &val, string key);

        void handleRespond(Json::Value& msg); 
        void handleCall(Json::Value& msg); 
        void handleAgainCall(Json::Value& msg); 
        void handleDouble(Json::Value& msg); 
        void handleAgainDouble(Json::Value& msg); 
        void handleOut(Json::Value& msg);
        void handleAgainOut(Json::Value& msg);
        void handleReprepare(Json::Value& msg);

        void sendCall(void);
//////////////////////////////////////////////////////////////////////////////////////////////

	public:

		void doLogin();
		void doFold();
		void doFollow();
		void doSee();
		void doChangeTable();
		void doCompare();
		void doAllIn();


		void sendLoginPackage();
		void sendFoldPackage();
		void sendFollowPackage();
		void sendSeePackage();
		void sendChangeTablePackage();
		void sendComparePacket();
		void sendAllInPacket();


	public:
		int connectToServer(const char* ip,int port,int uid);
		int closeConnect();

		int getTargetSeatId();


	public:
		XtRobotClient(struct ev_loop* evloop);
		~XtRobotClient();


	private:
        ///////////////////////////////////////
        std::vector<XtCard>         m_card;                   //底牌
		ev_timer                    m_showTimer;              //出牌动画时间结束后再出牌
        ///////////////////////////////////////
		struct ev_loop* m_evloop;

		ev_io m_evWrite;
		ev_io m_evRead;
		ev_timer m_evFollowTimer;
		ev_timer m_evSeeTimer;
		ev_timer m_evFoldTimer;
		ev_timer m_evCompareTimer;
		ev_timer m_evAllInTimer;

		int m_serverfd;
		/*game info */
		int m_uid;
		bool m_isBetting;
		bool m_hasSee;
		bool m_isAllIn;
		int m_seatid;
		int m_cardType;
		int m_maxRound;
		int m_seatBettingInfo[5];
		int m_curRound;



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





