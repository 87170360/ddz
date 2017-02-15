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
	int 				index;      //redis�����±�
	// table router information     
	int					vid;        //����id
	int					tid;        //����id
	int					seatid;     //��λid

	// player information 
	int                 uid;       
	std::string			skey;
	std::string			name;
	int					sex;
	std::string			avatar;        ///ͷ��
	std::string			birthday;
	std::string			zone;
	int					exp;
	int					rmb;
	int					money;         ///���
	int					coin;
	int					total_board;   ///�ܴ���
	int					total_win;     ///Ӯ����
	int					pcount;        ///play count (��N�ֽ�������Ǯʱreset)
	int					vtime;
	int					vlevel;
	std::string			ps;

	int                 max_win_money;    //���Ӯȡ 
	int                 best_board;       //���������
	std::string         best_board_detail;//�����飬��ʽΪ�Զ��ŷָ���16������(01,02,03,04,05) 
};


///---�������������
class XtRobotClient 
{
	public:
		static void onReadData(struct ev_loop *loop, struct ev_io *w, int revents);
		static void onWriteData(struct ev_loop *loop, struct ev_io *w, int revents);
		///����
		static void onDoFold(struct ev_loop* loop,struct ev_timer* w,int events);
		static void onDoFollow(struct ev_loop* loop,struct ev_timer* w,int events);  ///����
		static void onDoSee(struct ev_loop* loop,struct ev_timer* w,int events);     ///����
		static void onDoCompare(struct ev_loop* loop,struct ev_timer* w,int events); ///����
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

		 //�����״η���
		void handleSendCard(Json::Value& cmd); 

		//���η���
		void handleSecondCard(Json::Value& cmd);

		//����ׯ
		void handleMakeBanker(Json::Value& cmd);

		//��������Ϣ
		void addPlayer(Json::Value& cmd);

	public:

		void doLogin();       ///---��¼
		void doFold();        ///---����
		void doFollow();      ///---��ע 
		void doSee();         ///---����  
		void doChangeTable(); ///---���� 
		void doCompare();     ///---����
		void doAllIn();       ///---ȫ��

		void doChoiceCard();  //ѡ��

		void doGrabBanker();  //��ׯ

		void doDoubleBet();   //��ע

		void sendLoginPackage();

		void sendFoldPackage();
		void sendFollowPackage();
		void sendSeePackage();
		void sendChangeTablePackage();
		void sendComparePacket();
		void sendAllInPacket();

		void sendChoiceCardPacket();

		//��ׯ
		void sendGrabBankerPacket();

		//��ע
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

		ev_timer m_evChoiceCardTimer;   //����

		ev_timer m_evGrabBankerTimer;   //��ׯ

		ev_timer m_evDoubleBetTimer;    //��ע

		int m_serverfd;


		/*game info */
		int m_uid;
		bool m_isBetting;
		bool m_hasSee;
		bool m_isAllIn;
		int m_seatid;
		int m_cardType;
		int m_maxRound;
		int m_seatBettingInfo[5];   ///�����Ƿ�Betting: (0:������ 1:������)
		int m_curRound;

		unsigned char m_mycard[5];   //�ҵ�����


		/* parse data */
		int m_state;
		int m_curHeaderLen;

		char m_headerBuf[sizeof(struct Header)];
		Header* m_header;

		std::string m_body;
		Jpacket m_packet;


		std::list<XtBuffer*> m_writeQueue;

		//���map(uid->SimplyPlayer)
		std::map<int, SimplyPlayer*> mPlayerMap;
		int mBankerUid;
		int base_money;   //�����ע

};







#endif /*_XT_ROBOT_CLIENT_H_*/





