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

        int getCardType(const std::vector<XtCard>& card);

	//private:
		bool pop(XtCard& card);

        bool isRocket(const vector<XtCard>& card) const;
        bool isBomb(const vector<XtCard>& card) const;
        bool isShuttle(const vector<XtCard>& card);
        bool isAircraft(const vector<XtCard>& card);
        bool is4and2(const vector<XtCard>& card);
        bool isDoubleStraight(const vector<XtCard>& card);
        bool isStraight(const vector<XtCard>& card);
        bool isThree(const vector<XtCard>& card);
        bool isPair(const vector<XtCard>& card);
        bool isSingle(const vector<XtCard>& card) const;

        //保留相同点数的牌是N张的牌
        void keepN(vector<XtCard>& result, const vector<XtCard>& card, int nu);
        //是否M带N模式
        bool isMN(const vector<XtCard>& card, int m , int n) const;
        //是否是连续, 需要降序队列, 不判断n之间是否相同
        bool isNContinue(const vector<XtCard>& card, int n) const;

	private:
		vector<XtCard> m_cards;
};

#endif /*_XT_SHULLE_DECK_H_*/ 
