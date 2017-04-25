#ifndef _HOLDCARD_H_
#define _HOLDCARD_H_

#include <vector>
#include <algorithm>

#include "card.h"

using namespace std;

class Holdcard
{
	public:
		Holdcard(void);
		~Holdcard(void);
	public:
        //key : Card value
		std::map<int, Card> m_cards;
};

#endif /* _HOLDCARDS_H_ */
