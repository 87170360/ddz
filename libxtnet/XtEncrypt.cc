#include <string.h>

#include "XtEncrypt.h"



void XtEncrypt::xorfunc(uint8_t* data,int len,uint8_t key)
{
	for(int i=0;i<len;i++)
	{
		*(data+i)=(*(data+i)+key);
	}

}

void XtEncrypt::xorfunc(uint8_t* data,int len ,const char* key)
{
	int key_len=strlen(key);
	xorfunc(data,len,key,key_len);

}


void XtEncrypt::xorfunc(uint8_t* data,int len,const char* key,int key_len)
{

	for(int i=0;i<len;i++)
	{
		data[i]=data[i]^key[i%key_len];
	}
}



