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
	void setBanned(int banned);           // 设置禁言
	int getBanned();   
	int mBanned;                         // 禁言标记
	unsigned long long  last_speak_time; // 发言时间
	int mQuickSpeakTimes;                // 快速发言次数

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
	int getTmpSeatBet(int seat_id);   //中间临时的


	/* bet money */
	void addBetMoneyResult(int value);
	void decBetMoneyResult(int value);
	int getBetMoneyResult();
	void setBetMoneyResult(int value);

	/* venues id */
	void setVid(int vid) 
	{ 
		m_vid = vid; 
	}

	int getVid() 
	{
		return m_vid;
	}

private:
	/* player info */
	int m_uid;
	std::string m_name;
	std::string m_avatar;
	int m_money;
	int m_sex;


	/* player is  role */
	bool m_isUnRole;  //非庄家标记



	/* player seat bet */
	int m_seatBetNu[AK_SEAT_ID_NU];  //玩家在各个位置的下注数
	int m_betMoneyResult;

	int m_tmpSeatBetNu[AK_SEAT_ID_NU];  //临时存放


	RedisClient* m_datarc;
	AllKillGame* m_game;
	AllKillServer* m_server;

public:
	//++玩家坐哪个位置  (从0开始)
	int m_seatId;

	//++所在场id
	int m_vid; 

	//++连赢次数
	int m_winCount;

};

#endif /*_ALL_KILL_PLAYER_H_*/

