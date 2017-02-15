#ifndef _XT_MSG_PROTOCOL_H_
#define _XT_MSG_PROTOCOL_H_

#include <stdint.h>
#include <string>

#include <list>
#include "XtNetMacros.h"
#include "XtProtocol.h"


//#define XT_SUPPORT_MSG_HEADER_MAGIC_ID

#define XT_MSG_HEADER_MAGIC_ID 0x4a3b2c1d


#define XT_DEAULT_MSG_LENGTH (1024*32)


class XtBuffer;

class XtMsgProtocol :public XtProtocol 
{
	public:
		class  MsgHeader
		{
			public:

#ifdef XT_SUPPORT_MSG_HEADER_MAGIC_ID 
				uint32_t m_magicId;
#endif 

				uint32_t  m_length;
		};

	public:
		XtMsgProtocol();
		~XtMsgProtocol();

	public:
		void setMaxMsgLength(int length);

		void setXorEnabled(int enable);
		void setXorKey(const char* key);


	public:
		int parseData() XT_OVERRIDE;

		XtBuffer* pack(const uint8_t* data,int length) XT_OVERRIDE;
		XtBuffer* unpack() XT_OVERRIDE; 

	private:

		std::list<XtBuffer*> m_msgs;
		uint32_t m_maxMsgLength;

		int m_xorEnabled;
		std::string m_xorKey;

};


#endif /*_XT_MSG_PROTOCOL_H_*/




