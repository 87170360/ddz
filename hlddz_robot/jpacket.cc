#include "jpacket.h"
#include <stdio.h>
#include <stdlib.h>

void xorfunc(std::string &nString)
{
	const int KEY = 13;
	int strLen = (nString.length());
	char *cString = (char*)(nString.c_str());
	
	for (int i = 0; i < strLen; i++)
	{
		*(cString + i) = (*(cString + i) ^ KEY);
	}
}

Jpacket::Jpacket()
{
}

Jpacket::~Jpacket()
{
}

std::string& Jpacket::tostring()
{
    return str;
}

Json::Value& Jpacket::tojson()
{
    return val;
}

void Jpacket::end()
{
    //header[0] = 0xFA;
    //header[1] = 0xF1;
	std::string out = val.toStyledString().c_str();
	//printf("sendDataStyled: [%s]\n", out.c_str());
	xorfunc(out);
    header.m_length = out.length();
	
    str.clear();
    str.append((const char *)&header, sizeof(struct Header));
    str.append(out);
}

int Jpacket::parse(std::string &str)
{
	xorfunc(str);
	//printf("recvData: [%s]\n", str.c_str());
    if (reader.parse(str, val) < 0)
    {
        return -1;
    }
	
	//printf("recvDataStyled: [%s]\n", val.toStyledString().c_str());
    return 0;
}


