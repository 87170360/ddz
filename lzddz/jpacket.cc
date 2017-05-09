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
    printf("%s", DESC_STATE[0]);
    printf("%s", DESC_OP[0]);
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

	string out = val.toStyledString().c_str();

	//xt_log.info("game packet:%s\n",out.c_str());

	xorfunc(out);
	header.length = out.length();

	str.clear();
	str.append((const char *)&header, sizeof(struct Header));
	str.append(out);
}

int Jpacket::parse(std::string& data)
{
	xorfunc(data);
    //xt_log.debug("parse data %s\n", data.c_str());

	if (reader.parse(data, val) < 0)
	{
		return -1;
	}
	str=data;

	return 0;
}

int Jpacket::safe_check()
{
    if(!val.isMember("cmd"))
    {
        xt_log.error("not cmd key \n");
        return -3;
    }
	if (!val["cmd"].isNumeric())
    {
        xt_log.error("cmd not member \n");
        return -2;
    }

	int cmd = val["cmd"].asInt();

	switch (cmd)
	{
		case CLIENT_LOGIN:
			{
				if (!val["uid"].isNumeric() || !val["skey"].isString())
				{
					xt_log.error("command client_login error\n");
					return -1;
				}
			}
            break;
		case CLIENT_CALL:
			{
				if (!val["act"].isBool())
				{
					xt_log.error("command client_call error\n");
					return -1;
				}
			}
            break;
		case CLIENT_GRAB:
			{
				if (!val["act"].isBool())
				{
					xt_log.error("command client_call error\n");
					return -1;
				}
			}
            break;
		case CLIENT_DOUBLE:
			{
				if (!val["double"].isBool())
				{
					xt_log.error("command client_double error\n");
					return -1;
				}
			}
            break;
		case CLIENT_OUT:
			{
				if (!val["keep"].isBool())
				{
					xt_log.error("command client_out error\n");
					return -1;
				}
			}
            break;
		case CLIENT_VIEW:
			{
				if (!val["uid"].isNumeric())
				{
					xt_log.error("command client_view error\n");
					return -1;
				}
			}
            break;
		case CLIENT_ENTRUST:
			{
				if (!val["active"].isBool())
				{
					xt_log.error("command client_entrust error\n");
					return -1;
				}
			}
            break;
		case CLIENT_CHAT:
			{
				if (!val["content"].isString() && !val["chatid"].isNumeric())
				{
					xt_log.error("command client_chat error\n");
					return -1;
				}
			}
            break;
		case CLIENT_MOTION:
			{
				if (!val["type"].isNumeric() && !val["target_id"].isNumeric())
				{
					xt_log.error("command client_motion error\n");
					return -1;
				}
			}
            break;
	}

	return cmd;
}

