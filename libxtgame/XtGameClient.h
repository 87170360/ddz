#ifndef _XT_GAME_CLIENT_H_
#define _XT_GAME_CLIENT_H_


#include "XtStreamClient.h"

#define XT_GAME_CLINET_ENCRYPT_KEY 13


class XtGameServer;
class XtGamePlayer;
class XtJsonPacket;
class XtMsgProtocol;
class XtProtocol;

///---游戏客户端相关协议数据读/写
class XtGameClient:public XtStreamClient 
{

	public:
		XtGameClient(struct ev_loop* loop,XtGameServer* server);
		virtual ~XtGameClient();

	public:
		int send(const void* data,unsigned int length);

	public:
		void setUid(int uid){m_uid=uid;}   ///设置用户ID
		int getUid(){return m_uid;}

		void setPlayer(XtGamePlayer* player);
		XtGamePlayer* getPlayer();

		void setProtocol(XtProtocol* protocol);
		XtProtocol* getProtocol();

	public:
		virtual void onReciveData(void* data,int len);
		virtual void onClose();

	public:
		virtual void onRecivePacket(XtBuffer** packet,int num);



	private:
		int m_uid;
		XtProtocol*   m_protocol;
		XtGameServer* m_server;
		XtGamePlayer* m_player;

};

#endif /*_XT_GAME_CLIENT_H_*/

