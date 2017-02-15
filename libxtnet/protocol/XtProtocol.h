#ifndef _XT_PROTOCOL_H_
#define _XT_PROTOCOL_H_

#include <stdint.h>


class XtBuffer;
///---Э��ӿ���(��������)
class XtProtocol 
{
	public:
		XtProtocol();
		virtual ~XtProtocol();


	public:
		int append(uint8_t* data,int len);


	public:
		virtual int parseData()=0;

		virtual XtBuffer* unpack()=0;
		virtual XtBuffer* pack(const uint8_t* data,int len)=0;


	protected:
		XtBuffer* m_buffer;
};


#endif /*_XT_PROTOCOL_H_*/


