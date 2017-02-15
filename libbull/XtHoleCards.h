#ifndef _HOLE_CARDS_H_
#define _HOLE_CARDS_H_

#include <vector>
#include <algorithm>

#include "XtCard.h"

using namespace std;
///---手牌(手上拿的牌)
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

	public:
		std::vector<XtCard> m_cards;     ///---牌列表（多少张手牌）
		int m_cardType;                  ///---牌型
		int m_isTeshu;                   ///---特殊? 
};

#endif /* _HOLE_CARDS_H_ */
