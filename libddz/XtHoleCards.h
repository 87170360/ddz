#ifndef _HOLE_CARDS_H_
#define _HOLE_CARDS_H_

#include <vector>
#include <algorithm>

#include "XtCard.h"

using namespace std;

class XtHoleCards
{
	public:
		XtHoleCards();

		void addCard(XtCard c);

		void sort();

		void analysis();

		int compare(XtHoleCards &hc);

		int getCardType();

		void clear() { m_cards.clear(); m_cardType=0; m_isTeshu=0;};

		void copyCards(std::vector<XtCard> &v);

		void copyCards(std::vector<int> &v);

		void debug();

        bool same(XtHoleCards &hc);

	public:
		std::vector<XtCard> m_cards;
		int m_cardType;
		int m_isTeshu;
};

#endif /* _HOLE_CARDS_H_ */
