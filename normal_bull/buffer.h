#ifndef _BUFFER_H_
#define _BUFFER_H_



#ifndef MAX_BUF_LEN
#   define MAX_BUF_LEN (1024*8)
#endif

#ifndef DEF_BUF_LEN
#   define DEF_BUF_LEN (1024*8)
#endif


struct Buffer 
{
    char	*data;
    ssize_t len;
    ssize_t pos;

    Buffer(const char *bytes, ssize_t nbytes)
	{
        pos = 0;
        len = nbytes;
        data = new char[nbytes];
        memcpy(data, bytes, nbytes);
    }

    virtual ~Buffer()
	{
        delete [] data;
    }
    
    char *dpos()
	{
        return data + pos;
    }

    ssize_t nbytes()
	{
        return len - pos;
    }
};


#endif /*_BUFFER_H_*/


