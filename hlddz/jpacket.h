#ifndef __JPACKET_H__
#define __JPACKET_H__

#include <json/json.h>

enum parse_state
{
    PARSE_HEADER    	= 0,
    PARSE_BODY      	= 1,
    PARSE_ERR       	= 2,
    PARSE_CROSS_DOMAIN	= 3,
};


struct Header {
    //unsigned char   sig[2];
	unsigned int    length;
	//unsigned int    info;
};

class Jpacket
{
public:
    Jpacket();
    virtual ~Jpacket();

    std::string& tostring();
    Json::Value& tojson();

    void end();
    int parse(std::string&);
    int sefe_check();

private:
    struct Header           header;

public:
    std::string             str;
    Json::Value             val;
    Json::Reader            reader;
    int                     len;
};

#endif
