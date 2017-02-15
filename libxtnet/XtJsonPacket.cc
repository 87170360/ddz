#include <stdio.h>
#include "XtJsonPacket.h"


XtJsonPacket::XtJsonPacket()
{

}

XtJsonPacket::~XtJsonPacket()
{


}

int XtJsonPacket::parse(const char* str)
{
	Json::Reader reader;
	int ret=reader.parse(str,m_val);
	if(ret<0)
	{
		return -1;
	}
	m_str=str;
	return 0;
}


int XtJsonPacket::parse(const char* str,int len)
{
	Json::Reader reader;
	int ret=reader.parse(str,str+len,m_val);
	if(ret<0)
	{
		return -1;
	}

	//printf("%s\n",str);

	m_str.resize(0);
	m_str.insert(0,(char*)str,len);
	return 0;
}



void XtJsonPacket::pack()
{
	m_str=m_val.toStyledString();
}

