#ifndef _XT_SHULLE_DECK_H_
#define _XT_SHULLE_DECK_H_

#include "XtCard.h"
#include "XtHoleCards.h"

class XtShuffleDeck
{
	public:
		XtShuffleDeck();
		~XtShuffleDeck();

	public:
		void shuffle(int seed);
		void fill();
		void empty();
		int count() const;
		bool getHoleCards(XtHoleCards* hole_card, unsigned int num);
		int changeHoleCards(int pos,XtHoleCards* hole_card);

		bool push(const XtCard& card);
		bool pop(XtCard& card);

        void showCards(void) const;


	private:
		vector<XtCard> m_cards;
};

#endif /*_XT_SHULLE_DECK_H_*/

