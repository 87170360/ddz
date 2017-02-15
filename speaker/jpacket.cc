#include "jpacket.h"
#include "proto.h"
#include "log.h"

extern Log xt_log;

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
    string out = val.toStyledString().c_str();
    xt_log.debug("sendDataStyled: [%s]\n", out.c_str());
    xorfunc(out);
    header.length = out.length();
    
    str.clear();
    str.append((const char *)&header, sizeof(struct Header));
    str.append(out);
}

int Jpacket::parse(std::string &str)
{
    xorfunc(str);
    xt_log.debug("recvData: [%s]\n", str.c_str());
    if (reader.parse(str, val) < 0)
    {
        return -1;
    }
    
    xt_log.debug("recvDataStyled: [%s]\n", val.toStyledString().c_str());
    return 0;
}

int Jpacket::noxor_parse(std::string &str)
{
    xt_log.debug("recvData: [%s]\n", str.c_str());
    if (reader.parse(str, val) < 0)
    {
        return -1;
    }

    xt_log.debug("recvDataStyled: [%s]\n", val.toStyledString().c_str());
    return 0;
}

int Jpacket::sefe_check()
{
    if (!val["cmd"].isNumeric()) return -2;

    int cmd = val["cmd"].asInt();
    
    /*
    switch (cmd)
    {
        case CLIENT_LOGIN_REQUEST:
        {
            if (val["tid"].isNumeric()
                && val["uid"].isNumeric()
                && val["skey"].isString())
                break;
            else
            {
                //xt_log.error("command[login] error\n");
                return -1;
            }
        }
    }
    */

    return cmd;
}

