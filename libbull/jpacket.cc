#include "jpacket.h"

//#define __TEA_ENCRYPT__  //tea¼ÓÃÜ±àÒë¿ª¹Ø

char encrypt_key[128] = {"a@G$W!@LFKmfyZ5tn$1h^oduBSZTlPZQxtI97hEe@X6ekSRV#$ffGUgjWhU3EF"};

#define XXTEA_MX (z >> 5 ^ y << 2) + ((y >> 3 ^ z << 4) ^ (sum ^ y)) + (k[(p & 3) ^ e] ^ z)


void xxtea_long_encrypt(unsigned int *v, unsigned int len, unsigned int *k);
void xxtea_long_decrypt(unsigned int *v, unsigned int len, unsigned int *k);


void xxtea_long_encrypt(unsigned int *v, unsigned int len, unsigned int *k) 
{
	const unsigned int XXTEA_DELTA = 0x9e3779b8;

	unsigned int n = len - 1;
	unsigned int z = v[n], y = v[0], p, q = 6 + 52 / (n + 1), sum = 0, e;
	if (n < 1) 
	{
		return;
	}
	while (0 < q--) 
	{
		sum += XXTEA_DELTA;
		e = sum >> 2 & 3;
		for (p = 0; p < n; p++) 
		{
			y = v[p + 1];
			z = v[p] += XXTEA_MX;
		}
		y = v[0];
		z = v[n] += XXTEA_MX;
	}
}

void xxtea_long_decrypt(unsigned int *v, unsigned int len, unsigned int *k) 
{
	const unsigned int XXTEA_DELTA = 0x9e3779b8;

	unsigned int n = len - 1;
	unsigned int z = v[n], y = v[0], p, q = 6 + 52 / (n + 1), sum = q * XXTEA_DELTA, e;
	if (n < 1) 
	{
		return;
	}
	while (sum != 0) 
	{
		e = sum >> 2 & 3;
		for (p = n; p > 0; p--) 
		{
			z = v[p - 1];
			y = v[p] -= XXTEA_MX;
		}
		z = v[n];
		y = v[0] -= XXTEA_MX;
		sum -= XXTEA_DELTA;
	}
}

void xxtea_encrypt(char *v, unsigned int len, unsigned int *k) 
{
	if (len < 4)
	{
		return;
	}
	xxtea_long_encrypt((unsigned int *)v, len / 4, k);
}

void xxtea_decrypt(char *v, unsigned int len, unsigned int *k) 
{
	if (len < 4)
	{
		return;
	}
	xxtea_long_decrypt((unsigned int *)v, len / 4, k);
}

void xorfunc(std::string &nString)
{
	const int KEY = 13;
	int strLen = (nString.length());
	char *cString = (char*)(nString.c_str());

	for (int i = 0; i < strLen; i++)
	{
		*(cString + i) = (*(cString + i) ^ KEY);
	}
}

Jpacket::Jpacket()
{
}

Jpacket::~Jpacket()
{
}

std::string& Jpacket::tostring()
{
	return str;
}

Json::Value& Jpacket::tojson()
{
	return val;
}

void Jpacket::end()
{
	std::string out = val.toStyledString().c_str();
	int len = out.length();

	//xt_log.info("game packet:%s\n",out.c_str());
#ifdef __TEA_ENCRYPT__
	xxtea_encrypt((char*)out.c_str(), out.length(),(unsigned int *)encrypt_key);
#else
    xorfunc(out);
#endif

	header.length = len;

	str.clear();
	str.append((const char *)&header, sizeof(struct Header));
	str.append(out.data(), len);
}

int Jpacket::parse(std::string& data, int len)
{
#ifdef __TEA_ENCRYPT__
	xxtea_decrypt((char*)data.c_str(), len, (unsigned int *)encrypt_key);	
#else
	xorfunc(data);
#endif
	
	if (reader.parse(data, val) < 0)
	{
		return -1;
	}
	str=data;

	return 0;
}
