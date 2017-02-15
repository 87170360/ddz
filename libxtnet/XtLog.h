#ifndef _XT_LOG_H_
#define _XT_LOG_H_ 


#include <stdint.h>
#include <string>

///---ÈÕÖ¾Àà
class XtLog 
{
	public:
		static XtLog* getDefaultLog();
		static void deleteDefaultLog();

	public:


	public:
		XtLog();
		virtual ~XtLog();

	public:
		int start(const std::string& log_file, int level, int console, int rotate, int64_t max_size, int max_file);
		void stop();
		void panic(const char* fmt, ...);
		void fatal(const char* fmt, ...);
		void error(const char* fmt, ...);
		void warn(const char* fmt, ...);
		void info(const char* fmt, ...);
		void debug(const char* fmt, ...);

	private:
		void output(const char* buf, int cnt);
		void rotate();	
		bool checkDayChanged();
		void rotateDay();

	private:
		std::string			 m_logFile;
		int					m_level;
		int					m_fileFd;
		int					m_console;
		//int					m_rotate;
		int64_t				m_maxSize;
		int64_t				m_currentSize;
		int					m_maxFile;
		int m_year;
		int m_month;
		int m_day;

#ifdef XT_POSIX_PTHREAD
		pthread_mutex_t		m_mutex;
#endif

};



#define XT_LOG_PANIC(fmt,...) \
	do { \
		XtLog::getDefaultLog()->panic("%s:%d:"fmt,__FILE__,__LINE__,##__VA_ARGS__); \
	}while(0);


#define XT_LOG_ERROR(fmt,...) \
	do { \
		XtLog::getDefaultLog()->error("%s:%d:"fmt,__FILE__,__LINE__,##__VA_ARGS__); \
	}while(0);

#define XT_LOG_FATAL(fmt,...) \
	do { \
		XtLog::getDefaultLog()->fatal("%s:%d:"fmt,__FILE__,__LINE__,##__VA_ARGS__); \
	}while(0);

#define XT_LOG_WARN(fmt,...) \
	do { \
		XtLog::getDefaultLog()->warn("%s:%d:"fmt,__FILE__,__LINE__,##__VA_ARGS__); \
	}while(0);


#define XT_LOG_INFO(fmt,...) \
	do { \
		XtLog::getDefaultLog()->info("%s:%d:"fmt,__FILE__,__LINE__,##__VA_ARGS__); \
	}while(0);



#define XT_LOG_DEBUG(fmt,...) \
	do { \
		XtLog::getDefaultLog()->debug("%s:%d:"fmt,__FILE__,__LINE__,##__VA_ARGS__); \
	}while(0);


#endif 



