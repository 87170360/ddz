#include <stdio.h>

#include "XtMsgProtocol.h"
#include "XtBuffer.h"
#include "XtEncrypt.h"


#define XT_MSG_HEADER_SIZE sizeof(XtMsgProtocol::MsgHeader)


XtMsgProtocol::XtMsgProtocol()
{
	m_maxMsgLength=XT_DEAULT_MSG_LENGTH;
	m_xorEnabled=false;
	m_xorKey="";

}


XtMsgProtocol::~XtMsgProtocol()
{
	while(!m_msgs.empty())
	{
		XtBuffer* buffer=m_msgs.front();
		delete buffer;
		m_msgs.pop_front();
	}
}




int XtMsgProtocol::parseData()
{
	while(true)
	{
		int size=m_buffer->nbytes();

		//printf("parseData %d\n",size);

		if(size>=(int)XT_MSG_HEADER_SIZE)
		{
			MsgHeader* header=(MsgHeader*) m_buffer->dpos();

		//	printf("header size=%d\n",(int)XT_MSG_HEADER_SIZE);

#ifdef XT_SUPPORT_MSG_HEADER_MAGIC_ID
			if(header->m_magicId!=XT_MSG_HEADER_MAGIC_ID)
			{
				return -1;
			}
#endif 
			uint32_t length=header->m_length;

		//	printf("header_ length=%u\n",length);

			if(length>m_maxMsgLength)
			{
				return -1;
			}

			if(size>=(int)(XT_MSG_HEADER_SIZE+length))
			{
				XtBuffer* data=new XtBuffer(m_buffer->dpos()+XT_MSG_HEADER_SIZE,length);

				if(m_xorEnabled||m_xorKey.length()!=0)
				{
					XtEncrypt::xorfunc((uint8_t*)data->dpos(),data->nbytes(),m_xorKey.c_str(),m_xorKey.length());
				}

				//printf("data:%s\n",data->dpos());

				m_msgs.push_back(data);
				m_buffer->forward(XT_MSG_HEADER_SIZE+length);
				continue;
			}
		}
		break;
	}
	return 0;
}

///协议组包
XtBuffer* XtMsgProtocol::pack(const uint8_t* data,int len)
{
	XtBuffer* buffer=new XtBuffer();
	MsgHeader header;

	header.m_length=len;

#ifdef XT_SUPPORT_MSG_HEADER_MAGIC_ID 
	header.m_magicId=XT_MSG_HEADER_MAGIC_ID;
#endif 



	buffer->append((uint8_t*)&header,XT_MSG_HEADER_SIZE);    ///包长
	buffer->append(data,len);                                ///包体

	if(m_xorEnabled||m_xorKey.length()!=0)
	{
		XtEncrypt::xorfunc((uint8_t*)buffer->dpos()+XT_MSG_HEADER_SIZE,buffer->nbytes()-XT_MSG_HEADER_SIZE,m_xorKey.c_str(),m_xorKey.length());
	}

	return buffer;
}

///协议解包
XtBuffer* XtMsgProtocol::unpack()
{
	if(m_msgs.empty())
	{
		return NULL;
	}

	XtBuffer* ret=m_msgs.front();
	m_msgs.pop_front();
	return ret;
}


void XtMsgProtocol::setXorEnabled(int enable)
{
	m_xorEnabled=enable;
}

void XtMsgProtocol::setXorKey(const char* key)
{
	m_xorKey=key;
}




