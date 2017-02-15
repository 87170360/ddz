#include <stdio.h>
#include <string.h>


#include "XtBuffer.h"



#define XT_BUFFER_DEFAULT_SIZE 32




int XtBuffer_NextPowerOf2(int n)
{
	if(n<=XT_BUFFER_DEFAULT_SIZE)
	{
		return XT_BUFFER_DEFAULT_SIZE;
	}

	int p=XT_BUFFER_DEFAULT_SIZE;


    if (n && !(n & (n - 1)))
        return n;

    while (p < n) {
        p <<= 1;
    }
    return p;


}



XtBuffer::XtBuffer()
{
	m_buf=NULL;
	m_bufCap=0;
	m_bufSize=0;
	m_beginPos=0;
}


XtBuffer::XtBuffer(const uint8_t* data,int len)
{
	int buf_cap=XtBuffer_NextPowerOf2(len);

	m_buf=new uint8_t[buf_cap];
	memcpy(m_buf,data,len);

	m_bufCap=buf_cap;
	m_bufSize=len;

	m_beginPos=0;

}

XtBuffer::~XtBuffer()
{
	if(m_buf)
	{
		delete[] m_buf;
		m_buf=NULL;
	}

	m_bufCap=0;
	m_bufSize=0;
	m_beginPos=0;
}

void XtBuffer::forward(int size)
{
	if(size>m_bufSize)
	{
		size=m_bufSize;
	}

	m_beginPos+=size;
	m_bufSize-=size;
}


void XtBuffer::append(const uint8_t* data,int len)
{
	int buf_end_free=m_bufCap-(m_bufSize+m_beginPos);

	if(buf_end_free<len)
	{
		enlarge(len);
	}
	memcpy(m_buf+m_beginPos+m_bufSize,data,len);

	m_bufSize+=len;
}

int XtBuffer::read(void* data,int len)
{
	if(nbytes()<len)
	{
		return -1;
	}

	memcpy(data,m_buf+m_beginPos,len);
	forward(len);
	return 0;
}


const uint8_t* XtBuffer::dpos()
{
	return m_buf+m_beginPos;
}

int XtBuffer::nbytes()
{
	return m_bufSize;
}


void XtBuffer::enlarge(int len)
{
	int free_size=(m_bufCap-m_bufSize);
	if(free_size<len)
	{
		int next_size=XtBuffer_NextPowerOf2(m_bufSize+len);

		uint8_t* new_buf= new uint8_t[next_size];

		memcpy(new_buf,m_buf+m_beginPos,m_bufSize);

		delete[] m_buf;
		m_buf=new_buf;

		m_beginPos=0;
		m_bufCap=next_size;
	}
	else 
	{
		for(int i=0;i<m_bufSize;i++)
		{
			m_buf[i]=m_buf[i+m_beginPos];
		}
		m_beginPos=0;
	}
}



XtBuffer* XtBuffer::copy()
{
	return new XtBuffer(dpos(),nbytes());
}

