#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <ev.h>

#include "PartyClient.h"
#include "PartyMacros.h"
#include "log.h"

Log xt_log;

static std::map<int, const char*> g_cmd;
static std::map<int, const char*> g_awardName;
static std::map<int, const char*> g_animalName;
static std::map<int, const char*> g_colorName;
static std::map<int, const char*> g_awardTypeName;
static std::map<int, const char*> g_statusName;
//static bool g_firstBet = true;
static int g_uid = 0;
static bool g_sendChat = false;  //机器人聊天
//机器人下注类型权重
static std::vector<int> g_btw; 
static int g_total_btw = 0;
//机器人下注范围
static std::vector<int> g_br; 
//下注额度类型
static std::vector<int> g_bt; 

static const char* getName(std::map<int, const char*>& namelist, int type, const char* typeDesc)
{
    if(namelist.find(type) == namelist.end())
    {
        printf("%s desc not find, type:%d \n", typeDesc, type);
        return "none";  
    }
    return namelist[type];
}

static void parseRobotConf(const Jpacket& packet)
{
    g_btw.clear();
    g_total_btw = 0;
    for(int i = 0; i < static_cast<int>(packet.val["btw"].size()); ++i)
    {
        g_btw.push_back(packet.val["btw"][i].asInt());
        g_total_btw += packet.val["btw"][i].asInt();
    }

    g_br.clear();
    for(int i = 0; i < static_cast<int>(packet.val["br"].size()); ++i)
    {
        g_br.push_back(packet.val["br"][i].asInt());
    }

    g_bt.clear();
    for(int i = 0; i < static_cast<int>(packet.val["bt"].size()); ++i)
    {
        g_bt.push_back(packet.val["bt"][i].asInt());
    }
}

static int getRobotBetNu(int idx)
{
   int tmpBr = g_br[idx]; 

   int tmpRand = rand() % static_cast<int>(g_bt.size()); 

   return tmpBr * g_bt[tmpRand];
}

static int getRobotBetAwid(void)
{
   int val = rand() % g_total_btw; 
   int tmp = 0;
   for(int i = 0; i < static_cast<int>(g_btw.size()); ++i)
   {
        tmp += g_btw[i]; 
        if(tmp > val)
        {
            return i; 
        }
   }
   return 0;
}

static void sendBetMsg(PartyClient* client)
{
    int aw_id = getRobotBetAwid();
    int bet_nu = getRobotBetNu(aw_id);
    Jpacket packet;
    packet.val["cmd"]       =   PA_BET_C;
    packet.val["aw_id"]     =   aw_id;
    packet.val["aw_pos"]    =   1;
    packet.val["bet_nu"]    =   bet_nu;
    packet.end();
    client->send(packet.tostring());
    printf("robot bet, aw_id: %s, bet_nu: %d\n", getName(g_awardName, aw_id, "get award name"), bet_nu);
}

class PosInfo
{
    public:
        void operator = (const PosInfo& rhs)
        {
            color = rhs.color; 
            animal = rhs.animal;
        }
        int color;
        int animal;
};

static std::map<int, PosInfo> g_posInfo;

static void server_close(PartyClient* client,void* data)
{
    fprintf(stderr,"client close\n");
    exit(0);
}

static void printData(Jpacket& packet)
{
    int cmd = packet.val["cmd"].asInt();
    if(g_cmd.find(cmd) == g_cmd.end())
    {
        printf("cmd desc not find, %d: \n", cmd);
        return;  
    }

    const char* cmdname = getName(g_cmd, cmd, "cmd");
    printf("cmd: %s\n", cmdname);
    switch(cmd)
    {
        case PA_START_B:
            {
                printf("recive data from party server:%s\n", packet.val.toStyledString().c_str());
            }
            break;
        case PA_BET_S:
            {
                //printf("recive data from party server:%s\n", packet.val.toStyledString().c_str());
            }
            break;
        case PA_PREPARE_B:
            {
                //printf("recive data from party server:%s\n", packet.val.toStyledString().c_str());
                int remain = packet.val["remain"].asInt();
                int bonus = packet.val["bouns"].asInt();
                printf("remain: %d, bonus: %d\n", remain, bonus);
                g_posInfo.clear();
                for(int i = 0; i < static_cast<int>(packet.val["posInfo"].size()); ++i)
                {
                    PosInfo posinfo;
                    posinfo.animal  = packet.val["posInfo"][i]["animal"].asInt();
                    posinfo.color   = packet.val["posInfo"][i]["color"].asInt();
                    g_posInfo[i]    = posinfo; 
                    printf("animal: %s, color: %s\n", getName(g_animalName, posinfo.animal, "animal"), getName(g_colorName, posinfo.color, "color"));
                }

                //proba
                printf("proba:\n");
                for(int i = 0; i < static_cast<int>(packet.val["proba"].size()); ++i)
                {
                    printf("    awardname: %s, value:: %d\n", getName(g_awardName, packet.val["proba"][i]["aw_id"].asInt(), "awardname"), packet.val["proba"][i]["value"].asInt());
                }

                //bonus
                printf("bonus: %d\n", packet.val["bonus"].asInt());
            }
            break;
        case PA_END_B:
            {
                //printf("recive data from party server:%s\n", packet.val.toStyledString().c_str());
                int big_pos = packet.val["big_pos"].asInt();
                int aw_pos = packet.val["aw_pos"].asInt();
                printf("big_pos: %d, aw_pos: %d\n", big_pos, aw_pos);

                //外盘旋转后情况
                std::map<int, PosInfo> tmp;
                for(std::map<int, PosInfo>::const_iterator it = g_posInfo.begin(); it != g_posInfo.end(); ++it)
                {
                    PosInfo info;
                    info.animal = it->second.animal;
                    info.color = g_posInfo[(it->first + big_pos) % 24].color;
                    tmp[(it->first + big_pos) % 24] = info; 
                }

                /*
                   printf("pos info:\n");
                   for(std::map<int, PosInfo>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
                   {
                   printf(" animal: %s, color: %s\n", getName(g_animalName, it->second.animal, "animal"), getName(g_colorName, it->second.color, "color"));
                   }
                   */

                PosInfo cur = tmp[aw_pos];
                printf("cur pos info, animal: %s, color: %s\n", getName(g_animalName, cur.animal, "animal"), getName(g_colorName, cur.color, "color"));

                //aw_type
                printf("aw_type: %s\n", getName(g_awardTypeName, packet.val["aw_type"].asInt(), "aw_type"));

                //aw_light
                printf("aw_light: %s\n", packet.val["aw_light"].asBool() ? "true" : "false" );

                //award
                printf("award:\n");
                for(int i = 0; i < static_cast<int>(packet.val["award"].size()); ++i)
                {
                    printf("    awardname: %s, aw_proba: %d\n", getName(g_awardName, packet.val["award"][i]["aw_id"].asInt(), "awardname"), packet.val["award"][i]["aw_proba"].asInt());
                }

                //history
                printf("history:\n");
                for(int i = 0; i < static_cast<int>(packet.val["history"].size()); ++i)
                {
                    printf("    awardname: %s, lightning: %d\n", getName(g_awardName, packet.val["history"][i]["aw_id"].asInt(), "awardname"), packet.val["history"][i]["lightning"].asInt());
                }

                //player bet
                printf("playerBet:\n");
                for(int i = 0; i < static_cast<int>(packet.val["playerBet"].size()); ++i)
                {
                    printf("    awardname: %s, bet_num: %d\n", getName(g_awardName, packet.val["playerBet"][i]["aw_id"].asInt(), "awardname"), packet.val["playerBet"][i]["bet_num"].asInt());
                }

                //proba
                printf("proba:\n");
                for(int i = 0; i < static_cast<int>(packet.val["proba"].size()); ++i)
                {
                    printf("    awardname: %s, value:: %d\n", getName(g_awardName, packet.val["proba"][i]["aw_id"].asInt(), "awardname"), packet.val["proba"][i]["value"].asInt());
                }

                //top
                printf("top:\n");
                for(int i = 0; i < static_cast<int>(packet.val["top"].size()); ++i)
                {
                    printf("    name: %s, reward: %d, uid: %d\n", packet.val["top"][i]["name"].asCString(), packet.val["top"][i]["reward"].asInt(), packet.val["top"][i]["uid"].asInt());
                }
            }
            break;
        case PA_UPDATE_B:
            {
                /*
                   printf("remain: %d\n", packet.val["remain"].asInt());
                   for(int i = 0; i < static_cast<int>(packet.val["betInfo"].size()); ++i)
                   {
                   printf("    awardname: %s, bet_num: %d\n", getName(g_awardName, packet.val["betInfo"][i]["aw_id"].asInt(), "awardname"), packet.val["betInfo"][i]["bet_num"].asInt());
                   }
                   */
            }
            break;
        case PA_LOGIN_S:
            {
                //printf("recive data from party server:%s\n", packet.val.toStyledString().c_str());
                //proba
                printf("proba:\n");
                for(int i = 0; i < static_cast<int>(packet.val["proba"].size()); ++i)
                {
                    printf("    awardname: %s, value:: %d\n", getName(g_awardName, packet.val["proba"][i]["aw_id"].asInt(), "awardname"), packet.val["proba"][i]["value"].asInt());
                }

                //status remain
                printf("    ret: %d, remain: %d, status: %s\n", packet.val["ret"].asInt(), packet.val["remain"].asInt(), getName(g_statusName, packet.val["status"].asInt(), "status"));   

                //top
                printf("top:\n");
                for(int i = 0; i < static_cast<int>(packet.val["top"].size()); ++i)
                {
                    printf("    name: %s, reward: %d, uid: %d\n", packet.val["top"][i]["name"].asCString(), packet.val["top"][i]["reward"].asInt(), packet.val["top"][i]["uid"].asInt());
                }

                //limit
                printf("limit: %d\n", packet.val["limit"].asInt());

                //bonus
                printf("bonus: %d\n", packet.val["bonus"].asInt());

                //history
                printf("history:\n");
                for(int i = 0; i < static_cast<int>(packet.val["history"].size()); ++i)
                {
                    printf("    aw_id: %d, lightning: %s, fresh: %s\n", packet.val["history"][i]["aw_id"].asInt(), (packet.val["history"][i]["lightning"].asBool() ? "true" : "false"), (packet.val["history"][i]["fresh"].asBool() ? "true" : "false"));
                }
            }
            break;
        case PA_CHAT_B:
            {
                printf("recive data from party server:%s\n", packet.val.toStyledString().c_str());
            }
            break;
    }
    printf("=============================================\n");
}

static void recivie_data(PartyClient* client, void* data, Jpacket& packet1)
{
    printData(packet1);

    int cmd = packet1.val["cmd"].asInt();

    if(cmd == PA_START_B || cmd == PA_LOGIN_S)
    {
        parseRobotConf(packet1);
    }

    if(cmd == PA_START_B)
    {
        sendBetMsg(client);
    }

    if(cmd == PA_UPDATE_B)
    {
        sendBetMsg(client);
    }

    if(cmd == PA_UPDATE_B && g_sendChat)
    {
        Jpacket packet;
        packet.val["cmd"]           =   PA_CHAT_C;
        packet.val["content"]       =   "I am a robot!";
        packet.end();
        client->send(packet.tostring());
    }
}

void init(void)
{
    g_cmd[PA_START_B]      = "PA_START_B";
    g_cmd[PA_END_B]        = "PA_END_B";
    g_cmd[PA_LOGIN_C]      = "PA_LOGIN_C";
    g_cmd[PA_LOGIN_S]      = "PA_LOGIN_S";
    g_cmd[PA_BET_C]        = "PA_BET_C";
    g_cmd[PA_BET_S]        = "PA_BET_S";
    g_cmd[PA_UPDATE_B]     = "PA_UPDATE_B";
    g_cmd[PA_CHAT_C]       = "PA_CHAT_C";
    g_cmd[PA_CHAT_B]       = "PA_CHAT_B";
    g_cmd[PA_LOGOUT_C]     = "PA_LOGOUT_C";
    g_cmd[PA_LOGOUT_S]     = "PA_LOGOUT_S";
    g_cmd[PA_CANCLE_C]     = "PA_CANCLE_C";
    g_cmd[PA_CANCLE_S]     = "PA_CANCLE_S";
    g_cmd[PA_FOLLOW_C]     = "PA_FOLLOW_C";
    g_cmd[PA_FOLLOW_S]     = "PA_FOLLOW_S";
    g_cmd[PA_AUTO_S]       = "PA_AUTO_S";
    g_cmd[PA_AUTO_C]       = "PA_AUTO_C";
    g_cmd[PA_PREPARE_B]    = "PA_PREPARE_B";

    g_awardName[AWARD_1]       = "红兔子";
    g_awardName[AWARD_2]       = "黄兔子";
    g_awardName[AWARD_3]       = "绿兔子";
    g_awardName[AWARD_4]       = "红猴子";
    g_awardName[AWARD_5]       = "黄猴子";
    g_awardName[AWARD_6]       = "绿猴子";
    g_awardName[AWARD_7]       = "红熊猫";
    g_awardName[AWARD_8]       = "黄熊猫";
    g_awardName[AWARD_9]       = "绿熊猫";
    g_awardName[AWARD_10]      = "红狮子";
    g_awardName[AWARD_11]      = "黄狮子";
    g_awardName[AWARD_12]      = "绿狮子";
    g_awardName[AWARD_13_1]    = "兔子";
    g_awardName[AWARD_13_2]    = "猴子";
    g_awardName[AWARD_13_3]    = "熊猫";
    g_awardName[AWARD_13_4]    = "狮子";
    g_awardName[AWARD_14_1]    = "红色";
    g_awardName[AWARD_14_2]    = "黄色";
    g_awardName[AWARD_14_3]    = "绿色";

    g_animalName[RABBIT]   = "兔";
    g_animalName[MONKEY]   = "猴";
    g_animalName[PANDA]    = "熊";
    g_animalName[LION]     = "狮";

    g_colorName[RED]       = "红";
    g_colorName[YELLOW]    = "黄";
    g_colorName[GREEN]     = "绿";

    g_awardTypeName[SINGLE]          = "单项";
    g_awardTypeName[THREE]           = "大三元";
    g_awardTypeName[FOUR]            = "大四喜";
    g_awardTypeName[LAMP]            = "送灯";

    g_statusName[STATE_START]        = "游戏进行中";
    g_statusName[STATE_END]          = "游戏结算中";
    g_statusName[STATE_PREPARE]      = "游戏准备中";
}



int main(int argc,char** argv)
{
    if(argc != 4)
    {
        printf("useage %s <ip> <port> <uid>\n",argv[0]);
        exit(0);
    }

    srand(time(NULL)+getpid());
    init();

    int socket_fd;
    struct sockaddr_in serv_addr;

    socket_fd=socket(AF_INET,SOCK_STREAM,0);
    if(socket_fd==-1)
    {
        printf("create Socket failed\n");
        return -1;
    }

    char* ip=argv[1];
    int port=atoi(argv[2]);

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr=inet_addr(ip);

    memset(&serv_addr.sin_zero,0,8);

    if(connect(socket_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))==-1)
    {
        printf("connect to server failed, port:%d, ip:%s\n", port, ip);
        return -1;
    }

    struct ev_loop* loop = ev_loop_new(0);

    PartyClient* client = new PartyClient(loop);
    client->setOnReciveCmdFunc(recivie_data,NULL);
    client->setOnCloseFunc(server_close,NULL);
    client->connectStart(socket_fd);

    int uid = atoi(argv[3]);
    g_uid = uid;
    Jpacket packet;
    if(uid == -1)
    {
        //packet.val["cmd"]=AK_ASK_SERVER_SHUT_DOWN;
        packet.val["cmd"]=PA_LOGIN_C;
    }
    else 
    {
        packet.val["cmd"]=PA_LOGIN_C;
    }

    packet.val["uid"]=uid;
    packet.end();
    client->send(packet.tostring());
    ev_loop(loop,0);
    ev_loop_destroy(loop);
    delete client;

    return 0;
}
