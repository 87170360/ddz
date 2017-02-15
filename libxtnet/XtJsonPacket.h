#ifndef _XT_JSON_PACKET_H_
#define _XT_JSON_PACKET_H_

#include <string>
#include <json/json.h>
///---JSON格式组包及解包
class XtJsonPacket
{
	public:
		XtJsonPacket();
		virtual ~XtJsonPacket();


	public:
		int parse(const char* str);
		int parse(const char* str,int length);
		void pack();

	public:
		std::string m_str;
		Json::Value m_val;
};



#endif /*_XT_JSON_PACKET_H_*/



