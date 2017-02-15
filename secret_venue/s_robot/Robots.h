#ifndef _XT_ROBOTS_H_
#define _XT_ROBOTS_H_

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>

#include <ev.h>
#include <json/json.h>
#include "XtRobotClient.h"
///---管理所有机器人
class Robots 
{
	public:
		int parseArg(int argc,char** argv);
	
	public:
		void mainLoop();

	private:
		std::string m_configFileName;

		Json::Value m_conf;

		std::vector<XtRobotClient*> m_robotClient;
		struct ev_loop* m_evloop;
};


#endif /*_XT_ROBOTS_H_*/



