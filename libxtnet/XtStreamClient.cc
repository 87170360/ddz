#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <list>

#include "XtStreamClient.h"
#include "XtLog.h"
#include "XtBuffer.h"

#define XT_STREAM_READ_BUF_LEN (1024*4)

static void XtStreamClient_onReciveData(struct ev_loop* loop,struct ev_io* w,int revents )
{

	XtStreamClient* c = (XtStreamClient*)w->data;
	c->readData();
}


static void XtStreamClient_onWriteData(struct ev_loop* loop,struct ev_io* w,int events)
{
	XtStreamClient* c = (XtStreamClient*)w->data;
	c->writeData();
}



XtStreamClient::XtStreamClient(struct ev_loop* loop)
{
	m_clientFd=-1;
	m_isClose=true;
	m_evLoop=loop;
}

XtStreamClient::~XtStreamClient()
{
	ev_io_stop(m_evLoop,&m_evRead);
	ev_io_stop(m_evLoop,&m_evWrite);

	clearWriteBuffer();

	if(!m_isClose&&m_clientFd>0)
	{
		close(m_clientFd);
		XT_LOG_INFO("client fd(%d) destrutor\n",m_clientFd);
	}
}




int XtStreamClient::send(const void* buf,unsigned int len)
{
	if(m_isClose)
	{
		XT_LOG_ERROR("client(%d) is close\n",m_clientFd);
	}
	else 
	{
		if(m_writeQueue.empty())
		{
			ev_io_start(m_evLoop,&m_evWrite);
		}
		m_writeQueue.push_back(new XtBuffer((const uint8_t*)buf,len));
		return 0;
	}
	return -1;
}

int XtStreamClient::send(XtBuffer* buffer,int clone)
{

	if(m_isClose)
	{
		XT_LOG_ERROR("client(%d) is close\n",m_clientFd);
	}
	else 
	{
		XtBuffer* send_buffer=NULL;

		if(clone)
		{
			send_buffer=buffer->copy();
		}
		else 
		{
			send_buffer=buffer;
		}

		if(m_writeQueue.empty())
		{
			ev_io_start(m_evLoop,&m_evWrite);
		}

		m_writeQueue.push_back(send_buffer);
		return 0;
	}
	return -1;
}





int XtStreamClient::send(const std::string& data)
{
	return this->send((uint8_t*) data.c_str(),data.length());
}


///对已经连接的fd，设置读、写事件 
int XtStreamClient::connectStart(int client_fd)
{
	m_clientFd=client_fd;

	if(m_clientFd<=0)
	{
		XT_LOG_ERROR("error fd(%d) to start connect.\n",client_fd);
		return -1;
	}

	m_isClose=0;
	m_evRead.data=this;
	ev_io_init(&m_evRead,XtStreamClient_onReciveData,m_clientFd,EV_READ);
	ev_io_start(m_evLoop,&m_evRead);


	m_evWrite.data=this;
	ev_io_init(&m_evWrite,XtStreamClient_onWriteData,m_clientFd,EV_WRITE);
	return 0;
}


int XtStreamClient::closeConnect()
{
	m_isClose=1;

	ev_io_stop(m_evLoop,&m_evRead);
	ev_io_stop(m_evLoop,&m_evWrite);

	if(m_clientFd>0)
	{
		close(m_clientFd);
	}

	clearWriteBuffer();


	onClose();
	return 0;
}



int XtStreamClient::writeData()
{
	if(m_isClose)
	{
		return -1;
	}

	if(m_writeQueue.empty())
	{
		ev_io_stop(m_evLoop,&m_evWrite);
		return 0;
	}

	XtBuffer* buffer=m_writeQueue.front();

	ssize_t write_byte=write(m_clientFd,buffer->dpos(),buffer->nbytes());
	if(write_byte<=0)
	{
		if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
		{
			XT_LOG_WARN("XtStreamClient:write failed[%s]\n", strerror(errno));
			return 0;
		}

		XT_LOG_ERROR("XtStreamClient:error(%s) in written(%d)\n",strerror(errno),m_clientFd);
		closeConnect();
		return -1;
	}

	buffer->forward(write_byte);

	if(buffer->nbytes()==0)
	{
		m_writeQueue.pop_front();
		delete buffer;
	}

	if(m_writeQueue.empty())
	{
		ev_io_stop(m_evLoop,&m_evWrite);
	}

	return 1;
}


int XtStreamClient::readData()
{
	if(m_isClose)
	{
		return -1;
	}


	int ret;
	uint8_t recv_buf[XT_STREAM_READ_BUF_LEN];

	ret=read(m_clientFd,recv_buf,XT_STREAM_READ_BUF_LEN);

	if(ret<0)
	{
		if(errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
		{

			XT_LOG_WARN("readData failed(%s)\n",strerror(errno));
			return 0;
		}
		XT_LOG_ERROR("read header failed[%s].\n",strerror(errno));

		closeConnect();
		return -1;
	}

	if(ret==0)
	{
		XT_LOG_WARN("XtStreamClient:connection close in read header[%d].\n",m_clientFd);
		closeConnect();
		return 0;
	}
	onReciveData(recv_buf,ret);
	return 0;
}




void XtStreamClient::clearWriteBuffer()
{
	while(!m_writeQueue.empty())
	{
		delete m_writeQueue.front();
		m_writeQueue.pop_front();
	}
}

