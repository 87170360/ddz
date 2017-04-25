#ifndef _XT_SHULLE_DECK_H_
#define _XT_SHULLE_DECK_H_

#include "card.h"
#include "holdcard.h"
#include <set>

class Shuffledeck
{
	public:
		Shuffledeck(void);
		~Shuffledeck(void);

	private:
		vector<Card> m_cards;

};

#endif /*_XT_SHULLE_DECK_H_*/ 
