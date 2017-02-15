#include <iostream>
#include <string>

#include "wordfilter.h"

static const char *key_one[] = {"共","法","习","回","出","交","批","买","q","Q","群","低","非","卖","淘","买","支"};
static const char *key_two[] = {"产","轮","近","收","售","易","发","卖","群","群","号","价","官","家","宝","家","付"};
static const char *key_three[] = {"党","0","平","0","0","0","0","0","0","0","0","方","0","0","0","0","宝"};


// 关键字过滤
int keyword_filter(const std::string &in_str)
{
	size_t pos1;
	size_t pos2;
	size_t pos3;
	int len = sizeof(key_one) / sizeof(char *);

//	int len2 = sizeof(key_two) / sizeof(char *);
//	int len3 = sizeof(key_three) / sizeof(char *);
//	printf("len:%d len2:%d len3:%d.\n", len, len2, len3);
//	printf("in_str:%s.\n", in_str.c_str());

	for (int i = 0; i < len; i++) {
		pos1 = in_str.find(key_one[i]);
		pos2 = in_str.find(key_two[i]);
		if (pos1 != std::string::npos && pos2 != std::string::npos) {
			if (strcmp(key_three[i], "0") == 0) {
//				printf("key word %s %s %s.\n", key_one[i], key_two[i], key_three[i]);
				return -1;
			}
			pos3 = in_str.find(key_three[i]);
			if (pos3 != std::string::npos) {
//				printf("key word %s %s %s.\n", key_one[i], key_two[i], key_three[i]);
				return -1;
			}
		}
	}

	return 0;
}

//int main()
//{
//	std::string str = "12轮3a淘a回a付4宝 44";
//	int ret = keyword_filter(str);
//	printf("ret:%d.\n", ret);
//
//	return 0;
//}

