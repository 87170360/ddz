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

		void copyCards(std::vector<XtCard> &v);
		void copyCards(std::vector<int> &v);
		void debug();

        bool same(XtHoleCards &hc);
        void reset(void);

        void popCard(const std::vector<XtCard>& out); 

	public:
		std::vector<XtCard> m_cards;
};

#endif /* _HOLE_CARDS_H_ */
