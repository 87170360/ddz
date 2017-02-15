#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>


#include "XtLog.h"
#include "XtEnums.h"



static XtLog* ms_log=NULL;

XtLog* XtLog::getDefaultLog()
{
	if(ms_log==NULL)
	{
		ms_log=new XtLog;
	}
	return ms_log;
}

void XtLog::deleteDefaultLog()
{
	if(ms_log)
	{
		delete ms_log;
		ms_log=NULL;
	}
}




static const char* s_logTyepStr[]= { "panic", "fatal", "error", "warn", "info", "debug" };


#define XT_COMMON_LOG(log_type) \
	static char buf[1024000]; \
	time_t t;\
    struct tm gt;\
    struct tm* p;\
    int len;\
    int cnt;\
	time(&t);\
    p = localtime_r(&t, &gt);\
    if(!p)\
        p = &gt;\
	len = sprintf(buf, "[%s][%.4d-%.2d-%.2d %.2d:%.2d:%.2d] ",\
                s_logTyepStr[log_type],\
                p->tm_year + 1900,\
                p->tm_mon + 1,\
                p->tm_mday,\
                p->tm_hour,\
                p->tm_min,\
                p->tm_sec);\
	va_list argptr;\
	va_start(argptr, fmt);\
    cnt = vsprintf(buf + len, fmt, argptr);\
	va_end(argptr);\
    cnt += len;\



XtLog::XtLog()
{
	m_level=E_XtLogLevel::LEVEL_DEBUG;
	m_fileFd=-1;
	m_console=E_ConsoleState::CONSOLE_OFF;
	//m_rotate=ROTATE_OFF;
	m_maxSize=0;
	m_currentSize=0;
	m_maxFile=0;


	time_t timep;
	time(&timep);


	struct tm* current_day=localtime(&timep);

	m_year=current_day->tm_year+1900;
	m_month=current_day->tm_mon;
	m_day=current_day->tm_mday;

#ifdef XT_POSIX_PTHREAD 
	pthread_mutex_init(&m_mutex,NULL);
#endif 

}

XtLog::~XtLog()
{

#ifdef XT_POSIX_PTHREAD 
	pthread_mutex_destroy(&m_mutex);
#endif 

}




int XtLog::start(const std::string& log_file,
			int level, 
			int console,
			int rotate, 
			int64_t max_size,
			int max_file)
{
	struct stat st;

	m_logFile=log_file;
	m_fileFd=open(log_file.c_str(),O_RDWR|O_APPEND|O_CREAT,0644);

	if(m_fileFd<0)
	{
		fprintf(stderr,"XtLog:Can't Open Log File(%s)",m_logFile.c_str());
		return -1;
	}

	if(fstat(m_fileFd,&st)<0)
	{
		fprintf(stderr,"XtLog:fstat Log File(%s) Error",m_logFile.c_str());
		return -1;
	}

	m_level=level;
	m_console=console;
	//m_rotate=rotate;

	m_maxSize=max_size;
	m_currentSize=st.st_size;
	m_maxFile=max_file;

	return 0;

}


void XtLog::stop()
{
	if(m_fileFd!=-1)
	{
		close(m_fileFd);
		m_fileFd=-1;
	}
}

void XtLog::output(const char* buf,int length)
{
	if(m_console == E_ConsoleState::CONSOLE_ON)
	{
		printf("%s",buf);
		return;
	}

#ifdef XT_POSIX_PTHREAD 
	pthread_mutex_lock(&m_mutex);
#endif

	if(m_fileFd<0 || checkDayChanged())
	{
		rotateDay();
	}
	
	if(m_fileFd>0)
	{
		int ret=write(m_fileFd,buf,length);
		if(ret<0)
		{
			fprintf(stderr,"XtLog:Write To File Error");
		}
		m_currentSize+=length;
	}
	else 
	{
		printf("%s",buf);
	}
#ifdef XT_POSIX_PTHREAD 
	pthread_mutex_unlock(&m_mutex);
#endif 

}

void XtLog::rotateDay()
{
	char src[1024];
	char dst[1024];

	close(m_fileFd);
	m_fileFd=-1;

	m_currentSize=0;

	snprintf(src, sizeof(src), "%s",m_logFile.c_str());
	snprintf(dst, sizeof(dst), "%s.%04d-%02d-%02d", m_logFile.c_str(), m_year, m_month+1, m_day);
	rename(src, dst);

	m_fileFd= open(m_logFile.c_str(), O_RDWR | O_APPEND | O_CREAT, 0644);
	if (m_fileFd< 0) 
	{
		fprintf(stderr,"XtLog:open log file(%s) error\n",m_logFile.c_str());
		return;
	}

	time_t timep;
	time(&timep);

	struct tm* current_day=localtime(&timep);

	m_year = current_day->tm_year + 1900;
	m_month = current_day->tm_mon;
	m_day = current_day->tm_mday;
}



void XtLog::panic(const char *fmt, ...) 
{
	if (m_level < E_XtLogLevel::LEVEL_PANIC)
	{
		return;
	}

	XT_COMMON_LOG(m_level)
	output(buf, cnt);

	exit(1);
}

void XtLog::fatal(const char *fmt, ...) 
{
	if (m_level < E_XtLogLevel::LEVEL_FATAL)
	{
		return;
	}

	XT_COMMON_LOG(m_level)
	output(buf, cnt);

}

void XtLog::error(const char *fmt, ...) 
{
	if (m_level < E_XtLogLevel::LEVEL_ERROR)
	{
		return;
	}

	XT_COMMON_LOG(m_level)

	output(buf, cnt);
}

void XtLog::warn(const char *fmt, ...) 
{
	if (m_level < E_XtLogLevel::LEVEL_WARN)
	{
		return;
	}

	XT_COMMON_LOG(m_level)
	output(buf, cnt);
}

void XtLog::info(const char *fmt, ...) 
{
	if (m_level < E_XtLogLevel::LEVEL_INFO)
	{
		return;
	}

	XT_COMMON_LOG(m_level)
	output(buf, cnt);

}

void XtLog::debug(const char *fmt, ...) 
{
	if (m_level < E_XtLogLevel::LEVEL_DEBUG)
	{
		return;
	}

	XT_COMMON_LOG(m_level)
	output(buf, cnt);
}

bool XtLog::checkDayChanged()
{

	time_t timep;
	time(&timep);

	struct tm* current_day=localtime(&timep);

	if (m_year == (current_day->tm_year+1900)
			&& m_month == current_day->tm_mon
			&& m_day == current_day->tm_mday) 
	{
		return false;
	}

	return true;
}

