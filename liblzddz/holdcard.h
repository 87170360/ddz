#ifndef _HOLDCARD_H_
#define _HOLDCARD_H_

#include <vector>
#include <map>
#include <algorithm>

#include "card.h"

using namespace std;

class Holdcard
{
	public:
		Holdcard(void);
		~Holdcard(void);
	public:
		vector<Card> m_cards;
        //分组牌, key : DT_TYPE
		std::map<int, Card> m_divide;
};

#endif /* _HOLDCARDS_H_ */
