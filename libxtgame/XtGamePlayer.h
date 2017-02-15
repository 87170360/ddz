#ifndef _XT_GAME_PLAYER_H_
#define _XT_GAME_PLAYER_H_

class XtGameClient;

class XtGamePlayer 
{
	public:
		XtGamePlayer(){};
		virtual ~XtGamePlayer(){};

	public:
		virtual int getUid()=0;


	public:
		void setClient(XtGameClient* client)
		{
			m_client=client;
		}

		XtGameClient* getClient(XtGameClient* client)
		{
			return m_client;
		}

	protected:
		XtGameClient* m_client;
};


#endif /*_XT_GAME_PLAYER_H_*/


