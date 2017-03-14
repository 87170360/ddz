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
	m_evFoldTimer.data=this;
	m_evFollowTimer.data=this;
	m_evSeeTimer.data=this;
	m_evWrite.data=this;
	m_evRead.data=this;
	m_evCompareTimer.data=this;
	m_evAllInTimer.data=this;



	m_evloop=evloop;

	m_header=(struct Header*)m_headerBuf;

	m_serverfd=-1;

	m_uid=0;
	m_hasSee=false;
	m_isBetting=false;
	m_isAllIn=false;
	m_seatid=-1;
	m_state=XT_PARSE_HEADER;
	m_curRound=0;

}


XtRobotClient::~XtRobotClient()
{
	if(m_serverfd!=-1)
	{
		ev_io_stop(m_evloop,&m_evWrite);
		ev_io_stop(m_evloop,&m_evRead);
		ev_timer_stop(m_evloop,&m_evFoldTimer);
		ev_timer_stop(m_evloop,&m_evFollowTimer);
		ev_timer_stop(m_evloop,&m_evSeeTimer);
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
	printf("WriteData To Server\n");

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


void XtRobotClient::onDoFold(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;

	self->sendFoldPackage();
}

void XtRobotClient::onDoFollow(struct ev_loop* loop,struct ev_timer* w,int events)
{

	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendFollowPackage();
}

void XtRobotClient::onDoSee(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendSeePackage();
}

void XtRobotClient::onDoCompare(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendComparePacket();
}

void XtRobotClient::onDoAllIn(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendAllInPacket();
}





int XtRobotClient::closeConnect()
{
	if(m_serverfd!=-1)
	{
		ev_io_stop(m_evloop,&m_evWrite);
		ev_io_stop(m_evloop,&m_evRead);
		ev_timer_stop(m_evloop,&m_evFoldTimer);
		ev_timer_stop(m_evloop,&m_evFollowTimer);
		ev_timer_stop(m_evloop,&m_evSeeTimer);
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

	printf("onReciveCmd(%d)\n",cmd);

	switch(cmd)
	{
		case SERVER_GAME_START_BC:
			{
				handleGameStart(val);
				break;
			}

		case SERVER_GAME_END_BC:
			{
				//handleGameEnd(val);
				break;
			}

		case SERVER_ROBOT_CHANGE_UC:
			{
				handleRobotChange(val);
				break;
			}

		case SERVER_TABLE_INFO_UC:
			{
				handleTableInfo(val);
				break;
			}

		case SERVER_BET_SUCC_UC:
			{

				handleBetBc(val);
				break;
			}

		case SERVER_BET_SUCC_BC:
			{
				handleBetBc(val);
				break;
			}

		case SERVER_NEXT_BET_BC:
			{
				handleGameNextBet(val);
			}
	}

//	printf("recive data");
	return 0;
}




void XtRobotClient::handleGameEnd(Json::Value& data)
{
	if(rand()%12<6)
	{
		doChangeTable();
	}
}

void XtRobotClient::handleRobotChange(Json::Value& data)
{
	doChangeTable();
}

void XtRobotClient::handleTableInfo(Json::Value& data)
{
	m_seatid=data["seatid"].asInt();
	m_isBetting=false;
}


void XtRobotClient::handleGameStart(Json::Value& data)
{
	m_isBetting=true;
	m_hasSee=false;
	m_isAllIn=false;
	m_cardType=0;
	m_maxRound=0;
	m_curRound=0;

	for(int i=0;i<5;i++)
	{
		m_seatBettingInfo[i]=0;
	}

	int size= data["seatids"].size();
	for(int i=0;i<size;i++)
	{
		m_seatBettingInfo[data["seatids"][i].asInt()]=1;
	}
}

void XtRobotClient::handleBetBc(Json::Value& data)
{
	int action=data["action"].asInt();

	switch(action)
	{
		case PLAYER_CALL:
			break;

		case PLAYER_RAISE:
			break;

		case PLAYER_COMPARE:
			{

				int seat_id=data["seatid"].asInt();
				int status=data["status"].asInt();

				int target_seatid = data["target_seatid"].asInt();
				int target_status = data["target_status"].asInt();

				if(target_status==2)
				{
					m_seatBettingInfo[target_seatid]=0;
					if(target_seatid==m_seatid)
					{
						m_isBetting=false;
					}
				}
				if(status==2)
				{
					m_seatBettingInfo[seat_id]=0;
					if(seat_id==m_seatid)
					{
						m_isBetting=false;
					}
				}
			}
			break;

		case PLAYER_SEE:
			{
				if(data["uid"].asInt()==m_uid)
				{
					m_cardType=data["card_type"].asInt();
					switch(m_cardType)
					{
						case CARD_TYPE_BAOZI:
						case CARD_TYPE_SHUNJIN:
						case CARD_TYPE_JINHUA:
							m_maxRound=rand()%9+10;
							break;

						case CARD_TYPE_SHUNZI:
							m_maxRound=rand()%4+7;
							break;

						case CARD_TYPE_DUIZI:
							m_maxRound=rand()%4+4;
							break;

						case CARD_TYPE_DANPAI:
						case CARD_TYPE_TESHU:
							m_maxRound=0;
							break;
					}
				}
			}

			break;

		case PLAYER_FOLD:
			{
				int seat_id=data["seatid"].asInt();
				m_seatBettingInfo[seat_id]=0;
			}
			break;

		case PLAYER_ALLIN :
			m_isAllIn=true;

			break;

		case PLAYER_ALLIN_COMPARE :
			break;
	}
}


void XtRobotClient::handleGameNextBet(Json::Value&  val)
{
	if(!m_isBetting)
	{
		return;
	}

	printf("uid=(%d,%d)",m_uid,val["uid"].asInt());
	if(m_uid==val["uid"].asInt())
	{
		m_curRound++;
		int cur_round=val["cur_round"].asInt();
		if(m_curRound>30)
		{
			doFold();
			return;
		}

		if(!m_hasSee)
		{
			if(rand()%10<7||m_isAllIn)
			{
				doSee();
				m_hasSee=true;
			}
			else 
			{
				doFollow();
			}
			return ;
		}

		if(m_cardType==CARD_TYPE_DANPAI||m_cardType==CARD_TYPE_TESHU)
		{
			doFold();
			m_isBetting=false;
			return;
		}

		if(m_isAllIn)
		{
			if(m_cardType==CARD_TYPE_SHUNZI)
			{
				if(rand()%10<6)
				{
					doAllIn();
				}
				else 
				{
					doFold();
				}
			}
			else  if(m_cardType==CARD_TYPE_DUIZI)
			{
				if(rand()%14<2)
				{
					doAllIn();
				}
				else 
				{
					doFold();
				}

			}
			else 
			{
				doAllIn();
			}

			return;
		}

		if(cur_round<m_maxRound)
		{
			doFollow();
			return;
		}
		else 
		{
			doCompare();
			return;
		}
	}
}





void XtRobotClient::doLogin()
{
	sendLoginPackage();
}

void XtRobotClient::doFold()
{
	ev_timer_stop(m_evloop,&m_evFoldTimer);
	ev_timer_set(&m_evFoldTimer,rand()%5+2,0);
	ev_timer_start(m_evloop,&m_evFoldTimer);

}

void XtRobotClient::doFollow()
{
	ev_timer_stop(m_evloop,&m_evFollowTimer);
	ev_timer_set(&m_evFollowTimer,rand()%5+1,0);
	ev_timer_start(m_evloop,&m_evFollowTimer);
}

void XtRobotClient::doSee()
{
	ev_timer_stop(m_evloop,&m_evSeeTimer);
	ev_timer_set(&m_evSeeTimer,rand()%4+1,0);
	ev_timer_start(m_evloop,&m_evSeeTimer);
}


void XtRobotClient::doCompare()
{
	ev_timer_stop(m_evloop,&m_evCompareTimer);
	ev_timer_set(&m_evCompareTimer,rand()%3+2,0);
	ev_timer_start(m_evloop,&m_evCompareTimer);
}

void XtRobotClient::doAllIn()
{
	ev_timer_stop(m_evloop,&m_evAllInTimer);
	ev_timer_set(&m_evAllInTimer,rand()%3+2,0);
	ev_timer_start(m_evloop,&m_evAllInTimer);
}




void XtRobotClient::doChangeTable()
{
	sendChangeTablePackage();
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


	ev_timer_init(&m_evFoldTimer,XtRobotClient::onDoFold,4,0);
	ev_timer_init(&m_evFollowTimer,XtRobotClient::onDoFollow,4,0);
	ev_timer_init(&m_evSeeTimer,XtRobotClient::onDoSee,4,0);
	ev_timer_init(&m_evCompareTimer,XtRobotClient::onDoCompare,4,0);
	ev_timer_init(&m_evAllInTimer,XtRobotClient::onDoAllIn,4,0);

	doLogin();

	return 0;
}


void XtRobotClient::sendLoginPackage()
{
	Jpacket data;
	data.val["cmd"]=CLIENT_LOGIN_REQ;
	data.val["uid"]=m_uid;
	data.val["skey"]="fsdffdf";
	data.end();

	send(data.tostring());
}

void XtRobotClient::sendChangeTablePackage()
{

	Jpacket data;
	data.val["cmd"]=CLIENT_CHANGE_REQ;

	data.end();
	send(data.tostring());
}

void XtRobotClient::sendFoldPackage()
{
	Jpacket data;
	data.val["cmd"] = CLIENT_BET_REQ;
	data.val["action"] = PLAYER_FOLD;
	data.end();
	send(data.tostring());
}

void XtRobotClient::sendSeePackage()
{
	Jpacket data;
	data.val["cmd"] = CLIENT_BET_REQ;
	data.val["action"] = PLAYER_SEE;
	data.end();
	send(data.tostring());
}

void XtRobotClient::sendFollowPackage()
{
	Jpacket data;
	data.val["cmd"] = CLIENT_BET_REQ;
	data.val["action"] = PLAYER_CALL;
	data.end();
	send(data.tostring());
}



void XtRobotClient::sendAllInPacket()
{
	int target_seat_id=getTargetSeatId();
	Jpacket data;
	data.val["cmd"] = CLIENT_BET_REQ;
	data.val["action"] = PLAYER_ALLIN_COMPARE;
	data.val["seatid"]=m_seatid;
	data.val["target_seatid"]=target_seat_id;
	data.end();
	send(data.tostring());
}

void XtRobotClient::sendComparePacket()
{
	int target_seat_id=getTargetSeatId();
	Jpacket data;
	data.val["cmd"] = CLIENT_BET_REQ;
	data.val["action"] = PLAYER_COMPARE;
	data.val["seatid"]=m_seatid;
	data.val["target_seatid"]=target_seat_id;
	data.end();
	send(data.tostring());
}


int XtRobotClient::getTargetSeatId()
{
	std::vector<int> bets;

	for(int i=0;i<5;i++)
	{
		if((m_seatBettingInfo[i]==1)&&(i!=m_seatid))
		{
			bets.push_back(i);
		}
	}

	random_shuffle(bets.begin(),bets.end());

	if(bets.size()>0)
	{
		return bets[0];
	}

	return m_seatid;
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

