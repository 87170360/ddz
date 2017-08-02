#ifndef _ALL_KILL_PLAYER_H_
#define _ALL_KILL_PLAYER_H_

#include <string>
#include "AllKillMacros.h"

class RedisClient;
class AllKillGame;
class AllKillServer;



class AllKillPlayer 
{

#ifdef  AK_DEBUG
	public:
		static int ms_objectNu;
#endif 

	public:
		AllKillPlayer();
		~AllKillPlayer();

	public:
		void setData(int uid,AllKillServer* server,AllKillGame* game);
		int getUid(){return m_uid;}

		int incMoney(int value);
		int decMoney(int value);

		void setUnRole(bool value){m_isUnRole=value;}
		bool getIsUnRole(){return m_isUnRole;}


		int setTimeLock(int time);

		int updateMoney();

		int getMoney();

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


		/* bet money */
		void addBetMoneyResult(int value);
		void decBetMoneyResult(int value);
		int getBetMoneyResult();
		void setBetMoneyResult(int value);

		//lsx
		//check
		bool checkBanned(void);
        int getAllKillTotalBet(void);

		void setDeskSeatid(int iSeatid){ m_iDeskSeatid = iSeatid; }
		int getDeskSeatid(){ return m_iDeskSeatid; }

	private:
		/* player info */
		int m_uid;
		std::string m_name;
		std::string m_avatar;
		int m_money;
		int m_sex;


		/* player is  role */
		bool m_isUnRole;

		int m_iDeskSeatid;

		/* player seat bet */
		int m_seatBetNu[AK_SEAT_ID_NU];
		int m_betMoneyResult;



		RedisClient* m_datarc;
		AllKillGame* m_game;
		AllKillServer* m_server;


};

#endif /*_ALL_KILL_PLAYER_H_*/

