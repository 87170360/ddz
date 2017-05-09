#ifndef _XT_BUFFER_H_
#define _XT_BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


struct XtBuffer 
{
    char* m_data;
    ssize_t m_len;

    XtBuffer(const char *bytes, ssize_t nbytes)
	{
        m_len = nbytes;
        m_data = new char[nbytes];
        memcpy(m_data, bytes, nbytes);
    }

    virtual ~XtBuffer()
	{
        delete [] m_data;
    }
};

#endif /*_XT_BUFFER_H_*/



