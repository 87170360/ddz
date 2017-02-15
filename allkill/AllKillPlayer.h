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
	void setBanned(int banned);           // ���ý���
	int getBanned();   
	int mBanned;                         // ���Ա��
	unsigned long long  last_speak_time; // ����ʱ��
	int mQuickSpeakTimes;                // ���ٷ��Դ���

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
	int getTmpSeatBet(int seat_id);   //�м���ʱ��


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
	bool m_isUnRole;  //��ׯ�ұ��



	/* player seat bet */
	int m_seatBetNu[AK_SEAT_ID_NU];  //����ڸ���λ�õ���ע��
	int m_betMoneyResult;

	int m_tmpSeatBetNu[AK_SEAT_ID_NU];  //��ʱ���


	RedisClient* m_datarc;
	AllKillGame* m_game;
	AllKillServer* m_server;

public:
	//++������ĸ�λ��  (��0��ʼ)
	int m_seatId;

	//++���ڳ�id
	int m_vid; 

	//++��Ӯ����
	int m_winCount;

};

#endif /*_ALL_KILL_PLAYER_H_*/

