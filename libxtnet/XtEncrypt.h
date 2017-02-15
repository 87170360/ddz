#ifndef _XT_ENCRYPT_H_
#define _XT_ENCRYPT_H_

#include <stdint.h>

///--º”√‹
class XtEncrypt 
{
	public:
		static void xorfunc(uint8_t* data,int len,uint8_t key);
		static void xorfunc(uint8_t* data,int len,const char* key);
		static void xorfunc(uint8_t* data,int len,const char* key,int key_len);
};


#endif /*_XT_ENCRYPT_H_*/


