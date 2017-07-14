#include <assert.h>
#include "PartyPlayer.h"
#include "PartyMacros.h"
#include "PartyGame.h"
#include "PartyServer.h"
#include "redis_client.h"
#include "log.h"

extern Log xt_log;

static int red_data[] = {static_cast<int>(AWARD_1), static_cast<int>(AWARD_4), static_cast<int>(AWARD_7), static_cast<int>(AWARD_10)};
static std::set<int> red_animal(red_data, red_data + 4);

static int yellow_data[] = {static_cast<int>(AWARD_2), static_cast<int>(AWARD_5), static_cast<int>(AWARD_8), static_cast<int>(AWARD_11)};
static std::set<int> yellow_animal(yellow_data, yellow_data + 4);

static int green_data[] = {static_cast<int>(AWARD_3), static_cast<int>(AWARD_6), static_cast<int>(AWARD_9), static_cast<int>(AWARD_12)};
static std::set<int> green_animal(green_data, green_data + 4);

#ifdef AK_DEBUG 
int PartyPlayer::ms_objectNu=0;
#endif 

PartyPlayer::PartyPlayer() :
    m_uid(-1),
    m_name(""),
    m_avatar(""),
    m_money(0),
    m_sex(-1),
    m_vlevel(0),
    m_datarc(NULL),
    m_game(NULL),
    m_server(NULL),
    m_totalReward(0),
    m_rewardStamp(0),
    m_reward(0),
    m_rewardLam(0),
    m_rewardLig(0),
    m_auto(false),
    m_repeatFollow(false)
{
#ifdef AK_DEBUG
    ms_objectNu++;
#endif 
}

PartyPlayer::~PartyPlayer()
{
#ifdef AK_DEBUG 
    ms_objectNu--;
#endif 

}

void PartyPlayer::setData(int uid,PartyServer* server,PartyGame* game)
{
    m_uid=uid;
    m_server=server;
    m_game=game;
    m_datarc=m_server->getDataRedis(uid);


    int ret = m_datarc->command("hgetall hu:%d", uid);
    if (ret < 0) 
    {
        xt_log.error("update info error, because get player infomation error.\n");
        return ;
    }

    if (m_datarc->is_array_return_ok() < 0) 
    {
        xt_log.error("update info error, because get player infomation error.\n");
        return ;
    }

    m_avatar=m_datarc->get_value_as_string("avatar");
    m_name=m_datarc->get_value_as_string("name");
    m_money=m_datarc->get_value_as_int("money");
    m_sex=m_datarc->get_value_as_int("sex");
    m_vlevel=m_datarc->get_value_as_int("vlevel");
    
    m_rewardStamp = m_datarc->get_value_as_int("party_stamp");
    m_totalReward = m_datarc->get_value_as_int("party_award");
    //printf("load redis, party_stamp: %ld, party_award: %d\n", m_rewardStamp, m_totalReward);
}


int PartyPlayer::setTimeLock(int value)
{
    int now=time(NULL)+value;

    int ret=m_datarc->command("hset hu:%d mlock_time %d",m_uid,now);
    return  ret;
}



int PartyPlayer::getMoney()
{
#ifdef AK_DEBUG 
    if(m_uid<VALID_CLIENT_UID_MIN)
    {
        return 20000000;
    }
    return m_money;

#else 
    return m_money;
#endif


}




int PartyPlayer::updateMoney()
{

#ifdef AK_DEBUG 
    if(m_uid<VALID_CLIENT_UID_MIN)
    {
        return 20000000;
    }
#endif 

    int ret;
    ret=m_datarc->command("hget hu:%d money",m_uid);
    if(ret<0)
    {
        xt_log.error("update money error\n");
        return -1;
    }

    m_money=atoi(m_datarc->reply->str);
    //printf("player(%d),money is %d\n",m_uid,m_money);

    return m_money;
}

int PartyPlayer::incMoney(int value)
{
#ifdef AK_DEBUG 
    if(m_uid<VALID_CLIENT_UID_MIN)
    {
        return 0;
    }
#endif 
    assert(value>=0);
    int ret;
    ret=m_datarc->command("hincrby hu:%d money %d",m_uid,value);
    if(ret<0)
    {
        xt_log.error("incr money error\n");
        return -1;
    }
    m_money=m_datarc->reply->integer;
    return 0;
}

int PartyPlayer::decMoney(int value)
{

#ifdef AK_DEBUG 
    if(m_uid<VALID_CLIENT_UID_MIN)
    {
        return 0;
    }
#endif 

    assert(value>=0);
    int ret;
    ret=m_datarc->command("hincrby hu:%d money -%d",m_uid,value);

    if(ret<0)
    {
        xt_log.error("decr money error\n");
        return -1;
    }
    m_money=m_datarc->reply->integer;
    return 0;
}

bool PartyPlayer::checkBanned(void)
{
    if(m_uid<VALID_CLIENT_UID_MIN)
    {
        return false;
    }

    int ret = m_datarc->command("hget hu:%d banned", m_uid);
    if(ret < 0)
    {
        xt_log.error("get banned error\n");
        return false;
    }

    int banned = atoi(m_datarc->reply->str);
    return (banned != 0);
}

void PartyPlayer::procReward(void)
{
    //送灯
    const std::set<int>& aw_set = m_game->getLamp();
    //单向奖，特殊奖
    int aw_id = m_game->getAwid();
    int ml = m_game->getMingLevel();

    for(std::map<int, int>::const_iterator it = m_bet.begin(); it != m_bet.end(); ++it)
    {
        //spcial single
        if((m_game->isSpecialAward(m_game->getAwid()) && hitSpecial(it->first)) || (it->first == aw_id))
        {
            //double prob = m_game->getProba(aw_id, ml);
            //特殊奖赔率即各自的赔率
            double prob = m_game->getProba(it->first, ml);
            m_reward += prob * it->second; 
        }
        //lamp
        int tmpxx = it->first;
        if(aw_set.find(tmpxx) != aw_set.end())
        {
            double prob = m_game->getProba(it->first, ml);
            m_rewardLam += prob * it->second; 
        }
    }

    //lightning
    if(m_game->getLightning())
    {
        m_rewardLig = m_reward;
    }

    if(m_reward > 0 || m_rewardLam > 0)
    {
        incMoney((m_reward + m_rewardLig + m_rewardLam) * (1 - g_tax)); 
        addTotalReward();
    }
}

bool PartyPlayer::hitSpecial(int bet_aw)
{
    int aw_id = m_game->getAwid();
    if(aw_id == AWARD_13_1 && bet_aw >= AWARD_1 && bet_aw <= AWARD_3)
    {
        return true;
    }
    if(aw_id == AWARD_13_2 && bet_aw >= AWARD_4 && bet_aw <= AWARD_6)
    {
        return true;
    }
    if(aw_id == AWARD_13_3 && bet_aw >= AWARD_7 && bet_aw <= AWARD_9)
    {
        return true;
    }
    if(aw_id == AWARD_13_4 && bet_aw >= AWARD_10 && bet_aw <= AWARD_12)
    {
        return true;
    }

    if(aw_id == AWARD_14_1 && red_animal.find(bet_aw) != red_animal.end())
    {
        return true;
    }
    if(aw_id == AWARD_14_2 && yellow_animal.find(bet_aw) != yellow_animal.end())
    {
        return true;
    }
    if(aw_id == AWARD_14_3 && green_animal.find(bet_aw) != green_animal.end())
    {
        return true;
    }
    return false;
}

void PartyPlayer::reset(void)
{
    if(!m_bet.empty())
    {
        m_his           = m_bet;
    }
    m_reward        = 0;
    m_rewardLam     = 0;
    m_rewardLig     = 0;
    m_repeatFollow  = false;
    m_bet.clear();
    //xt_log.info("player reset, his size: %d\n", m_his.size());
}

int PartyPlayer::bet(int aw_id, int num)
{
    if(m_bet.find(aw_id) != m_bet.end())
    {
        m_bet[aw_id] += num; 
    } 
    else
    {
        m_bet[aw_id] = num;
    }

    //xt_log.info("bet: awid: %d, num: %d\n", aw_id, num);
    return m_bet[aw_id];
}
        
void PartyPlayer::cancleBet(void) 
{
    m_bet.clear();
    m_repeatFollow = false;
}

int PartyPlayer::getCurrentTotalBet(void)
{
    int total = 0;
    for(std::map<int, int>::const_iterator it = m_bet.begin(); it != m_bet.end(); ++it)
    {
        total += it->second; 
    }
    return total;
}

int PartyPlayer::getHisoryTotalBet(void)
{
    int total = 0;
    for(std::map<int, int>::const_iterator it = m_his.begin(); it != m_his.end(); ++it)
    {
        total += it->second; 
    }
    return total;
}

void PartyPlayer::useHistoryBet(void)
{
    m_bet = m_his;
    m_repeatFollow = true;
}
        
int PartyPlayer::getPureReward(void)
{
    int totalWin = m_reward + m_rewardLam + m_rewardLig; 
    int totalBet = getCurrentTotalBet();
    return totalWin - totalBet;
}
        
void PartyPlayer::addTotalReward(void)
{
    long int cur_time = static_cast<long int>(time(NULL));
    //是否同一天
    //算税前
    if(isSameDay(cur_time, m_rewardStamp))
    {
        m_totalReward += m_reward + m_rewardLig + m_rewardLam;
        //printf("same date, party_stamp: %ld, party_award: %d\n", m_rewardStamp, m_totalReward);
    }
    else
    {
        m_totalReward = m_reward + m_rewardLig + m_rewardLam;
        m_rewardStamp = cur_time;
        //printf("diff date, party_stamp: %ld, party_award: %d\n", m_rewardStamp, m_totalReward);
    }
    setTotalReward2Redis();
}
        
void PartyPlayer::initPlayerLogin(void)
{
    if(m_uid < VALID_CLIENT_UID_MIN)
    {
        return; 
    }

    long int cur_time = static_cast<long int>(time(NULL));
    //如果这里不初始化一次，非当天的数据会上排行榜
    if(!isSameDay(cur_time, m_rewardStamp))
    {
        m_rewardStamp = cur_time;
        m_totalReward = 0;
        setTotalReward2Redis();
    }
}
        
bool PartyPlayer::isSameDay(long int stamp1, long int stamp2)
{
    return (stamp1 / 86400) == (stamp2 / 86400);
}
        
void PartyPlayer::setTotalReward2Redis(void)
{
    if(m_uid < VALID_CLIENT_UID_MIN)
    {
        return; 
    }

    int ret = m_datarc->command("hset hu:%d party_stamp %ld", m_uid, m_rewardStamp);
    if(ret < 0)
    {
        xt_log.error("PartyPlayer setTotalReward2Redis error(hset hu:%d, party_stamp %ld).\n", m_uid, m_rewardStamp);
    }

    ret = m_datarc->command("hset hu:%d party_award %d", m_uid, m_totalReward);
    if(ret < 0)
    {
        xt_log.error("PartyPlayer setTotalReward2Redis error(hset hu:%d, party_award %ld).\n", m_uid, m_totalReward);
    }
}
