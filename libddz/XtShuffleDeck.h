#ifndef _XT_SHULLE_DECK_H_
#define _XT_SHULLE_DECK_H_

#include "XtCard.h"
#include "XtHoleCards.h"

class XtShuffleDeck
{
	public:
		XtShuffleDeck();
		~XtShuffleDeck();

		void shuffle(int seed);
		void fill();
		void empty();
		int count() const;
		bool getHoleCards(std::vector<XtCard>& card, unsigned int num);
		int changeHoleCards(int pos,XtHoleCards* hole_card);

		bool push(const XtCard& card);
        void showCards(void) const;

        int getCardType(const std::vector<XtCard>& card) const;

	//private:
		bool pop(XtCard& card);
        bool isRocket(const vector<XtCard>& card) const;
        bool isBomb(const vector<XtCard>& card) const;
        bool isShuttle(const vector<XtCard>& card) const;

        void analyze(map<int, int>& result, const vector<XtCard>& card) const;
        void keepN(vector<XtCard>& result, const vector<XtCard>& card, int nu);

	private:
		vector<XtCard> m_cards;
};

#endif /*_XT_SHULLE_DECK_H_*/

