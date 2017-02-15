#ifndef __JPACKET_H__
#define __JPACKET_H__

#include <json/json.h>

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
    int noxor_parse(std::string&);

private:
    struct Header           header;

public:
    std::string             str;
    Json::Value             val;
    Json::Reader            reader;
    int                     len;
};

#endif
