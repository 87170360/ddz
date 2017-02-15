#include "XtGameClient.h"

#include "XtBuffer.h"
#include "XtJsonPacket.h"
#include "protocol/XtMsgProtocol.h"
#include "XtEncrypt.h"
#include "XtGameServer.h"
#include "XtLog.h"


XtGameClient::XtGameClient(struct ev_loop* loop,XtGameServer* server)
	:XtStreamClient(loop)
{

	m_uid=0;
	m_protocol=NULL;
	m_server=server;
	m_player=NULL;

}

XtGameClient::~XtGameClient()
{
	if(m_protocol)
	{
		delete m_protocol;
	}
}


int XtGameClient::send(const void* data,unsigned int length)
{
	if(m_isClose)
	{
		return -1;
	}

	if(!m_protocol)
	{
		return -1;
	}

	XtBuffer* buffer=m_protocol->pack((uint8_t*)data,length);
	XtStreamClient::send(buffer,false);
	return 0;
}




void XtGameClient::setPlayer(XtGamePlayer* player)
{
	m_player=player;
}


XtGamePlayer* XtGameClient::getPlayer()
{
	return m_player;
}


void XtGameClient::setProtocol(XtProtocol* protocol)
{
	if(m_protocol)
	{
		delete m_protocol;
		m_protocol=NULL;
	}

	m_protocol=protocol;
}


XtProtocol* XtGameClient::getProtocol()
{
	return m_protocol;
}


void XtGameClient::onReciveData(void* data,int len)
{
	if(!m_protocol)
	{
		return;
	}


	m_protocol->append((uint8_t*)data,len);

	std::vector<XtBuffer*> buffers;

	while(true)
	{
		XtBuffer* packet=m_protocol->unpack();

		if(!packet)
		{
			break;
		}
		buffers.push_back(packet);
	}
	int buffer_nu=buffers.size();

	if(buffer_nu>0)
	{
		onRecivePacket(&buffers[0],buffer_nu);

		for(int i=0;i<buffer_nu;i++)
		{
			delete buffers[i];
		}
		buffers.resize(0);
	}
}


void XtGameClient::onClose()
{
	if(m_server)
	{
		m_server->removeClient(this);
	}

}



void XtGameClient::onRecivePacket(XtBuffer** packet,int num)
{
	if(!m_server)
	{
		return;
	}


	XtGameServer* server=m_server;
	for(int i=0;i<num;i++)
	{
		server->onReciveClientCmd(this,packet[i]);
	}
}


