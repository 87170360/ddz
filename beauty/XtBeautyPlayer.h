#ifndef _XT_BEAUTY_PLAYER_H_
#define _XT_BEAUTY_PLAYER_H_

#include <string>

#include "XtGamePlayer.h"
#include "XtBeautyMacros.h"

class XtGameServer;
class XtBeautyGame;
class XtRedisClient;


class XtBeautyPlayer :public XtGamePlayer
{
	public:
		void setData(int uid,XtGameServer* server,XtBeautyGame* game);

		int getUid();

		int incMoney(int value);
		int decMoney(int value);


		int updateMoney();

		int getMoney();

		int getMoneyLockTime();

		const char* getName()
		{
			return m_name.c_str();
		}

		const char* getAvatar()
		{
			return m_avatar.c_str();
		}

		int getSex()
		{
			return m_sex;
		}

		void resetSeatBet();

		void addSeatBet(int seat_id,int value);
		int getSeatBet(int seat_id);

		void setBetMoneyResult(int money){m_resultMoney=money;}
		int getBetMoneyResult(){return m_resultMoney;}
		void addBetMoneyResult(int value){m_resultMoney+=value;}

	private:
		/* player info */
		int m_uid;
		std::string m_name;
		std::string m_avatar;
		int m_money;
		int m_sex;


		/* player seat bet */
		int m_seatBetNu[BG_SEAT_ID_NU];
		int m_resultMoney;




		XtRedisClient* m_datarc;
		XtBeautyGame* m_game;
		XtGameServer* m_server;

};

#endif /*_XT_BEAUTY_PLAYER_H_*/



