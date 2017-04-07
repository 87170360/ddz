#include <algorithm>
#include <vector>
#include "XtRobotClient.h"


#include<sys/socket.h>
#include <unistd.h>
#include <string.h> 
#include<stdio.h>
#include<sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "proto.h"
#include "jpacket.h"

XtRobotClient::XtRobotClient(struct ev_loop* evloop)
{
    m_evWrite.data=this;
    m_evRead.data=this;
    m_showTimer.data = this;

    m_evloop=evloop;

    m_header=(struct Header*)m_headerBuf;

    m_serverfd=-1;

    m_state=XT_PARSE_HEADER;
}


XtRobotClient::~XtRobotClient()
{
    if(m_serverfd!=-1)
    {
        ev_io_stop(m_evloop,&m_evWrite);
        ev_io_stop(m_evloop,&m_evRead);
        ev_timer_stop(m_evloop, &m_showTimer);
        close(m_serverfd);
    }

    while(!m_writeQueue.empty())
    {
        delete m_writeQueue.front();
        m_writeQueue.pop_front();
    }
}

void XtRobotClient::onReadData( struct ev_loop* loop, struct ev_io* w, int revents)
{
    int ret;
    static char recv_buf[XT_DEF_BUF_LEN];
    XtRobotClient* self = (XtRobotClient*) w->data;

    if (self->m_state == XT_PARSE_HEADER) 
    {
        ret = read(self->m_serverfd, &self->m_headerBuf[self->m_curHeaderLen],
                sizeof(struct Header) - self->m_curHeaderLen);

        if (ret < 0) 
        {
            if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
            {
                printf("read cb read header failed[%s]\n", strerror(errno));
                return;
            }

            self->closeConnect();

            return;
        }


        if (ret == 0) 
        {
            printf("connection close in read header[%d]\n", self->m_serverfd);
            self->closeConnect();
            return;
        }

        self->m_curHeaderLen+= ret;

        if (self->m_curHeaderLen== sizeof(struct Header)) 
        {
            if (self->m_header->m_length > XT_MAX_BUF_LEN || self->m_header->m_length == 0) 
            {
                self->closeConnect();
                return;
            }

            self->m_state = XT_PARSE_BODY;
            self->m_curHeaderLen= 0;
            self->m_body.clear();
        }
    } 
    else if (self->m_state == XT_PARSE_BODY) 
    {
        ret = read(self->m_serverfd, recv_buf, self->m_header->m_length - self->m_body.length());


        if (ret < 0) 
        {
            if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
            {
                printf("read body failed[%s]\n", strerror(errno));
                return;
            }
            printf("read body failed[%s]\n", strerror(errno));
            self->closeConnect();
            return;
        }

        if (ret == 0) 
        {
            printf("connection close in read body[%d]\n", self->m_serverfd);
            self->closeConnect();
            return;
        }


        recv_buf[ret] = '\0';
        self->m_body.append(recv_buf);

        if (self->m_body.length() == self->m_header->m_length) 
        {
            self->m_state = XT_PARSE_HEADER;
            if (self->m_packet.parse(self->m_body) < 0) 
            {
                printf("parse err!!\n");
                self->closeConnect();
                return;
            }

            self->onReciveCmd(self->m_packet);

        }
    } 

}

void XtRobotClient::onWriteData(struct ev_loop *loop, struct ev_io *w, int revents)
{

    XtRobotClient* self = (XtRobotClient*) w->data;

    if (self->m_writeQueue.empty()) 
    {
        ev_io_stop(EV_A_ w);
        return;
    }
    //printf("WriteData To Server\n");

    XtBuffer* buffer = self->m_writeQueue.front();

    ssize_t written = write(self->m_serverfd, buffer->m_data, buffer->m_len);

    if (written < 0) {
        if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
            printf("write failed[%s]\n", strerror(errno));
            return;
        }
        /* todo close this client */
        printf("unknow err in written [%d]\n", self->m_serverfd);
        self->closeConnect();
        return;
    }

    self->m_writeQueue .pop_front();
    delete buffer;
}

void XtRobotClient::tfShow(struct ev_loop* loop, struct ev_timer* w, int events)
{
    printf("showtimer active.\n");
    ev_timer_stop(loop,w);
    XtRobotClient* self = (XtRobotClient*) w->data;
    self->sendCall();
}

int XtRobotClient::closeConnect()
{
    if(m_serverfd!=-1)
    {
        ev_io_stop(m_evloop,&m_evWrite);
        ev_io_stop(m_evloop,&m_evRead);
        ev_timer_stop(m_evloop, &m_showTimer);
        close(m_serverfd);
    }
    m_serverfd=-1;
    while(!m_writeQueue.empty())
    {
        delete m_writeQueue.front();
        m_writeQueue.pop_front();
    }
    return 0;
}


int XtRobotClient::onReciveCmd(Jpacket& data)
{
    Json::Value &val = data.tojson();
    int cmd = val["cmd"].asInt();
    switch(cmd)
    {
        case SERVER_RESPOND:
            handleRespond(val);
            break;
        case SERVER_CARD_1:
            handleCall(val);
            break;
        case SERVER_AGAIN_CALL:
            handleAgainCall(val);
            break;
        case SERVER_RESULT_CALL:
            handleDouble(val);
            break;
        case SERVER_DOUBLE:
            break;
        case SERVER_RESULT_DOUBLE:
            handleOut(val);
            break;
        case SERVER_AGAIN_OUT:
            handleAgainOut(val);
            break;
        case SERVER_REPREPARE:
            handleReprepare(val);
            break;
        case SERVER_END:
            handleEnd(val);
            break;
        case SERVER_KICK:
            handleKick(val);
            break;
        case SERVER_TIME:
            handleTime(val);
            break;
    }

    return 0;
}

void XtRobotClient::vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
    if (cards.empty()) 
    {
        //packet.val[key].append(0);
        return;
    }

    for (unsigned int i = 0; i < cards.size(); i++) 
    {
        packet.val[key].append(cards[i].m_value);
    }
}

void XtRobotClient::map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key)
{
    std::map<int, XtCard>::iterator it;
    for (it = cards.begin(); it != cards.end(); it++)
    {
        XtCard &card = it->second;
        packet.val[key].append(card.m_value);
    }
}

/*
   void XtRobotClient::json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key)
   {
   Json::Value &val = packet.tojson();

   for (unsigned int i = 0; i < val[key].size(); i++)
   {
   XtCard card(val[key][i].asInt());

   cards.push_back(card);
   }
   }
   */

void XtRobotClient::json_array_to_vector(std::vector<XtCard> &cards, Json::Value &val, string key)
{
    for (unsigned int i = 0; i < val[key].size(); i++)
    {
        XtCard card(val[key][i].asInt());

        cards.push_back(card);
    }
}

void XtRobotClient::handleRespond(Json::Value& msg) 
{
    int msgid = msg["msgid"].asInt();
    int code = msg["code"].asInt();
    //printf("msgid:%d\n", msgid);
    //printf("code:%d\n", code);
    switch(msgid)
    {
        case CLIENT_LOGIN:
            {
                if(code == CODE_SUCCESS) 
                {
                    Jpacket data;
                    data.val["cmd"]     =   CLIENT_PREPARE;
                    data.end();
                    send(data.tostring());
                }
            }
            break;
    }

    if(code != CODE_SUCCESS)
    {
        printf("msgid:%d, code:%d\n", msgid, code);
    }
}

void XtRobotClient::handleCall(Json::Value& msg) 
{
    m_card.clear();
    json_array_to_vector(m_card, msg, "card");
    XtCard::sortByDescending(m_card);

    if(msg["cur_id"].asInt() != m_uid)
    {
        return;
    }
    int show_time = msg["show_time"].asInt();
    ev_timer_stop(m_evloop, &m_showTimer);
    ev_timer_set(&m_showTimer, show_time, 0);
    ev_timer_start(m_evloop, &m_showTimer);
    printf("handle call, showtimer active after %d second.\n", show_time);
}

void XtRobotClient::handleAgainCall(Json::Value& msg) 
{
    if(msg["cur_id"].asInt() != m_uid)
    {
        return;
    }
    Jpacket data;
    data.val["cmd"]     =   CLIENT_CALL;
    data.val["score"]   =   msg["score"].asInt() + 1;
    data.val["score"]   =   0;
    data.end();

    send(data.tostring());
}

void XtRobotClient::handleDouble(Json::Value& msg) 
{
    if(msg["lord"].asInt() == m_uid)
    {
        json_array_to_vector(m_card, msg, "card");
        XtCard::sortByDescending(m_card);
        return;
    }

    Jpacket data;
    data.val["cmd"]     =   CLIENT_DOUBLE;
    data.val["double"]  =   true;
    data.end();
    send(data.tostring());
}

void XtRobotClient::handleOut(Json::Value& msg) 
{
    if(msg["cur_id"].asInt() != m_uid)
    {
        return;
    }

    //return;

    Jpacket data;
    data.val["cmd"]     =   CLIENT_OUT;

    vector<XtCard> outCard;
    m_deck.getFirst(m_card, outCard);
    vector_to_json_array(outCard, data, "card");
    data.val["keep"]     =   outCard.empty();
    data.end();

    send(data.tostring());
}

void XtRobotClient::handleAgainOut(Json::Value& msg)
{
    vector<XtCard> preCard;
    json_array_to_vector(preCard, msg, "card");

    //删除自己上次出的牌
    if(msg["pre_id"].asInt() == m_uid && !msg["keep"].asBool())
    {
        //remove 
        vector<XtCard> newCard;
        bool find = false;
        for(vector<XtCard>::iterator it1 = m_card.begin(); it1 != m_card.end(); ++it1)
        {
            find = false;
            for(vector<XtCard>::iterator it2 = preCard.begin(); it2 != preCard.end(); ++it2)
            {
                if(it1->m_face == it2->m_face && it1->m_suit == it2->m_suit) 
                {
                    find = true;
                    break;
                }
            }
            if(!find)
            {
                newCard.push_back(*it1);
            }
        }
        m_card = newCard;
    }

    //return;

    //自己上轮的牌没人顶
    if(msg["cur_id"].asInt() == m_uid && msg["out_id"].asInt() == m_uid) 
    {
        Jpacket data;
        data.val["cmd"]     =   CLIENT_OUT;
        vector<XtCard> outCard;
        m_deck.getFirst(m_card, outCard);
        vector_to_json_array(outCard, data, "card");
        data.val["keep"]     =   outCard.empty();
        data.end();
        send(data.tostring());
    }
    else if(msg["cur_id"].asInt() == m_uid && msg["out_id"].asInt() != m_uid) 
    {//别人出牌
        vector<XtCard> outCard;
        outCard.clear();

        XtCard::sortByDescending(m_card);
        XtCard::sortByDescending(preCard);
        bool isOut = m_deck.getOut(m_card, preCard, outCard);
        Jpacket data;
        data.val["cmd"]     =   CLIENT_OUT;
        data.val["keep"]    =   !isOut;
        vector_to_json_array(outCard, data, "card");
        data.end();
        send(data.tostring());
    }
}

void XtRobotClient::handleReprepare(Json::Value& msg)
{
    m_card.clear();
    Jpacket data;
    data.val["cmd"]     =   CLIENT_PREPARE;
    data.end();
    send(data.tostring());
}

void XtRobotClient::handleEnd(Json::Value& msg)
{
    //printf("handleEnd !\n");
    m_card.clear();
    Jpacket data; 
    //data.val["cmd"]     =   (rand() % 2) > 0 ? CLIENT_CHANGE : CLIENT_PREPARE;
    data.val["cmd"]     =   CLIENT_PREPARE;
    //data.val["cmd"]     =   CLIENT_CHANGE;
    //data.val["cmd"]     =   CLIENT_LOGOUT;
    data.end();
    send(data.tostring());
}

void XtRobotClient::handleKick(Json::Value& msg)
{
    printf("handleKick !\n");
    int uid = msg["uid"].asInt();
    if(m_uid == uid)
    {
        Jpacket data; 
        data.val["cmd"]     =   CLIENT_CHANGE;
        data.end();
        send(data.tostring());
    }
}

void XtRobotClient::handleTime(Json::Value& msg)
{
    int time = msg["time"].asInt();
    //printf("handleTime!, time:%d\n", time);
}

void XtRobotClient::sendCall(void)
{
    Jpacket data;
    data.val["cmd"]     =   CLIENT_CALL;
    data.val["score"]   =   rand() % 2;
    data.end();

    send(data.tostring());
}

void XtRobotClient::doLogin()
{
    sendLoginPackage();
}

int XtRobotClient::connectToServer(const char* ip,int port,int uid)
{

    int socket_fd;
    struct sockaddr_in serv_addr;

    socket_fd=socket(AF_INET,SOCK_STREAM,0);
    if(socket_fd==-1)
    {
        printf("create Socket failed\n");
        return -1;
    }

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr=inet_addr(ip);
    memset(&serv_addr.sin_zero,0,8);

    if(connect(socket_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))==-1)
    {
        printf("connect to server failed\n");
        return -1;
    }



    m_serverfd=socket_fd;
    m_uid=uid;

    ev_io_init(&m_evRead,XtRobotClient::onReadData,m_serverfd,EV_READ);
    ev_io_start(m_evloop,&m_evRead);

    ev_io_init(&m_evWrite,XtRobotClient::onWriteData,m_serverfd,EV_WRITE);

    ev_timer_init(&m_showTimer, XtRobotClient::tfShow, 2, 0);

    doLogin();

    return 0;
}


void XtRobotClient::sendLoginPackage()
{
    Jpacket data;
    data.val["cmd"]=CLIENT_LOGIN;
    data.val["uid"]=m_uid;
    data.val["skey"]="fsdffdf";
    data.end();

    send(data.tostring());
}

int XtRobotClient::send(const char *buf, unsigned int len)
{
    if (m_serverfd>=0)
    {
        if (m_writeQueue.empty()) 
        {
            m_evWrite.data = this;
            ev_io_start(m_evloop, &m_evWrite);
        }
        m_writeQueue.push_back(new XtBuffer(buf, len));
        return 0;
    }

    printf("server error\n");

    return -1;
}

int XtRobotClient::send(const std::string &res)
{
    return send(res.c_str(), res.length());
    //return safe_writen(res.c_str(), res.length());
}

