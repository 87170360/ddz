#ifndef _ALL_KILL_CLIENT_H_
#define _ALL_KILL_CLIENT_H_

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

#include <ev.h>
#include <json/json.h>

#include "jpacket.h"
#include "buffer.h"



class AllKillClient
{
#ifdef AK_DEBUG 
	public:
		static int ms_objectNu;
#endif 

	public:
		typedef void (*ReciveCmdFunc)(AllKillClient* client,void* data,Jpacket& packet);
		typedef void (*CloseFunc)(AllKillClient* client,void* data);


	public:
		static void onReciveData(struct ev_loop* loop,struct ev_io* w,int revents );
		static void onWriteData(struct ev_loop* loop,struct ev_io* w,int events);

	public:
		AllKillClient(struct ev_loop* loop);
		~AllKillClient();

	public:
		void setUserData(void* data) { m_userData=data; }
		void* getUserData() { return m_userData; }


		void setUid(int uid){m_uid=uid;}
		int getUid(){return m_uid;}

		int getClientFd(){return m_clientFd;}


	public:
		void setOnReciveCmdFunc(ReciveCmdFunc func,void* data)
		{
			m_onReciveCmdFunc=func;
			m_reciveCmdFuncData=data;
		}
		void setOnCloseFunc(CloseFunc func,void* data)
		{
			m_onCloseFunc=func;
			m_closeFuncData=data;
		}

	public:
		int send(const char *buf, unsigned int len);
		int send(const std::string& res);

		void readData();
		void writeData();


		int connectStart(int client_fd);
		int closeConnect();

	private:
		/* data buffer */
		std::list<Buffer*> m_writeQueue;
		char m_headerBuf[sizeof(struct Header)];
		struct Header* m_header;
		unsigned int m_curHeaderLen;
		std::string m_body;
		Jpacket m_packet;
		int m_state;

		/* client fd */
		int m_clientFd;
		int m_isClose;


		/* user info */
		int m_uid;


		/* user_data */
		void* m_userData;


		/* recive cmd */
		ReciveCmdFunc m_onReciveCmdFunc;
		void* m_reciveCmdFuncData;

		/* closeFunc */
		CloseFunc m_onCloseFunc;
		void* m_closeFuncData;


		/* ev */
		ev_io m_evWrite;
		ev_io m_evRead;
		struct ev_loop* m_evLoop;
};

#endif /*_ALL_KILL_CLIENT_H_*/


