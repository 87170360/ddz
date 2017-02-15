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

#include <map>


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



class SimplyPlayer
{		
public:
	int 				index;      //redis数组下标
	// table router information     
	int					vid;        //场馆id
	int					tid;        //桌子id
	int					seatid;     //坐位id

	// player information 
	int                 uid;       
	std::string			skey;
	std::string			name;
	int					sex;
	std::string			avatar;        ///头像
	std::string			birthday;
	std::string			zone;
	int					exp;
	int					rmb;
	int					money;         ///金币
	int					coin;
	int					total_board;   ///总次数
	int					total_win;     ///赢场数
	int					pcount;        ///play count (玩N局奖励，领钱时reset)
	int					vtime;
	int					vlevel;
	std::string			ps;

	int                 max_win_money;    //最大赢取 
	int                 best_board;       //最大牌类型
	std::string         best_board_detail;//牌详情，格式为以逗号分隔的16进制数(01,02,03,04,05) 
};


///---单个机器人玩家
class XtRobotClient 
{
	public:
		static void onReadData(struct ev_loop *loop, struct ev_io *w, int revents);
		static void onWriteData(struct ev_loop *loop, struct ev_io *w, int revents);
		///弃牌
		static void onDoFold(struct ev_loop* loop,struct ev_timer* w,int events);
		static void onDoFollow(struct ev_loop* loop,struct ev_timer* w,int events);  ///跟牌
		static void onDoSee(struct ev_loop* loop,struct ev_timer* w,int events);     ///看牌
		static void onDoCompare(struct ev_loop* loop,struct ev_timer* w,int events); ///比牌
		static void onDoAllIn(struct ev_loop* loop,struct ev_timer* w,int events);   ///

		static void onChoiceCard(struct ev_loop* loop,struct ev_timer* w,int events);
		static void onGrabBanker(struct ev_loop* loop,struct ev_timer* w,int events);
		static void onDoubleBet(struct ev_loop* loop,struct ev_timer* w,int events);
		
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

		 //处理首次发牌
		void handleSendCard(Json::Value& cmd); 

		//二次发牌
		void handleSecondCard(Json::Value& cmd);

		//处理定庄
		void handleMakeBanker(Json::Value& cmd);

		//处理房间信息
		void addPlayer(Json::Value& cmd);

	public:

		void doLogin();       ///---登录
		void doFold();        ///---弃牌
		void doFollow();      ///---跟注 
		void doSee();         ///---看牌  
		void doChangeTable(); ///---换桌 
		void doCompare();     ///---比牌
		void doAllIn();       ///---全跟

		void doChoiceCard();  //选牌

		void doGrabBanker();  //抢庄

		void doDoubleBet();   //下注

		void sendLoginPackage();

		void sendFoldPackage();
		void sendFollowPackage();
		void sendSeePackage();
		void sendChangeTablePackage();
		void sendComparePacket();
		void sendAllInPacket();

		void sendChoiceCardPacket();

		//抢庄
		void sendGrabBankerPacket();

		//下注
		void sendDoubleBetPacket();



	public:
		int connectToServer(const char* ip,int port,int uid);
		int closeConnect();

		int getTargetSeatId();


	public:
		XtRobotClient(struct ev_loop* evloop);
		~XtRobotClient();


	private:
		struct ev_loop* m_evloop;

		ev_io m_evWrite;
		ev_io m_evRead;
		ev_timer m_evFollowTimer;
		ev_timer m_evSeeTimer;
		ev_timer m_evFoldTimer;
		ev_timer m_evCompareTimer;
		ev_timer m_evAllInTimer;

		ev_timer m_evChoiceCardTimer;   //组牌

		ev_timer m_evGrabBankerTimer;   //抢庄

		ev_timer m_evDoubleBetTimer;    //下注

		int m_serverfd;


		/*game info */
		int m_uid;
		bool m_isBetting;
		bool m_hasSee;
		bool m_isAllIn;
		int m_seatid;
		int m_cardType;
		int m_maxRound;
		int m_seatBettingInfo[5];   ///桌子是否Betting: (0:无人玩 1:有人玩)
		int m_curRound;

		unsigned char m_mycard[5];   //我的手牌


		/* parse data */
		int m_state;
		int m_curHeaderLen;

		char m_headerBuf[sizeof(struct Header)];
		Header* m_header;

		std::string m_body;
		Jpacket m_packet;


		std::list<XtBuffer*> m_writeQueue;

		//玩家map(uid->SimplyPlayer)
		std::map<int, SimplyPlayer*> mPlayerMap;
		int mBankerUid;
		int base_money;   //房间底注

};







#endif /*_XT_ROBOT_CLIENT_H_*/





