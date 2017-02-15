#ifndef _XT_ENUMS_H_
#define _XT_ENUMS_H_

///---日志级别
class E_XtLogLevel
{
	public:
		enum 
		{
			LEVEL_PANIC = 0,
			LEVEL_FATAL,
			LEVEL_ERROR,
			LEVEL_WARN,
			LEVEL_INFO,
			LEVEL_DEBUG,
		};
};
///---控制台类型
class E_ConsoleState 
{
	public:
		enum 
		{
			CONSOLE_OFF=0,
			CONSOLE_ON=1
		};
};

#endif /*_XT_ENUMS_H_*/


