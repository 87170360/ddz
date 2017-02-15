#include <stdio.h>

#include "XtProtocol.h"
#include "../XtBuffer.h"



XtProtocol::XtProtocol()
{
	m_buffer=new XtBuffer;
}

XtProtocol::~XtProtocol()
{
	delete m_buffer;
	m_buffer=NULL;
}


int XtProtocol::append(uint8_t* data,int len)
{
	m_buffer->append(data,len);

	int ret=parseData();
	return ret;

}



