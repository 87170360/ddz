#ifndef _XT_BEAUTY_SERVER_H_
#define _XT_BEAUTY_SERVER_H_

#include "XtGameServer.h"


class XtBeautyServer:public XtGameServer 
{
	public:
		virtual XtGameLogic* onCreateGame();
		virtual XtGameClient* onCreateClient();

	public:
		int refreshPlayerNuToRedis();
};





#endif /*_XT_BEAUTY_SERVER_H_*/

