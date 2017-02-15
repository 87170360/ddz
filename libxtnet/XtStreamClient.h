#ifndef _XT_STREAM_CLIENT_H_
#define _XT_STREAM_CLIENT_H_
 
#include <string>
#include <list>
#include <ev.h>
#include <stdint.h>

class XtBuffer;


///数据流读写
class XtStreamClient 
{
	public:
		XtStreamClient(struct ev_loop* loop);
		~XtStreamClient();


	public:
		int send(XtBuffer* buffer,int clone);
		int send(const void* buffer,unsigned int len);
		int send(const std::string& data);



		int connectStart(int client_fd);
		int closeConnect();

		int getClientFd(){return m_clientFd;}


	public:
		virtual void onReciveData(void* data,int len)=0;    ///派生类去处理
		virtual void onClose()=0;

	public:
		int readData();
		int writeData();
		void clearWriteBuffer();


	protected:
		/* write buffer */
		std::list<XtBuffer*> m_writeQueue;


		/* client fd */
		int m_clientFd;
		bool m_isClose;


		/* ev info */
		ev_io m_evWrite;
		ev_io m_evRead;
		struct ev_loop* m_evLoop;
};

#endif /*_XT_STREAM_CLIENT_H_*/


