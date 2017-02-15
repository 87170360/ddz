#include "AllKillClient.h"

#ifndef DEF_BUF_LEN 
	#define  DEF_BUF_LEN (1024*4)
#endif 


#ifndef MAX_BUF_LEN 
	#define MAX_BUF_LEN (1024*32)
#endif 


#include "log.h"
extern Log xt_log;

#ifdef AK_DEBUG 
int AllKillClient::ms_objectNu=0;
#endif 





//收到数据
void AllKillClient::onReciveData(struct ev_loop* loop,struct ev_io* w,int events)
{
	AllKillClient* c = (AllKillClient*)w->data;
	c->readData();
}

void AllKillClient::onWriteData(struct ev_loop* loop,struct ev_io* w,int events)
{
	AllKillClient* c = (AllKillClient*)w->data;
	c->writeData();
}




AllKillClient::AllKillClient(struct ev_loop* loop)
{
	m_header = (struct Header*)(m_headerBuf);  //
	m_curHeaderLen = 0;
	m_state = PARSE_HEADER;	// 包头

	m_clientFd = -1;
	m_isClose = 1;       // 关闭标记


	m_uid = -1;
	m_userData = NULL;


	m_onReciveCmdFunc = 0;
	m_reciveCmdFuncData = 0;

	m_onCloseFunc = 0;
	m_closeFuncData = 0;

	m_evLoop = loop;   //事件循环对象

#ifdef AK_DEBUG 
	ms_objectNu++;
#endif 

}

AllKillClient::~AllKillClient()
{
	ev_io_stop(m_evLoop,&m_evRead);
	ev_io_stop(m_evLoop,&m_evWrite);
	while(!m_writeQueue.empty())
	{
		delete m_writeQueue.front();
		m_writeQueue.pop_front();
	}

	if(m_clientFd>0&&m_isClose==0)
	{

		close(m_clientFd);
		xt_log.info("~client fd[%d] uid[%d] destrutor\n", m_clientFd, m_uid);
	}

#ifdef AK_DEBUG 
	ms_objectNu--;
#endif 


}



void AllKillClient::writeData()
{
	if (m_writeQueue.empty()) 
	{
		ev_io_stop(m_evLoop,&m_evWrite);
		return;
	}


	Buffer* buffer = m_writeQueue.front();

	ssize_t written = write(m_clientFd, buffer->dpos(), buffer->nbytes());

	if (written < 0) 
	{
		if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
		{
			xt_log.warn("write failed[%s]\n", strerror(errno));
			return;
		}


		/* todo close this client */
		xt_log.error("unknow err in written [%d]\n",m_clientFd);
		closeConnect();

		return;
	}



	buffer->pos += written;


	if (buffer->nbytes() == 0) 
	{
		m_writeQueue.pop_front();
		delete buffer;
	}
}


int AllKillClient::send(const char *buf, unsigned int len)
{
	if (0 == m_isClose)
	{
		if (m_writeQueue.empty()) 
		{
			m_evWrite.data = this;
			ev_io_start(m_evLoop, &m_evWrite);
		}

		m_writeQueue.push_back(new Buffer(buf, len));
		return 0;
	}
	else 
	{
		xt_log.error("send error,client(%d) is close\n",m_uid);
	}


	return -1;
}


int AllKillClient::send(const std::string& res)
{
	return send(res.c_str(), res.length());
}



void AllKillClient::readData()
{
	int ret;
	char recv_buf[DEF_BUF_LEN];

	//printf("onReciveData\n");
	// 解析包头
	if (m_state == PARSE_HEADER) 
	{
		ret = read(m_clientFd, m_headerBuf+m_curHeaderLen, sizeof(struct Header) - m_curHeaderLen);

		if (ret < 0) 
		{
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
			{
				xt_log.debug("read header failed[%s]\n", strerror(errno));
				return;
			}

			xt_log.error("read header failed[%s].\n", strerror(errno));

			closeConnect();
			return;
		}


		if (ret == 0) 
		{
			xt_log.error("connection close in read header[%d].\n", m_clientFd);
			closeConnect();
			return;
		}


		m_curHeaderLen+= ret;

		if (m_curHeaderLen== sizeof(struct Header)) 
		{
			if (m_header->length > MAX_BUF_LEN || m_header->length == 0) 
			{
				closeConnect();
				return;
			}


			m_state= PARSE_BODY;
			m_curHeaderLen= 0;
			m_body.clear();
		}


	}
	else if (m_state == PARSE_BODY) 
	{	// 解析包体
		ret = read(m_clientFd, recv_buf, m_header->length - m_body.length());
		if (ret < 0) 
		{
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
			{
				xt_log.debug("read body failed[%s].\n", strerror(errno));
				return;
			}

			xt_log.debug("read body failed[%s].\n", strerror(errno));
			closeConnect();
			return;
		}


		if (ret == 0) 
		{
			xt_log.error("connection close in read body[%d].\n", m_clientFd);
			closeConnect();
			return;
		}


		recv_buf[ret] = '\0';
		m_body.append(recv_buf, ret);

		if (/*m_body.length()*/ret == m_header->length) 
		{
			m_state = PARSE_HEADER;
			if (m_packet.parse(m_body, ret) < 0) 
			{
				xt_log.error("body parse error.\n");
				closeConnect();
				return;
			}

			// 委派回调函数处理AllKillServer::onReciveClientCmd
			if(m_onReciveCmdFunc)
			{
				m_onReciveCmdFunc(this,m_reciveCmdFuncData,m_packet);
			}
		}
	}
}

//连接启动
int AllKillClient::connectStart(int client_fd)
{
	m_clientFd = client_fd;   //会话SOCKET ，并设置会话SOCKET读写事件
	if (m_clientFd <= 0)
	{
		xt_log.error("client error(%d)\n",client_fd);
		return -1;
	}


	m_isClose = 0;

	m_evRead.data = this;

	ev_io_init(&m_evRead,AllKillClient::onReciveData,m_clientFd,EV_READ);
	ev_io_start(m_evLoop,&m_evRead);

	m_evWrite.data = this;
	ev_io_init(&m_evWrite,AllKillClient::onWriteData,m_clientFd,EV_WRITE);

	return 0;
}



int AllKillClient::closeConnect()
{
	ev_io_stop(m_evLoop,&m_evRead);
	ev_io_stop(m_evLoop,&m_evWrite);
	close(m_clientFd);


	while(!m_writeQueue.empty())
	{
		delete m_writeQueue.front();
		m_writeQueue.pop_front();
	}

	m_isClose=1;

	// 委派回调函数处理AllKillServer::onClientClose
	if (m_onCloseFunc)
	{
		m_onCloseFunc(this,m_closeFuncData);
	}

	return 0;
}


















