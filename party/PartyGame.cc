#include <assert.h>
#include <math.h>
#include <sstream>
#include "PartyGame.h"
#include "PartyMacros.h"
#include "PartyPlayer.h"
#include "PartyServer.h"
#include "log.h"

extern Log xt_log;

//测试， 只能开一个
const static bool g_testLamp        = false;   //送灯
const static bool g_testDouble      = false;   //双倍(闪电)
const static bool g_testSpecial     = false;   //特殊
const static bool g_closeSpecial    = false;    //关闭特殊奖

static char g_buf[1024];
const static int g_proba[19][3] = 
{
    {6, 7, 8},      
    {4, 4, 5},      
    {5, 6, 7},      
    {13, 13, 15},   
    {8, 7, 10},     
    {9, 10, 12}, 
    {22, 24, 26},   
    {15, 17, 20},   
    {17, 21, 23},   
    {35, 40, 46},   
    {27, 27, 31}, 
    {30, 33, 40},   

    {15, 17, 20},   
    {30, 30, 37},   
    {54, 62, 69}, 
    {92, 100, 117}, 

    {54, 55, 66},   
    {61, 70, 82},   
    {76, 84, 95}
}; 

const static int g_weight[12][3] = 
{
    {250, 250, 200},      
    {200, 166, 142},      
    {166, 142, 125},      
    {125, 142, 100},     
    {111, 100, 83}, 
    {76, 76, 66},   
    {66, 58, 50},   
    {58, 47, 43},   
    {45, 41, 38},   
    {37, 37, 32}, 
    {33, 30, 25},   
    {28, 25, 21}
}; 

const static int g_total_weight[3] = {1195, 1114, 925};

const static int g_animalPos[24]    = 
{
    3, 7, 11, 15, 19, 23,
    2, 6, 10, 14, 18, 22,
    1, 5, 9, 13, 17, 21,
    0, 4, 8, 12, 16, 20,
};

const static int g_maxProbPos = 12;
//奖项赔率显示顺序
const int g_probPos[g_maxProbPos] = 
{
    static_cast<int>(AWARD_10),
    static_cast<int>(AWARD_12),
    static_cast<int>(AWARD_11),
    static_cast<int>(AWARD_7),
    static_cast<int>(AWARD_9),
    static_cast<int>(AWARD_8),
    static_cast<int>(AWARD_4),
    static_cast<int>(AWARD_6),
    static_cast<int>(AWARD_5),
    static_cast<int>(AWARD_1),
    static_cast<int>(AWARD_3),
    static_cast<int>(AWARD_2),
};

PartyGame::PartyGame() : 
    m_server(NULL), 
    m_evLoop(NULL), 
    m_ming(g_party_ming), 
    m_an(g_party_an), 
    m_tax(0), 
    m_timeCount(PARTY_TIMER_PREPARE),
    m_spec_total(0),
    m_aw_id(AWARD_1),
    m_light(false),
    m_openLevel(ODD_1),
    m_betLimit(0),
    m_vid(0),
    m_winMoney(0),
    m_first(true),
    m_totalIn(0),
    m_totalOut(0),
    m_awardPeople(0),
    m_party_compare(0),
    m_party_range1(0),
    m_party_range2(0),
    m_party_water(0),
    m_party_check(0),
    m_award_stamp(0),
    m_check_stamp(0),
    m_last_an(g_party_an),
    m_ctrl_1(0),
    m_ctrl_2(0),
    m_ctrl_pro(0)
{
}

PartyGame::~PartyGame()
{
}

int PartyGame::configGame(Json::Value& value)
{
    m_tbname = value["info"]["tbname"].asString();
    m_vid = value["info"]["vid"].asInt();

    return 0;
}

int PartyGame::start(PartyServer* server, struct ev_loop* loop)
{
	srand(time(NULL));

    m_server = server;
    m_evLoop = loop;

    m_timer_start.data = this;
    ev_timer_init(&m_timer_start, PartyGame::onTimerStart, 0, PARTY_TIMER_START);

    m_timer_end.data = this;
    ev_timer_init(&m_timer_end, PartyGame::onTimerEnd, 0, PARTY_TIMER_END);

    m_timer_update.data = this;
    ev_timer_init(&m_timer_update, PartyGame::onTimerUpdate, 0, PARTY_TIMER_UPDATE);

    m_timer_prepare.data = this;
    ev_timer_init(&m_timer_prepare, PartyGame::onTimerPrepare, 0, PARTY_TIMER_PREPARE);

    ev_timer_again(m_evLoop, &m_timer_prepare);
    ev_timer_again(m_evLoop, &m_timer_update);

    randomShow();

    updateConfig();

    return 0;
}

void PartyGame::onTimerEnd(struct ev_loop* loop,struct ev_timer* w,int revents)
{
    PartyGame* game=(PartyGame*)w->data;
    game->timerEnd();
}

void PartyGame::timerEnd(void)
{
    //xt_log.info("timerEnd over \n");
    ev_timer_stop(m_evLoop, &m_timer_end);
    endTimeup();
    //xt_log.info("timerStart start \n");
    ev_timer_again(m_evLoop, &m_timer_prepare);
}

void PartyGame::onTimerStart(struct ev_loop* loop,struct ev_timer* w,int revents)
{
    PartyGame* game=(PartyGame*)w->data;
    game->timerStart();
}

void PartyGame::timerStart(void)
{
    //xt_log.info("timerStart over \n");
    ev_timer_stop(m_evLoop, &m_timer_start);
    //ev_timer_stop(m_evLoop, &m_timer_update);
    //xt_log.info("timerEnd start \n");
    startTimeup();
    ev_timer_again(m_evLoop, &m_timer_end);

    //testGetSpecialProba();
    //testSpecialAward();
    //testGetSingleProba();
    //testSingleAward();
    //testLightningProba();
    //testGetLampProba();
    //testLampAward();
    //testGetLoseRate();
    //testGetAnimal();
    //testFinalMission();
}

void PartyGame::onTimerUpdate(struct ev_loop* loop,struct ev_timer* w,int revents)
{
    PartyGame* game=(PartyGame*)w->data;
    game->timerUpdate();
}

void PartyGame::timerUpdate(void)
{
    //ev_timer_again(m_evLoop, &m_timer_update);
    updateTimeup();
}

void PartyGame::onTimerPrepare(struct ev_loop* loop,struct ev_timer* w,int revents)
{
    PartyGame* game=(PartyGame*)w->data;
    game->timerPrepare();
}

void PartyGame::timerPrepare(void)
{
    ev_timer_stop(m_evLoop, &m_timer_prepare);
    //ev_timer_again(m_evLoop, &m_timer_update);
    ev_timer_again(m_evLoop, &m_timer_start);
    //开始处理
    prepareTimeup(); 
}

PartyPlayer* PartyGame::getPlayer(int uid)
{
    std::map<int,PartyPlayer*>::iterator iter= m_offlinePlayers.find(uid);
    if(iter!=m_offlinePlayers.end())
    {
        return iter->second;
    }

    iter=m_loginPlayers.find(uid);

    if(iter!=m_loginPlayers.end())
    {
        xt_log.error("player(%d) allready login\n",uid);
        return iter->second;
    }

    PartyPlayer* player=new PartyPlayer;
    player->setData(uid,m_server,this);

    m_offlinePlayers[uid]=player;

    return player;
}

void PartyGame::playerLogin(PartyPlayer* player)
{
    assert(player !=  NULL);
    int uid = player->getUid();

    player->initPlayerLogin();

    std::map<int,PartyPlayer*>::iterator iter = m_loginPlayers.find(uid);
    if(iter !=  m_loginPlayers.end())
    {
        xt_log.error("player(%d) all ready login\n",uid);
        return;
    }

    iter = m_offlinePlayers.find(uid);
    if(iter != m_offlinePlayers.end())
    {
        m_loginPlayers[iter->first] = iter->second;
        m_offlinePlayers.erase(iter);
    }
    else 
    {
        xt_log.error("player find in offline\n");
    }

    playerLoginRsp(player);
}

void PartyGame::playerLoginRsp(PartyPlayer* player)
{
    Jpacket packet;
    packet.val["cmd"]           = PA_LOGIN_S;
    packet.val["ret"]           = RET_SUCCESS;
    packet.val["remain"]        = m_timeCount;
    packet.val["status"]        = m_status;
    packet.val["bonus"]         = static_cast<int>(m_ming);
    packet.val["limit"]         = m_betLimit;
    packet.val["endTime"]       = PARTY_TIMER_END;
    packAwardPosInfo(packet);
    packBetInfo(packet);
    packHistory(packet);
    packTop(packet);
    int level = getMingLevel();
    packProba(packet, level);
    packBetType(packet);
    packRobotConf(packet);
    packet.end();
    m_server->unicast(player,packet.tostring());
}

void PartyGame::playerBet(PartyPlayer* player, Jpacket& package)
{
    if(m_status == STATE_END)
    {
        playerBetRsp(0, 0, player, RET_STATE);
        xt_log.info("playerBet, uid:%d, status is STATE_END\n", player->getUid());
        return;
    }

    int bet_nu = package.val["bet_nu"].asInt();
    if(bet_nu <= 0) 
    {
        playerBetRsp(0, 0, player, RET_NEGATIVE);
        xt_log.info("playerBet, uid:%d, bet_nu:%d \n", player->getUid(), bet_nu);
        return;
    }

    player->updateMoney();

    if(player->getMoney() < bet_nu) 
    {
        playerBetRsp(0, 0, player, RET_LACKMONEY);
        xt_log.info("playerBet, uid:%d, bet_nu:%d, money:%d \n", player->getUid(), bet_nu, player->getMoney());
        return;
    }

    int award_pos = package.val["aw_pos"].asInt();
    if(award_pos < 0 && award_pos >= g_maxProbPos)
    {
        playerBetRsp(0, 0, player, RET_BET);
        xt_log.info("playerBet, uid:%d, award_pos:%d \n", player->getUid(), award_pos);
        return;
    }

    //真人单局下注上限
    if(player->getUid() >= VALID_CLIENT_UID_MIN)
    {
        int totalBet = player->getCurrentTotalBet();
        if(totalBet + bet_nu > m_betLimit)
        {
            playerBetRsp(0, 0, player, RET_LIMIT);
            xt_log.info("playerBet, uid:%d, totalBet:%d, bet_nu:%d, limit:%d \n", player->getUid(), totalBet, bet_nu, m_betLimit);
            return;
        }

        //用户少于20000不能下注
        if(player->getMoney() < 20000) 
        {
            playerBetRsp(0, 0, player, RET_LACKMONEY);
            xt_log.info("playerBet money < 20000, uid:%d, money:%d \n", player->getUid(), player->getMoney());
            return;
        }
    }

    PARTY_AWARD_ID pai  = static_cast<PARTY_AWARD_ID>(g_probPos[award_pos]);
    addBetPlayer(player);

    if(player->getUid() < VALID_CLIENT_UID_MIN)
    {
        pai =  static_cast<PARTY_AWARD_ID>(package.val["aw_id"].asInt());
    }

    int total_bet =  player->bet(pai, bet_nu);
    player->decMoney(bet_nu);
    changeGameBet(player, pai, bet_nu);
    playerBetRsp(award_pos, total_bet, player, RET_SUCCESS);
}

void PartyGame::playerBetRsp(int aw_pos, int total_bet, PartyPlayer* player, PARTY_RETURN ret_code)
{
    Jpacket packet;
    packet.val["cmd"]       = PA_BET_S;
    packet.val["ret"]       = ret_code;
    packet.val["bet_nu"]    = total_bet;
    packet.val["aw_pos"]    = aw_pos;
    packet.val["money"]     = player->getMoney();
    packet.end();
    m_server->unicast(player,packet.tostring());
}

void PartyGame::playerCancle(PartyPlayer* player, Jpacket& package)
{
    if(m_status == STATE_END)
    {
        playerCancleRsp(player, RET_STATE);
        return;
    }

    if(player->getBet().empty())
    {
        playerCancleRsp(player, RET_NOBET);
        return;
    }

    int totalBet = player->getCurrentTotalBet();
    player->incMoney(totalBet);
    for(std::map<int, int>::const_iterator it = player->getBet().begin(); it != player->getBet().end(); ++it)
    {
        changeGameBet(player, it->first, -(it->second));
    }
    player->cancleBet();
    playerCancleRsp(player, RET_SUCCESS);
}

void PartyGame::playerCancleRsp(PartyPlayer* player, PARTY_RETURN ret_code)
{
    Jpacket packet;
    packet.val["cmd"]       = PA_CANCLE_S;
    packet.val["ret"]       = ret_code;
    packet.val["money"]     = player->getMoney();
    packet.end();
    m_server->unicast(player,packet.tostring());
}

void PartyGame::playerFollow(PartyPlayer* player,Jpacket& package)
{
    if(m_status == STATE_END)
    {
        playerFollowRsp(player, RET_STATE);
        return;
    }

    int totalBet = player->getHisoryTotalBet();
    if(player->getMoney() < totalBet) 
    {
        playerFollowRsp(player, RET_LACKMONEY);
        return;
    }

    if(player->m_repeatFollow)
    {
        playerFollowRsp(player, RET_REPEAT);
        return;
    }

    player->useHistoryBet();
    player->decMoney(totalBet);
    addBetPlayer(player);

    for(std::map<int, int>::const_iterator it = player->getBet().begin(); it != player->getBet().end(); ++it)
    {
        changeGameBet(player, it->first, it->second);
    }

    playerFollowRsp(player, RET_SUCCESS);
}

void PartyGame::playerFollowRsp(PartyPlayer* player, PARTY_RETURN ret_code)
{
    Jpacket packet;
    packet.val["cmd"]       = PA_FOLLOW_S;
    packet.val["ret"]       = ret_code;
    packet.val["money"]     = player->getMoney();
    packPersonBet(packet, player);
    packet.end();
    m_server->unicast(player, packet.tostring());
    //string str = packet.tostring();
    //m_server->unicast(player, str.c_str());
    //xt_log.info("followrsp :%s\n", str.c_str());
}

void PartyGame::playerLogout(PartyPlayer* player)
{
    //退出取消自动下注
    player->m_auto = false;

    int uid=player->getUid();
    m_loginPlayers.erase(uid);
    m_offlinePlayers[uid]=player;
    playerLogoutRsp(player);
}

void PartyGame::playerLogoutRsp(PartyPlayer* player)
{
    Jpacket packet;
    packet.val["cmd"] = PA_LOGOUT_S;
    packet.val["ret"] = RET_SUCCESS;
    packet.end();
    m_server->unicast(player,packet.tostring());
}

void PartyGame::playerAuto(PartyPlayer* player, Jpacket& package)
{
    player->m_auto = package.val["open"].asBool();
    playerAutoRsp(player, RET_SUCCESS);
}

void PartyGame::playerAutoRsp(PartyPlayer* player, PARTY_RETURN ret_code)
{
    //xt_log.info("playerAutoRsp:\n");
    Jpacket packet;
    packet.val["cmd"] = PA_AUTO_S;
    packet.val["ret"] = ret_code;
    packet.val["open"] = player->m_auto;
    packPersonBet(packet, player);
    packet.end();
    m_server->unicast(player,packet.tostring());
}

void PartyGame::bcUpdate(void)
{
    Jpacket packet;
    packet.val["cmd"]       = PA_UPDATE_B;
    packet.val["remain"]    = m_timeCount;
    packBetInfo(packet);
    packet.end();
    m_server->broadcast(NULL, packet.tostring());
}

void PartyGame::bcStart(void)
{
    Jpacket packet;
    packet.val["cmd"]       = PA_START_B;
    packet.val["remain"]    = m_timeCount;
    packet.val["limit"]     = m_betLimit;
    packRobotConf(packet);
    packet.end();

    m_server->broadcast(NULL, packet.tostring());
}

void PartyGame::bcEnd(void)
{
    Jpacket packet;
    packet.val["cmd"]           = PA_END_B;
    packet.val["big_pos"]       = getBigPos();
    packet.val["aw_pos"]        = getAwardPos(m_aw_id);
    packet.val["aw_type"]       = getAwardType();
    packet.val["aw_light"]      = m_light;

    packAllAward(packet);
    packLamp(packet);
    packHistory(packet);
    packTop(packet);
    packProba(packet, m_openLevel);
    //开特殊奖时候，相关的单项奖赔率改为特殊奖赔率
    //changeToSpecial(packet);

    for(std::map<int,PartyPlayer*>::iterator it = m_loginPlayers.begin(); it != m_loginPlayers.end(); ++it)
    {
        packet.val["reward"]    = static_cast<int>(it->second->m_reward + it->second->m_rewardLig + it->second->m_rewardLam);
        packet.val["money"]     = it->second->getMoney();
        packPersonBet(packet, it->second);
        packet.end();
        m_server->unicast(it->second, packet.tostring());
    }
}

void PartyGame::bcPrepare(void)
{
    Jpacket packet;
    packet.val["cmd"]       = PA_PREPARE_B;
    packet.val["remain"]    = m_timeCount;
    packet.val["bonus"]     = static_cast<int>(m_ming);

    packAwardPosInfo(packet);
    int level = getMingLevel();
    packProba(packet, level);

    //packet.end();
    //m_server->broadcast(NULL, packet.tostring());

    for(std::map<int, PartyPlayer*>::iterator it = m_loginPlayers.begin(); it != m_loginPlayers.end(); ++it)
    {
        PartyPlayer* player = it->second;
        packet.val["open"] = player->m_auto;
        packPersonBet(packet, player);
        packet.end();
        m_server->unicast(player, packet.tostring());
    }
}

int PartyGame::shutDown()
{
    ev_timer_stop(m_evLoop, &m_timer_start);
    ev_timer_stop(m_evLoop, &m_timer_end);
    ev_timer_stop(m_evLoop, &m_timer_update);
    ev_timer_stop(m_evLoop, &m_timer_prepare);

    //printf("m_offlinePlayers.size()=%d,m_loginPlayers.size()=%d\n",(int)m_offlinePlayers.size(),(int)m_loginPlayers.size());
    for(std::map<int,PartyPlayer*>::iterator iter=m_offlinePlayers.begin();iter!=m_offlinePlayers.end();++iter)
    {
        PartyPlayer* player=iter->second;
        assert(player);
        delete player;
    }

    for(std::map<int,PartyPlayer*>::iterator iter=m_loginPlayers.begin();iter!=m_loginPlayers.end();++iter)
    {
        PartyPlayer* player=iter->second;
        //	printf("uid=%d,player_addr=%ld\n",iter->first,(long)iter->second);
        assert(player);
        delete player;
    }

    m_loginPlayers.clear();
    m_server=NULL;
    return 0;
}

bool PartyGame::isSpecial(void)
{
    if(g_closeSpecial)
    {
        return false;
    }

    if(g_testSpecial)
    {
        return true;
    }

    //检查时间点
    time_t cur_time = time(NULL);
    if(cur_time > m_check_stamp)
    {
        if(checkSpecialLimit()) 
        {
            procSpecialInit();
            return true;
        }
        else
        {
            //超过开奖时间5小时
            if(cur_time - m_award_stamp > 18000)    
            {
                //设置检查时间(当前时间后推1小时)
                m_check_stamp = cur_time + 3600; 
            }
            else
            {
                //设置检查时间(开奖时间随机后推5小时)
                m_check_stamp = m_award_stamp + 18000; 
            }
        }
    }

    return false;
}

double PartyGame::getSpecialProba(int aw_id, int mi_lv, double b) const
{
    double m = getProba(aw_id, mi_lv); 
    //xt_log.info("getSpecialProba, aw_id:%d, m:%f, b:%f, 1.0 / m + b:%f \n", mi_lv, m, b, 1.0 / m +b);
    return max(1.0 / m + b, 0.0);
}

void PartyGame::specialAward(void)
{
    int mi_lv = getMingLevel();  
    double b = getAnFactor(); 

    for(int i = static_cast<int>(AWARD_13_1); i <= static_cast<int>(AWARD_14_3); ++i)
    {
        m_special[i] = getSpecialProba(i, mi_lv, b); 
    }

    for(std::map<int, double>::const_iterator iter = m_special.begin(); iter != m_special.end(); ++iter)
    {
        m_spec_total += iter->second; 
    }

    double rand_val = rand() % static_cast<int>(m_spec_total * 1000);
    for(std::map<int, double>::const_iterator iter = m_special.begin(); iter != m_special.end(); ++iter)
    {
        rand_val -= iter->second * 1000;     
        if(rand_val <= 0)
        {
            m_aw_id = iter->first;
            break;
        }
    }
}

void PartyGame::testSpecialAward(void)
{
    reset();
    isSpecial();
    specialAward();

    for(std::map<int, double>::const_iterator iter = m_special.begin(); iter != m_special.end(); ++iter)
    {
        xt_log.info("testSpecialAward, aw_id:%d, prob:%f, b:%f \n", iter->first, iter->second, iter->second / m_spec_total);
    }
    xt_log.info("testSpecialAward, aw_id:%d \n", m_aw_id);
}

void PartyGame::testGetSpecialProba(void) 
{
    m_ming = g_party_ming;
    int mi_lv = getMingLevel();  
    double b = getAnFactor(); 

    xt_log.info("testGetSpecialProba, mi_lv:%d, anFactor:%f \n", mi_lv, b);
    for(int i = static_cast<int>(AWARD_13_1); i <= static_cast<int>(AWARD_14_3); ++i)
    {
        double tmp = getSpecialProba(i, mi_lv, b); 
        xt_log.info("testGetSpecialProba, award_id:%d, proba:%f \n", i, tmp);
    }

    m_ming = g_party_ming2;
    mi_lv = getMingLevel();  
    b = getAnFactor(); 
    xt_log.info("testGetSpecialProba, mi_lv:%d, anFactor:%f \n", mi_lv, b);
    for(int i = static_cast<int>(AWARD_13_1); i <= static_cast<int>(AWARD_14_3); ++i)
    {
        double tmp = getSpecialProba(i, mi_lv, b); 
        xt_log.info("testGetSpecialProba, award_id:%d, proba:%f \n", i, tmp);
    }

    m_ming = g_party_ming2 * 2;
    mi_lv = getMingLevel();  
    b = getAnFactor(); 
    xt_log.info("testGetSpecialProba, mi_lv:%d, anFactor:%f \n", mi_lv, b);
    for(int i = static_cast<int>(AWARD_13_1); i <= static_cast<int>(AWARD_14_3); ++i)
    {
        double tmp = getSpecialProba(i, mi_lv, b); 
        xt_log.info("testGetSpecialProba, award_id:%d, proba:%f \n", i, tmp);
    }
}

bool PartyGame::isLightning(void)
{
    if(g_testDouble)
    {
        return true;
    }

    int mi_lv = getMingLevel();  
    int m = getProba(m_aw_id, mi_lv);
    double a = getMingFactor();
    double p3 = std::max(double(1 / m + a), 0.0);

    int randval = rand() % 1000;
    double p3val = p3 * 1000;
        

    if(randval < p3val)
    {
        //printf("light true, randval:%d, p3val:%f \n", randval, p3val);
        return true;
    }
    //printf("light false, randval:%d, p3val:%f \n", randval, p3val);
    return false;
}

void PartyGame::lightningAward(void)
{
    m_light = true;
}

void PartyGame::testLightningProba(void)
{
    reset();
    double ming_list[3] = { g_party_ming, g_party_ming2, g_party_ming2 * 2};
    m_ming = ming_list[rand() % 3];

    int mi_lv = getMingLevel();  
    double a = getMingFactor();

    for(int i = static_cast<int>(AWARD_1); i <= static_cast<int>(AWARD_14_3); ++i)
    {
        int m = getProba(i, mi_lv);
        double p3 = std::max(double(1 / m + a), 0.0);
        xt_log.info("testLightningProba, p3:%f, aw_id:%d, mi_lv:%d, a:%f, m:%d \n", p3, i, mi_lv, a, m);
    }
}

bool PartyGame::isLamp(void)
{
    if(g_testLamp)
    {
        m_ming = g_party_ming2 * 2;
        return true;
    }

    if(rand() % 1000 < getLampProba() * 1000)
    {
        return true;
    }
    return false;
}

/*
   明池参考值A  暗池参考值B  明池实际额度C  暗池实际额度D 
   1) 本局是否是特殊奖，若是，则不送灯。
   2) 本局若是单项奖，则计算本局送灯概率P4：
   a) 若C<=2*A，则P4=0
   b) 若2*A<C<6*A,则P4=(12.5-(12.5^2-(C/100000000-6)^2)^0.5)/18
   c) 若C>=6*A，P4=0.5
   */
double PartyGame::getLampProba(void) const
{
    if(isSpecialAward(m_aw_id))
    {
        return 0;
    }

    if(m_ming <= g_party_ming2)
    {
        return 0; 
    }
    else if(g_party_ming2 < m_ming && m_ming < 6 * g_party_ming)
    {
        return (12.5 - pow((pow(12.5, 2) - pow(m_ming / 100000000.0 - 6, 2)), 0.5)) / 18.0; 
    }
    else
    {
        return 0.5;
    }
}

void PartyGame::lampAward(void)
{
    int mi_lv = getMingLevel();  
    double b = getAnFactor(); 

    double single_total = 0;
    double val = 0;
    std::map<int, double> lampSelect;
    for(int i = static_cast<int>(AWARD_1); i <= static_cast<int>(AWARD_12); ++i)
    {
        if(i == m_aw_id)
        {
            continue; 
        }

        val = getSingleProba(i, mi_lv, b); 
        single_total += val; 
        lampSelect[i] = val;
        //xt_log.info("lampAward, aw_id:%d, val:%f\n", i, val);
    }

    double loseLimit = getLoseLimit();
    //xt_log.info("lampAward, loseLimit:%f\n", loseLimit);
    if(loseLimit <= 0.0)
    {
        return;
    }

    while(!lampSelect.empty())
    {
        bool find = false;
        int target_id;
        double rand_val = rand() % (static_cast<int>(single_total * 1000));
        //xt_log.info("lampAward, rand_val:%f\n", rand_val);
        for(std::map<int, double>::iterator iter = lampSelect.begin(); iter != lampSelect.end(); ++iter)
        {
            rand_val -= iter->second * 1000;     
            if(rand_val <= 0)
            {
                find = true;
                target_id = iter->first;
                single_total -= iter->second;
                lampSelect.erase(iter);
                //xt_log.info("lampAward, find, target_id:%d\n", target_id);
                break;
            }
        }

        if(find && m_aw_set.find(target_id) == m_aw_set.end())
        {
            double AwardLose = getAwardLose(target_id); 
            //xt_log.info("lampAward, AwardLose:%f, loseLimit:%f\n", AwardLose, loseLimit);
            if(loseLimit >= AwardLose)
            {
                m_aw_set.insert(target_id);
                loseLimit -= AwardLose; 
            }
            else
            {
                break; 
            }
        }
    }
}

/*
   明池参考值A  暗池参考值B  明池实际额度C  暗池实际额度D 
   计算本局可赔付的额度比例x：
   x=(1-0.5^(C/A-2))/4
   */
double PartyGame::getLoseRate(void) const
{
    return max((1 - pow(0.5, (m_ming / g_party_ming - 2))) / 4, 0.0);
}

//计算出本局可赔付的金币额度Y=x*C
double PartyGame::getLoseLimit(void) const
{
    return getLoseRate() * m_ming;
}

void PartyGame::testGetLampProba(void)
{
    reset();
    static int testIdx = 0;
    double ming_list[3] = { g_party_ming, g_party_ming2 * 2, g_party_ming2 * 6};
    m_ming = ming_list[(testIdx++) % 3];
    xt_log.info("testGetLampProba, proba:%f \n", getLampProba());
}

void PartyGame::testGetLoseRate(void)
{
    reset();
    static int testIdx = 0;
    double ming_list[3] = { g_party_ming, g_party_ming2 * 2, g_party_ming2 * 6};
    m_ming = ming_list[(testIdx++) % 3];
    xt_log.info("testGetLoseRate, rate:%f \n", getLoseRate());
    xt_log.info("testGetLoseRate, limit:%f \n", getLoseLimit());
}

void PartyGame::testLampAward(void)
{
    reset();
    static int testIdx = 0;
    double ming_list[3] = { g_party_ming, g_party_ming2 * 2, g_party_ming2 * 6};
    m_ming = ming_list[(testIdx++) % 3];
    xt_log.info("testLampAward\n"); 
    lampAward();
    for(std::set<int>::iterator it = m_aw_set.begin(); it != m_aw_set.end(); ++it)
    {
        xt_log.info("testLampAward, aw_id:%d\n", *it); 
    }
}

void PartyGame::finalMission(void)
{
    m_openLevel = getMingLevel();
    //xt_log.info("finalMission, m_aw_id:%d, prob:%d, mi_lv:%d\n", m_aw_id, prob, mi_lv);
    //结算玩家输赢
    for(std::map<int, PartyPlayer*>::iterator it = m_betPlayers.begin(); it != m_betPlayers.end(); ++it)
    {
        it->second->procReward();
    }

    //奖池计算
    procPool();
}

void PartyGame::testFinalMission(void)
{
    xt_log.info("testFinalMission\n"); 
    /*
       reset();
       static int testIdx = 0;
       double ming_list[3] = { g_party_ming, g_party_ming2, g_party_ming * 6};
       m_ming = ming_list[(testIdx++) % 3];

       static std::map<int,PartyPlayer*> betPlayers; 
       if(betPlayers.empty())
       {
       for(int i = static_cast<int>(AWARD_1); i <= static_cast<int>(AWARD_12); ++i)
       {
       PartyPlayer* pp = new PartyPlayer();
       pp->m_bet_nu = i * 10 + 1;
       pp->m_aw_id = i;
       betPlayers[i] = pp;
       }
       }
       m_betPlayers = betPlayers;
       startTimeup();
       */
}

void PartyGame::addBetPlayer(PartyPlayer* player)
{
    int uid=player->getUid();
    std::map<int,PartyPlayer*>::iterator it = m_betPlayers.find(uid);
    if(it == m_betPlayers.end())
    {
        m_betPlayers[uid] = player; 
    }
}

double PartyGame::getAwardLose(int aw_id) const
{
    int mi_lv = getMingLevel();  
    int prob = getProba(aw_id, mi_lv);
    int bet = 0;
    std::map<int, double>::const_iterator it = m_game_bet.find(aw_id);
    if(it != m_game_bet.end())
    {
        bet = it->second;
    }
    return prob * bet;
}

void PartyGame::singleAward(void)
{
    int mi_lv = getMingLevel();  
    double b = getAnFactor(); 

    double single_total = 0;
    double val = 0;
    for(int i = static_cast<int>(AWARD_1); i <= static_cast<int>(AWARD_12); ++i)
    {
        val = getSingleProba(i, mi_lv, b); 
        single_total += val; 
        m_single[i] = val;
    }

    //xt_log.info("singleAward, para: %d", static_cast<int>(single_total * 1000));
    int rand_val = rand() % (static_cast<int>(single_total * 1000));
    for(std::map<int, double>::const_iterator iter = m_single.begin(); iter != m_single.end(); ++iter)
    {
        rand_val -= iter->second * 1000;     
        if(rand_val <= 0)
        {
            m_aw_id = iter->first;
            break;
        }
    }
}

double PartyGame::getSingleProba(int aw_id, int mi_lv, double b) const
{
    double Nx = getWeight(aw_id, mi_lv);
    double p0 = Nx / g_total_weight[mi_lv];
    //    xt_log.info("getSingleProba, Nx:%d, p0:%d, b:%f, mi_lv:%d,(p0 + b) / (1 + 12 * b):%f\n", Nx, p0, b, mi_lv, (p0 + b) / (1 + 12 * b));
    return max((p0 + b) / (1 + 12 * b), 0.0);
}

void PartyGame::testGetSingleProba(void)
{
    reset();
    double ming_list[3] = { g_party_ming, g_party_ming2, g_party_ming2 * 2};
    m_ming = ming_list[rand() % 3];

    int mi_lv = getMingLevel();  
    double b = getAnFactor(); 
    xt_log.info("testGetSingleProba, mi_lv:%d, anfactor:%f\n", mi_lv, b);
    for(int i = static_cast<int>(AWARD_1); i <= static_cast<int>(AWARD_12); ++i)
    {
        double val = getSingleProba(i, mi_lv, b); 
        xt_log.info("testGetSingleProba, aw_id:%d, proba:%f\n", i, val);
    }
}

void PartyGame::testSingleAward(void)
{
    reset();
    double ming_list[3] = { g_party_ming, g_party_ming2, g_party_ming2 * 2};
    m_ming = ming_list[rand() % 3];
    singleAward();
    xt_log.info("testSingleAward, aw_id:%d \n", m_aw_id);
}

void PartyGame::startTimeup(void)
{
    //游戏结算
    //xt_log.info("startTimeup\n");
    m_status = STATE_END;
    m_timeCount = PARTY_TIMER_END;

    if(isControl())
    {
        procCtrl();
    }
    else
    {
        procNormal(); 
    }

    finalMission();
    addHistory();
    addTop();

    rollAnimal();
    bcEnd();
    saveResult();
    savePlayerResult();
    saveTop();
}

void PartyGame::prepareTimeup(void)
{
    xt_log.info("prepareTimeup\n");
    m_status = STATE_START;
    m_timeCount = PARTY_TIMER_START;

    updateConfig();
    bcStart();
}

void PartyGame::endTimeup(void)
{
    xt_log.info("endTimeup\n");
    m_status = STATE_PREPARE;
    m_timeCount = PARTY_TIMER_PREPARE; 
    reset();
    randomShow();

    procPlayerAuto();
    bcPrepare();

	m_server->refreshPlayerNuToRedis();
}

void PartyGame::updateTimeup(void)
{
    m_timeCount -= 1;
    if(m_status == STATE_START)
    {
        bcUpdate();
    }
    //xt_log.info("updateTimeup, m_timeCount:%d, m_status:%d\n", m_timeCount, m_status);
}

/*
   明池参考值A  暗池参考值B  明池实际额度C  暗池实际额度D 
   e) 明池等级：
   i. 当C<=A时，等级为1；
   ii. 当A<C<=2*A时，等级为2；
   iii. 当C>2*A时，等级为3。
   */
int PartyGame::getMingLevel(void) const
{
    if(m_ming <= g_party_ming)
    {
        return ODD_1;
    }
    else if(g_party_ming < m_ming && m_ming <= g_party_ming2)
    {
        return ODD_2;
    }
    else
    {
        return ODD_3;
    }
}

/*
   明池参考值A  暗池参考值B  明池实际额度C  暗池实际额度D 
   f) 明池动态因子a：
   i. 当C<=A时，a = (LOG((A+C) / A, 3) - 1) / 20
   ii. 当A<C<=2*A时，a=(1-LOG(2,3))*(C-2*A)/A/20
   iii. 当C>2*A时，a=(1 - 2^((2*A-C)/A)) / 20
   */
double PartyGame::getMingFactor(void) const
{
    if(m_ming <= g_party_ming)
    {
        return (MathLog((g_party_ming + m_ming) / g_party_ming, 3) - 1) / 20.0;
    }
    else if(g_party_ming < m_ming && m_ming <= g_party_ming2)
    {
        return (1 - MathLog(2, 3)) * (m_ming - g_party_ming2) / g_party_ming / 20.0;
    }
    else
    {
        return (1 - pow(2, (g_party_ming2 - m_ming) / g_party_ming)) / 20.0;
    }
}

/*
   明池参考值A  暗池参考值B  明池实际额度C  暗池实际额度D 
   g) 暗池动态因子b:
   i. 当D<=B时，b=(LOG((D+B)/B,2)-2)/90
   ii. 当B<D<=2*B时，b=(D-2*B)/B/90
   iii. 当2*B<D时，b=(1-9^((2*B-D)/B/90
   */
double PartyGame::getAnFactor(void) const
{
    if(m_an <= g_party_an)
    {
        return (MathLog((m_an + g_party_an) / g_party_an, 2) - 2) / 90.0;
    }
    else if(g_party_an < m_an && m_an <= g_party_an2)
    {
        return (m_an - g_party_an2) / g_party_an / 90.0;
    }
    else
    {
        return (1 - MathLog(9, g_party_an2 - m_an)) / g_party_an / 90.0; 
    }
}

double PartyGame::MathLog(double x, double y) const
{
    return log(y) / log(x);
}

double PartyGame::getProba(int aw_id, int mi_lv) const
{
    if(aw_id <= static_cast<int>(AWARD_MIN) || aw_id >= static_cast<int>(AWARD_MAX) || mi_lv <= ODD_MIN || mi_lv >= ODD_MAX)
    {
        xt_log.info("getProba error! aw_id:%d, mi_lv:%d\n", aw_id, mi_lv);
        return 0; 
    }
    //xt_log.info("getProba  aw_id:%d, mi_lv:%d, proba:%d\n", aw_id, mi_lv, g_proba[aw_id][mi_lv]);
    return static_cast<double>(g_proba[aw_id][mi_lv]); 
}

double PartyGame::getWeight(int aw_id, int mi_lv) const
{
    if(aw_id <= AWARD_MIN || aw_id >= AWARD_13_1 || mi_lv <= ODD_MIN || mi_lv >= ODD_MAX)
    {
        xt_log.info("getWeight error! aw_id:%d, mi_lv:%d\n", aw_id, mi_lv);
        return 0; 
    }
    return static_cast<double>(g_weight[aw_id][mi_lv]);
}

void PartyGame::reset(void)
{
    m_special.clear();
    m_spec_total = 0;
    m_single.clear();
    m_light = false;
    m_aw_id = AWARD_1;
    m_aw_set.clear();
    m_game_bet.clear();
    m_people_bet.clear();
    m_show.clear();
    m_betPlayers.clear();
    m_pos = 0;
    m_openLevel = ODD_1;
    m_winMoney = 0;

    for(std::map<int,PartyPlayer*>::iterator it = m_offlinePlayers.begin(); it != m_offlinePlayers.end(); ++it) 	
    {
        it->second->reset();
    }
    for(std::map<int,PartyPlayer*>::iterator it = m_loginPlayers.begin(); it != m_loginPlayers.end(); ++it) 	
    {
        it->second->reset();
    }

    m_totalIn = 0;
    m_totalOut = 0;
    m_awardPeople = 0;
}

bool PartyGame::isSpecialAward(int awid) const
{
    if(awid >= static_cast<int>(AWARD_13_1) && awid <= static_cast<int>(AWARD_14_3))  
    {
        return true;  
    }
    return false;
}

void PartyGame::procPool(void)
{
    double totalBet         = 0;
    double totalReward      = 0;
    double totalRewardLig   = 0;
    double totalRewardLam   = 0;
    for(std::map<int, PartyPlayer*>::iterator it = m_betPlayers.begin(); it != m_betPlayers.end(); ++it)
    {
        if(it->first < VALID_CLIENT_UID_MIN)
        {
            continue;
        }
        totalBet        += it->second->getCurrentTotalBet(); 
        totalReward     += it->second->m_reward; 
        totalRewardLig  += it->second->m_rewardLig; 
        totalRewardLam  += it->second->m_rewardLam; 
        if(it->second->m_reward > 0)
        {
            m_awardPeople++; 
        }
    }


    //in
    m_ming += totalBet * 0.3;
    m_tax  += totalBet * 0.3 * g_tax;
    m_an   += totalBet * 0.7 * g_tax;

    //out
    m_ming -= totalRewardLig;    
    m_ming -= totalRewardLam;    

    double pure_ming = m_ming - m_tax;
    m_tax  -= (totalReward / pure_ming) * m_tax;
    m_ming = pure_ming + m_tax;

    m_an   -= totalReward;

    m_winMoney = totalBet - totalReward - totalRewardLam - totalRewardLig;

    m_totalIn = totalBet;
    m_totalOut = totalReward + totalRewardLam + totalRewardLig;
}

void PartyGame::changeGameBet(PartyPlayer* player, int aw_id, int bet_nu)
{
    //xt_log.info("changeGameBet: aw_id:%d, bet_nu:%d\n", aw_id, bet_nu);
    std::map<int, double>::iterator it = m_game_bet.find(aw_id);
    if(it != m_game_bet.end())
    {
        it->second += bet_nu; 
    }
    else
    {
        m_game_bet[aw_id] = bet_nu; 
    }
    
    if(player->getUid() > VALID_CLIENT_UID_MIN)
    {
        std::map<int, double>::iterator it = m_people_bet.find(aw_id);
        if(it != m_people_bet.end())
        {
            it->second += bet_nu; 
        }
        else
        {
            m_people_bet[aw_id] = bet_nu; 
        }
    }
}

void PartyGame::randomShow(void)
{
    std::vector<int> vecTmp;
    for(int i = static_cast<int>(AWARD_1); i <= static_cast<int>(AWARD_12); ++i)
    {
        vecTmp.push_back(i); 
        vecTmp.push_back(getAnimal(i)); 
    }

    std::random_shuffle(vecTmp.begin(), vecTmp.begin() + 6);
    std::random_shuffle(vecTmp.begin() + 6, vecTmp.begin() + 12);
    std::random_shuffle(vecTmp.begin() + 12, vecTmp.begin() + 18);
    std::random_shuffle(vecTmp.begin() + 18, vecTmp.begin() + 24);

    int pos = 0;
    int aw  = 0;
    for(int i = 0; i < 24; ++i)
    {
        pos = g_animalPos[i];
        aw = vecTmp[i];
        m_show[pos] = aw;
    }
}

int PartyGame::getAnimal(int cur)
{
    if(cur >= AWARD_1 && cur <= AWARD_3)
    {
        return (rand() % 3);
    }
    else if(cur >= AWARD_4 && cur <= AWARD_6)
    {
        return (rand() % 3) + 3;
    }
    else if(cur >= AWARD_7 && cur <= AWARD_9)
    {
        return (rand() % 3) + 6;
    }
    else// if(cur >= AWARD_10 && cur <= AWARD_12)
    {
        return (rand() % 3) + 9;
    }
}

void PartyGame::testGetAnimal(void)
{
    reset();
    randomShow();
    for(std::map<int, int>::iterator it = m_show.begin(); it != m_show.end(); ++it)
    {
        xt_log.info("pos:%d, aw_id:%d\n", it->first, it->second);
    }
}

void PartyGame::addHistory(void)
{
    for(std::deque<historyData>::iterator iter = m_history.begin(); iter != m_history.end(); ++iter)
    {
        iter->fresh = false;
    }

    switch(m_aw_id)
    {
        case static_cast<int>(AWARD_1):
        case static_cast<int>(AWARD_2):
        case static_cast<int>(AWARD_3):
        case static_cast<int>(AWARD_4):
        case static_cast<int>(AWARD_5):
        case static_cast<int>(AWARD_6):
        case static_cast<int>(AWARD_7):
        case static_cast<int>(AWARD_8):
        case static_cast<int>(AWARD_9):
        case static_cast<int>(AWARD_10):
        case static_cast<int>(AWARD_11):
        case static_cast<int>(AWARD_12):
            m_history.push_back(historyData(m_aw_id, m_light, true));
            break;
        case static_cast<int>(AWARD_13_1):
            m_history.push_back(historyData(static_cast<int>(AWARD_1), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_2), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_3), m_light, true));
            break;
        case static_cast<int>(AWARD_13_2):
            m_history.push_back(historyData(static_cast<int>(AWARD_4), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_5), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_6), m_light, true));
            break;
        case static_cast<int>(AWARD_13_3):
            m_history.push_back(historyData(static_cast<int>(AWARD_7), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_8), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_9), m_light, true));
            break;
        case static_cast<int>(AWARD_13_4):
            m_history.push_back(historyData(static_cast<int>(AWARD_10), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_11), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_12), m_light, true));
            break;
        case static_cast<int>(AWARD_14_1):
            m_history.push_back(historyData(static_cast<int>(AWARD_1), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_4), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_7), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_10), m_light, true));
            break;
        case static_cast<int>(AWARD_14_2):
            m_history.push_back(historyData(static_cast<int>(AWARD_2), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_5), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_8), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_11), m_light, true));
            break;
        case static_cast<int>(AWARD_14_3):
            m_history.push_back(historyData(static_cast<int>(AWARD_3), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_6), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_9), m_light, true));
            m_history.push_back(historyData(static_cast<int>(AWARD_12), m_light, true));
            break;
    }

    int num = m_history.size() - HISTORYNUM; 
    if(num > 0)
    {
        for(int i = 0; i < num; ++i)
        {
            m_history.pop_front();
        }
    }
}

bool compareTop(PartyPlayer* a, PartyPlayer* b)
{
    return a->m_totalReward > b->m_totalReward;
}

void PartyGame::addTop(void)
{
    std::vector<PartyPlayer*> playerList;

    for(std::set<int>::iterator it = m_top.begin(); it != m_top.end(); ++it)
    {
        PartyPlayer* pp = getPlayer(*it);
        if(pp)
        {
            playerList.push_back(pp);  
        }
    }

    //有玩家当然下过，但是本轮没下注，所以不能用bet playerlist
    for(std::map<int, PartyPlayer*>::iterator it = m_loginPlayers.begin(); it != m_loginPlayers.end(); ++it)
    {
        PartyPlayer* pp = getPlayer(it->first);
        //排除机器人
        if(pp && pp->getUid() > VALID_CLIENT_UID_MIN)
        {
            playerList.push_back(pp);  
        }
    }
    std::sort(playerList.begin(), playerList.end(), compareTop);

    m_showTop.clear();
    m_top.clear();
    int num = 0;
    std::pair<std::set<int>::iterator,bool> pairTmp;
    for(std::vector<PartyPlayer*>::iterator it = playerList.begin(); it != playerList.end(); ++it)
    {
        if((*it)->m_totalReward <= 0)
        {
            continue; 
        }

        pairTmp = m_top.insert((*it)->getUid());
        if(pairTmp.second)
        {
            m_showTop.push_back((*it)->getUid());
            num++;
            if(num >= g_topNum)
            {
                break; 
            }
        }
    }
}

void PartyGame::rollAnimal(void)
{
    m_pos = rand() % g_grid;

    std::vector<int> animal; 
    animal.reserve(g_grid);
    std::vector<int> color; 
    color.reserve(g_grid);

    int newPos = 0;
    for(std::map<int, int>::iterator it = m_show.begin(); it != m_show.end(); ++it)
    {
        int aw_id = it->second;

        newPos = (it->first + m_pos) % g_grid;
        animal[newPos] = getAnimalType(aw_id);
        color.push_back(getColorType(aw_id));
    }

    for(int i = 0; i < g_grid; ++i)
    {
        m_show[i] = getAwardId(animal[i], color[i]);
    }
}

void PartyGame::packAwardPosInfo(Jpacket& packet)
{
    int i = 0;
    for(std::map<int, int>::iterator it = m_show.begin(); it != m_show.end(); ++it)
    {
        int aw_id = it->second;

        Json::Value tmp;
        tmp["animal"] = getAnimalType(aw_id);
        tmp["color"] = getColorType(aw_id);
        packet.val["posInfo"][i] = tmp;
        i++;
    }
}

void PartyGame::packBetInfo(Jpacket& packet)
{
    for(int i = static_cast<int>(AWARD_1); i <= static_cast<int>(AWARD_12); ++i)
    {
        if(m_game_bet.find(i) == m_game_bet.end())
        {
            m_game_bet[i] = 0; 
        }
    }

    int tmpAwid = 0;
    for(size_t i = 0; i < g_maxProbPos; ++i)
    {
        tmpAwid = g_probPos[i]; 
        if(m_game_bet.find(tmpAwid) != m_game_bet.end())
        {
            Json::Value tmp;
            tmp["aw_id"]                = tmpAwid;
            tmp["bet_num"]              = static_cast<int>(m_game_bet[tmpAwid]);
            packet.val["betInfo"][i]  = tmp;
        }
    }
}

void PartyGame::packBetType(Jpacket& packet)
{
    for(int i = 0; i < static_cast<int>(m_bt.size()); ++i)
    {
        packet.val["betType"][i]  = m_bt[i];
    }
}

void PartyGame::packPersonBet(Jpacket& packet, PartyPlayer* player)
{
    Json::Value tmp;
    packet.val["playerBet"] = tmp;

    int i = 0;
    for(std::map<int, int>::const_iterator it = player->getBet().begin(); it != player->getBet().end(); ++it)
    {
        Json::Value tmp;
        tmp["aw_id"]                = it->first;
        tmp["aw_pos"]               = g_probPos[it->first];
        tmp["bet_num"]              = it->second;
        packet.val["playerBet"][i]  = tmp;
        //        xt_log.info("person bet, aw_id:%d, bet_num:%d\n", it->first, it->second);
        i++;
    }

    if(player->getBet().empty())
    {
        Json::Value tmp;
        tmp.clear();
        packet.val["playerBet"] = tmp;
    }
}

void PartyGame::packHistory(Jpacket& packet)
{
    int j = 0;
    for(std::deque<historyData>::iterator iter = m_history.begin(); iter != m_history.end(); ++iter)
    {
        Json::Value tmp;
        tmp["aw_id"]                = iter->aw_id;
        tmp["lightning"]            = iter->lightning;
        tmp["fresh"]                = iter->fresh;
        packet.val["history"][j++]  = tmp;
    }
}

void PartyGame::packProba(Jpacket& packet, int level)
{
    for(size_t i = 0; i < g_maxProbPos; ++i)
    {
        Json::Value tmp;
        tmp["aw_id"]                = g_probPos[i];
        tmp["value"]                = static_cast<int>(getProba(g_probPos[i], level));
        packet.val["proba"][i]    = tmp;
    }
}

void PartyGame::packTop(Jpacket& packet)
{
    int k = 0;
    for(std::vector<int>::iterator iter = m_showTop.begin(); iter != m_showTop.end(); ++iter)
    {
        PartyPlayer* pp = getPlayer(*iter);
        if(pp)
        {
            Json::Value tmp;
            tmp["name"]             = pp->getName();
            tmp["reward"]           = pp->getTotalReward();
            tmp["uid"]              = *iter;
            tmp["sex"]              = pp->getSex();
            packet.val["top"][k]    = tmp;
        }
        k++;
    }
}

void PartyGame::packLamp(Jpacket& packet)
{
    int i = 0;
    for(std::set<int>::const_iterator it = m_aw_set.begin(); it != m_aw_set.end(); ++it)
    {
        packet.val["lamp"][i++] = getAwardPos(*it);
    }
}

void PartyGame::packAllAward(Jpacket& packet)
{
    int i = 0;
    if(isSpecialAward(m_aw_id)) 
    {
        switch(m_aw_id) 
        {
            case AWARD_13_1:
                packet.val["award"][i++] = packAwardProba(AWARD_1, AWARD_1);
                packet.val["award"][i++] = packAwardProba(AWARD_2, AWARD_2);
                packet.val["award"][i++] = packAwardProba(AWARD_3, AWARD_3);
                break;
            case AWARD_13_2:
                packet.val["award"][i++] = packAwardProba(AWARD_4, AWARD_4);
                packet.val["award"][i++] = packAwardProba(AWARD_5, AWARD_5);
                packet.val["award"][i++] = packAwardProba(AWARD_6, AWARD_6);
                break;
            case AWARD_13_3:
                packet.val["award"][i++] = packAwardProba(AWARD_7, AWARD_7);
                packet.val["award"][i++] = packAwardProba(AWARD_8, AWARD_8);
                packet.val["award"][i++] = packAwardProba(AWARD_9, AWARD_9);
                break;
            case AWARD_13_4:
                packet.val["award"][i++] = packAwardProba(AWARD_10, AWARD_10);
                packet.val["award"][i++] = packAwardProba(AWARD_11, AWARD_11);
                packet.val["award"][i++] = packAwardProba(AWARD_12, AWARD_12);
                break;
            case AWARD_14_1:
                packet.val["award"][i++] = packAwardProba(AWARD_1, AWARD_1);
                packet.val["award"][i++] = packAwardProba(AWARD_4, AWARD_4);
                packet.val["award"][i++] = packAwardProba(AWARD_7, AWARD_7);
                packet.val["award"][i++] = packAwardProba(AWARD_10, AWARD_10);
                break;
            case AWARD_14_2:
                packet.val["award"][i++] = packAwardProba(AWARD_2, AWARD_2);
                packet.val["award"][i++] = packAwardProba(AWARD_5, AWARD_5);
                packet.val["award"][i++] = packAwardProba(AWARD_8, AWARD_8);
                packet.val["award"][i++] = packAwardProba(AWARD_11, AWARD_11);
                break;
            case AWARD_14_3:
                packet.val["award"][i++] = packAwardProba(AWARD_3, AWARD_3);
                packet.val["award"][i++] = packAwardProba(AWARD_6, AWARD_6);
                packet.val["award"][i++] = packAwardProba(AWARD_9, AWARD_9);
                packet.val["award"][i++] = packAwardProba(AWARD_12, AWARD_12);
                break;
        }
    }
    else
    {
        packet.val["award"][i++] = packAwardProba(m_aw_id, m_aw_id);
    }

    for(std::set<int>::const_iterator it = m_aw_set.begin(); it != m_aw_set.end(); ++it)
    {
        packet.val["award"][i++] = packAwardProba(*it, *it);
    }
}

Json::Value PartyGame::packAwardProba(int aw_id, int prob_aw)
{
    Json::Value tmp;
    tmp["aw_id"]              = aw_id;
    tmp["aw_proba"]           = getProba(prob_aw, m_openLevel);
    tmp["aw_pos"]             = getAwardPos(aw_id);
    return tmp;
}
        
void PartyGame::packRobotConf(Jpacket& packet)
{
    for(size_t i = 0; i < m_btw.size(); ++i)
    {
        packet.val["btw"][i] = m_btw[i];
    }

    for(size_t i = 0; i < m_br.size(); ++i)
    {
        packet.val["br"][i] = m_br[i];
    }

    for(size_t i = 0; i < m_bt.size(); ++i)
    {
        packet.val["bt"][i] = m_bt[i];
    }
}

int PartyGame::getAwardPos(int pai) const
{
    int pos = 0;
    int sinaw = pai;

    if(isSpecialAward(pai))
    {
        switch(sinaw) 
        {
            case AWARD_13_1: sinaw = AWARD_1; break;
            case AWARD_13_2: sinaw = AWARD_4; break;
            case AWARD_13_3: sinaw = AWARD_7; break;
            case AWARD_13_4: sinaw = AWARD_10; break;
            case AWARD_14_1: sinaw = AWARD_10; break;
            case AWARD_14_2: sinaw = AWARD_11; break;
            case AWARD_14_3: sinaw = AWARD_12; break;
        }
    }

    for(std::map<int, int>::const_iterator it = m_show.begin(); it != m_show.end(); ++it)
    {
        if(it->second == static_cast<int>(sinaw))
        {
            pos = it->first; 
            break;
        }
    }

    return pos;
}

int PartyGame::getBigPos(void)
{
    return m_pos;
}

const static int g_animal[12] = 
{
    0, 0, 0,
    1, 1, 1,
    2, 2, 2,
    3, 3, 3
};

int PartyGame::getAnimalType(int aw_id) const
{
    if(aw_id < static_cast<int>(AWARD_1) || aw_id > static_cast<int>(AWARD_12))
    {
        xt_log.error("getAnimalType error, aw_id:(%d) \n", static_cast<int>(aw_id));
        return 0;
    }
    return g_animal[static_cast<int>(aw_id)];
}

const static int g_color[12] = 
{
    0, 1, 2,
    0, 1, 2,
    0, 1, 2,
    0, 1, 2,
};

int PartyGame::getColorType(int aw_id) const
{
    if(aw_id < static_cast<int>(AWARD_1) || aw_id > static_cast<int>(AWARD_12))
    {
        xt_log.error("getColorType error, aw_id:(%d) \n", static_cast<int>(aw_id));
        return 0;
    }
    return g_color[static_cast<int>(aw_id)];
}


const static int g_award[4][3] = 
{
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},
    {9, 10, 11}
};

int PartyGame::getAwardId(int animal, int color) const
{
    if(animal < RABBIT || animal > LION)
    {
        xt_log.error("getAwardId error, animal:%d \n", animal);
        return 0;
    }

    if(color < RED || color > GREEN)
    {
        xt_log.error("getAwardId error, color:%d \n", color);
        return 0;
    }
    return g_award[animal][color];
}

int PartyGame::getAwardType(void) const
{
    if(m_aw_id >= static_cast<int>(AWARD_13_1) && m_aw_id <= static_cast<int>(AWARD_13_4))  
    {
        return THREE;  
    }

    if(m_aw_id >= static_cast<int>(AWARD_14_1) && m_aw_id <= static_cast<int>(AWARD_14_3))  
    {
        return FOUR;  
    }

    if(!m_aw_set.empty())
    {
        return LAMP; 
    }

    return SINGLE;
}

void PartyGame::procPlayerAuto(void)
{
    for(std::map<int, PartyPlayer*>::iterator it = m_loginPlayers.begin(); it != m_loginPlayers.end(); ++it) 
    {
        PartyPlayer* player = it->second;
        if(!player->m_auto)
        {
            continue;
        }

        player->updateMoney();
        int totalBet = player->getHisoryTotalBet();
        if(player->getMoney() < totalBet) 
        {
            player->m_auto = false;
            continue;
        }

        player->useHistoryBet();
        player->decMoney(totalBet);
        addBetPlayer(it->second);

        for(std::map<int, int>::const_iterator it = player->getBet().begin(); it != player->getBet().end(); ++it)
        {
            changeGameBet(player, it->first, it->second);
        }

    }
}

void PartyGame::updateConfig(void)
{
    //xt_log.info("updateConfig\n");
    m_betLimit = static_cast<int>(getValue("bet_limit", 490000));
    //xt_log.info("m_betLimit:%d\n", m_betLimit);

    split(getStringValue("rb_bettype_weight", "120|110|100|90|80|70|60|50|40|30|20|10"), '|', m_btw);
    split(getStringValue("rb_bet_range", "120|110|100|90|80|70|60|50|40|30|20|10"), '|', m_br);
    split(getStringValue("info", "500,1500,2500,4000,5000"), ',', m_bt);

    /*
    xt_log.info("btw:%s\n", getStringValue("rb_bettype_weight", "not find"));
    for(size_t i = 0; i < m_btw.size(); ++i)
    {
        xt_log.info("index:%d value:%d\n", i, m_btw[i]);
    }
    */

    g_party_ming       = getValue("party_ming_refer", g_party_ming);
    g_party_an         = getValue("party_an_refer", g_party_an);
    //xt_log.info("g_party_ming:%f\n", g_party_ming);
    //xt_log.info("g_party_an:%f\n", g_party_an);

    g_party_ming2      = g_party_ming * 2;
    g_party_an2        = g_party_an * 2;

    m_party_compare = getValue("party_compare", 1.5);
    std::vector<int> tmpVec;
    split(getStringValue("party_range", "4-5"), '-', tmpVec);
    if(tmpVec.size() != 2)
    {
        m_party_range1 = 4; 
        m_party_range2 = 5; 
    }
    else
    {
        m_party_range1 = tmpVec[0]; 
        m_party_range2 = tmpVec[1]; 
    }
    m_party_water = getValue("party_water", 1500000000);
    m_party_check = static_cast<int>(getValue("party_check", 1));
    //xt_log.info("m_party_compare:%f, m_party_range1:%d, m_party_range2:%d, m_party_water:%f, m_party_check:%d\n", m_party_compare, m_party_range1, m_party_range2, m_party_water, m_party_check);
    
    if(m_first)
    {
        m_first = false;
        m_ming             = getValue("party_ming_init", m_ming);
        m_an               = getValue("party_an_init", m_an);
        m_last_an          = m_an;
        //xt_log.info("m_ming:%f\n", m_ming);
        //xt_log.info("m_an:%f\n", m_an);
        //必需在m_party_range1和m_party_range2加载后
        procSpecialInit();        
    }

    //干预参数
    std::vector<int> vecCtrl;
    split(getStringValue("party_control", "1-81000-100"), '-', vecCtrl);
    if(vecCtrl.size() != 3)
    {
        m_ctrl_1 = 1; 
        m_ctrl_2 = 91000; 
        m_ctrl_pro =100;
    }
    else
    {
        m_ctrl_1 = vecCtrl[0]; 
        m_ctrl_2 = vecCtrl[1]; 
        m_ctrl_pro = vecCtrl[2]; 
    }
    //printf("m_ctrl_1: %d, m_ctrl_2: %d, m_ctrl_pro: %d\n", m_ctrl_1, m_ctrl_2, m_ctrl_pro);
}

double PartyGame::getValue(const char* str, double defval) const
{
    if(!m_server)
    {
        xt_log.error("PartyGame getValue, AllKillServer* is null, str:%s.\n", str);
        return defval;
    }

    RedisClient* pCacheRedis = m_server->getCacheRedis();
    if(!pCacheRedis)
    {
        xt_log.error("PartyGame getValue, RedisClient* is null, str:%s. \n", str);
        return defval;
    }

    int ret = pCacheRedis->command("hget %s %s", m_tbname.c_str(), str);
    if(ret < 0)
    {
        xt_log.error("PartyGame getValue error(hget %s %s).\n", m_tbname.c_str(), str);
        return defval;
    }

    if(!pCacheRedis->reply->str)
    {
        return defval;
    }

    return atof(pCacheRedis->reply->str);
}

char* PartyGame::getStringValue(const char* str, char* defval) const 
{
    if(!m_server)
    {
        xt_log.error("PartyGame getValue, AllKillServer* is null, str:%s.\n", str);
        return defval;
    }

    RedisClient* pCacheRedis = m_server->getCacheRedis();
    if(!pCacheRedis)
    {
        xt_log.error("PartyGame getValue, RedisClient* is null, str:%s. \n", str);
        return defval;
    }

    int ret = pCacheRedis->command("hget %s %s", m_tbname.c_str(), str);
    if(ret < 0)
    {
        xt_log.error("PartyGame getValue error(hget %s %s).\n", m_tbname.c_str(), str);
        return defval;
    }

    if(!pCacheRedis->reply->str)
    {
        return defval;
    }

    return pCacheRedis->reply->str;
}

void PartyGame::changeToSpecial(Jpacket& packet)
{
    if(!isSpecialAward(m_aw_id))
    {
        return;
    }

    std::map<int, int> tmp_map;
    Json::Value jval;
    int key = 0;
    int value = 0;
    for(int i = 0; i < static_cast<int>(packet.val["award"].size()); ++i)
    {
        jval = packet.val["award"][i];
        key = jval["aw_id"].asInt();
        value = jval["aw_proba"].asInt();
        tmp_map[key] = value; 
    }

    int tmp_awid = 0;
    for(int i = 0; i < static_cast<int>(packet.val["proba"].size()); ++i)
    {
        tmp_awid = packet.val["proba"][i]["aw_id"].asInt();
        if(tmp_map.find(tmp_awid) != tmp_map.end())
        {
            //            xt_log.info("changeToSpecial aw_id:%d, befor:%d, after:%d\n", tmp_awid, packet.val["proba"][i]["value"].asInt(), tmp_map[tmp_awid]);
            packet.val["proba"][i]["value"] = tmp_map[tmp_awid];
        }
    }
}

void PartyGame::saveResult(void)
{
    std::string sql = "INSERT INTO party_log (create_time, vid, bet_people, award_type, proba, lamp, win, light, ming, an, bet_info, total_in, total_out, award_people) VALUES "; 

    sprintf(g_buf,"(%ld,%d,%d,\'%s\',%d,\'%s\',%d,%d,%f,%f,\'%s\',%d,%d,%d)",
            time(NULL),
            m_vid,
            static_cast<int>(m_betPlayers.size()),
            getAwardName(m_aw_id),
            static_cast<int>(getProba(m_aw_id, m_openLevel)),
            getLampFormat(), 
            static_cast<int>(m_winMoney),
            static_cast<int>(m_light ? 1 : 0),
            m_ming,
            m_an,
            getBetFormat(),
            m_totalIn,
            m_totalOut,
            m_awardPeople
           );

    sql += g_buf;

    //xt_log.info("sql: %s\n", sql.c_str());
    int ret=m_server->getSqlClient()->query(sql.c_str());

    if(ret!=0)
    {
        xt_log.error("sql query insert paryt_log error.\n");
        xt_log.error("sql: %s\n", sql.c_str());
    }
}

void PartyGame::savePlayerResult(void)
{
    for(std::map<int, PartyPlayer*>::iterator it = m_betPlayers.begin(); it != m_betPlayers.end(); ++it)
    {
        m_server->sendPlayerResultFlow(it->first, it->second->getPureReward(), it->second->getMoney()); 
    }
}
        
void PartyGame::saveTop(void)
{
    std::string sql = "INSERT INTO party_top (stamp, content, vid) VALUES "; 
    sprintf(g_buf,"(%ld,\'%s\',%d)", time(NULL), getTopFormat(), m_vid);
    sql += g_buf;

    //printf("sql: %s\n", sql.c_str());
    int ret=m_server->getSqlClient()->query(sql.c_str());

    if(ret!=0)
    {
        //printf("sql query insert paryt_top error.\n");
        //printf("sql: %s\n", sql.c_str());
    }
}

const char* PartyGame::getAwardName(int aw_id) const
{
    switch(aw_id)
    {
        case static_cast<int>(AWARD_1):       return "red_rabbit";
        case static_cast<int>(AWARD_2):       return "yellow_rabbit";
        case static_cast<int>(AWARD_3):       return "green_rabbit";
        case static_cast<int>(AWARD_4):       return "red_monkey";
        case static_cast<int>(AWARD_5):       return "yellow_monkey";
        case static_cast<int>(AWARD_6):       return "green_monkey";
        case static_cast<int>(AWARD_7):       return "red_panda";
        case static_cast<int>(AWARD_8):       return "yellow_panda";
        case static_cast<int>(AWARD_9):       return "green_panda";
        case static_cast<int>(AWARD_10):      return "red_lion";
        case static_cast<int>(AWARD_11):      return "yellow_lion";
        case static_cast<int>(AWARD_12):      return "green_lion";
        case static_cast<int>(AWARD_13_1):    return "rabbit";
        case static_cast<int>(AWARD_13_2):    return "monkey";
        case static_cast<int>(AWARD_13_3):    return "panda";
        case static_cast<int>(AWARD_13_4):    return "lion";
        case static_cast<int>(AWARD_14_1):    return "red";
        case static_cast<int>(AWARD_14_2):    return "yellow";
        case static_cast<int>(AWARD_14_3):    return "green";
    }
    return "noname"; 
}

const char* PartyGame::getLampFormat(void) const
{
    std::string ret = "";
    char buf[20];
    for(std::set<int>::const_iterator it = m_aw_set.begin(); it != m_aw_set.end(); ++it)
    {
        sprintf(buf,"%d", *it);
        ret += buf;  
    }
    return ret.c_str();
}

   
const char* PartyGame::getBetFormat(void) const
{
    std::string ret = "";
    char buf[35];
    for(std::map<int, double>::const_iterator it = m_people_bet.begin(); it != m_people_bet.end(); ++it)
    {
        sprintf(buf,"[%s]:%d-", getAwardName(it->first), static_cast<int>(it->second));
        ret += buf;
    }
    return ret.c_str();
}
        
const char* PartyGame::getTopFormat(void) 
{
    std::string ret = "";
    char buf[35];
    for(std::vector<int>::const_iterator it = m_showTop.begin(); it != m_showTop.end(); ++it)
    {
        PartyPlayer* pp = getPlayer(*it);
        if(pp)
        {
            sprintf(buf,"%d-%d|", *it, pp->getTotalReward());
            ret += buf;
        }
    }
    return ret.c_str();
}

void PartyGame::split(const std::string& s, char c, std::vector<int>& v) 
{
    v.clear();

    string::size_type i = 0;
    string::size_type j = s.find(c);

    while (j != string::npos) {
        v.push_back(atoi(s.substr(i, j-i).c_str()));
        i = ++j;
        j = s.find(c, j);

        if (j == string::npos)
        {
            v.push_back(atoi(s.substr(i, s.length()).c_str()));
        }
    }
}

void PartyGame::splitString(const std::string& s, char c, std::vector<std::string>& v) 
{
    v.clear();

    string::size_type i = 0;
    string::size_type j = s.find(c);

    while (j != string::npos) {
        v.push_back(s.substr(i, j-i));
        i = ++j;
        j = s.find(c, j);

        if (j == string::npos)
        {
            v.push_back(s.substr(i, s.length()));
        }
    }
}
        
void PartyGame::split2(const std::string& s, char c1, char c2, std::vector< std::pair<int, int> >& v)
{
    v.clear();

    std::vector<std::string> tmp;
    splitString(s, c1, tmp);

    std::vector<int> tmp2;
    for(std::vector<string>::iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
        tmp2.clear(); 
        split(*it, c2, tmp2);
        v.push_back(std::make_pair(tmp2[0], tmp2[1]));
    }
}
     
//设置开奖时间(当前时间)，设置检查时间(开奖时间随机后推【4～5】小时))
void PartyGame::procSpecialInit(void) 
{
    //设置开奖和检查时间
    m_award_stamp = time(NULL);
    int diff = (m_party_range2 - m_party_range1) * g_second_hour;
    if(diff <= 0)//配置异常
    {
        m_check_stamp = m_award_stamp + m_party_range1 * g_second_hour; 
    }
    else
    {
        m_check_stamp = m_award_stamp + ((rand() % diff) + m_party_range1 * g_second_hour); 
    }
    //xt_log.info("m_award_stamp:%ld, m_check_stamp:%ld \n", m_award_stamp, m_check_stamp);
    
    //设置开奖暗池值
    m_last_an = m_an;

}
        
bool PartyGame::checkSpecialLimit(void)
{
    //条件1 当前暗池额度＞【1.5】*暗池基准值
    if(m_an <= m_party_compare * g_party_an)
    {
        return false; 
    }

    //条件2 从上次暗池特殊奖开出到现在，总抽水达到【15】亿
    if(m_last_an - m_an <= m_party_water)
    {
        return false;
    }

    return true;
}
        
bool PartyGame::isControl(void) const
{
    if(rand() % 100 > m_ctrl_pro)
    {
        return false;
    }

    int allBet = static_cast<int>(getTotalBet());
    if(allBet < m_ctrl_1 || allBet > m_ctrl_2)
    {
        return false; 
    }

    if(allBet == 0)
    {
        return false;
    }

    return true;
}
        
double PartyGame::getTotalBet(void) const
{
    double result = 0;
    for(std::map<int, double>::const_iterator it = m_people_bet.begin(); it != m_people_bet.end(); ++it)
    {
        result += it->second; 
    }
    return result;
}

//干预处理
void PartyGame::procCtrl(void)
{
    //取一个系统赢随机的解
    int allBet = static_cast<int>(getTotalBet());
    //printf("allBet: %d\n", allBet);
    
    //在已经下注类型中存在解
    std::vector<int> resultBet;
    //未下注类型
    std::vector<int> resultNone;

    int betnu = 0;
    int betprob = 0;
    int awid = 0;
    int awnum = 0;
    int mi_lv = getMingLevel();  

    //设置一个默认值，万一没有必赢选项就使用默认值
    m_aw_id = AWARD_1;

    for(int i = static_cast<int>(AWARD_1); i <= static_cast<int>(AWARD_12); ++i)
    {
        awid = i;
        std::map<int, double>::const_iterator it = m_people_bet.find(awid);
        if(it != m_people_bet.end())
        {
            betnu = it->second; 
            betprob = static_cast<int>(getProba(awid, mi_lv)); 
            awnum = betnu * betprob;
            //找系统必赢选项
            if(awnum < allBet)
            {
                resultBet.push_back(awid);
                //printf("insert resultBet awnum: %d, awid: %d %s\n", awnum, awid, getAwardName(awid));
            }
        }
        else
        {
            resultNone.push_back(awid);
            //printf("insert resultNone awid: %d %s\n", awid, getAwardName(awid));
        }
    }

    if(resultBet.empty())
    {
        m_aw_id = resultNone[rand() % static_cast<int>(resultNone.size())];
    }
    else
    {
        m_aw_id = resultBet[rand() % static_cast<int>(resultBet.size())];
    }

    //printf("result awid: %d %s\n", m_aw_id, getAwardName(m_aw_id));
}

//概率处理
void PartyGame::procNormal(void)
{
    if(isSpecial())
    {
        specialAward();  
    }
    else
    {
        singleAward(); 
    }

    if(isLightning())
    {
        lightningAward(); 
    }

    if(isLamp())
    {
        lampAward();    
    }
}
