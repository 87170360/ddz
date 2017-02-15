#ifndef _XT_GAME_LOGIC_H_
#define _XT_GAME_LOGIC_H_

#include <json/json.h>


class XtGamePlayer;
class XtJsonPacket ;
class XtGameServer;




class XtGameLogic
{
	public:
		XtGameLogic();
		virtual ~XtGameLogic();

	public:
		virtual int configGame(Json::Value* value)=0;
		virtual int start(XtGameServer* server,struct ev_loop* loop)=0;
		virtual int shutDown()=0;

		virtual void onReciveClientCmd(XtGamePlayer* player,XtJsonPacket* packet)=0;

		virtual XtGamePlayer* getPlayer(int uid)=0;
		virtual int playerLogin(XtGamePlayer* player)=0;
		virtual int playerLogout(XtGamePlayer* player)=0;
};


#endif /*_XT_GAME_LOGIC_H_*/

