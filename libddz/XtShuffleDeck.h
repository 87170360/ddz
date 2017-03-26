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
        bool compareCard(const vector<XtCard>& card1, const vector<XtCard>& card2);
        void delCard(const vector<XtCard>& card, int seed);

	//private:
		bool pop(XtCard& card);

        bool isRocket(const vector<XtCard>& card) const;
        bool isBomb(const vector<XtCard>& card) const;
        bool isShuttle0(const vector<XtCard>& card);
        bool isShuttle2(const vector<XtCard>& card);
        bool isAircraft0(const vector<XtCard>& card);
        bool isAircraft1(const vector<XtCard>& card);
        bool isAircraft2s(const vector<XtCard>& card);
        bool is4and22s(const vector<XtCard>& card);
        bool is4and22d(const vector<XtCard>& card);
        bool is4and24(const vector<XtCard>& card);
        bool isDoubleStraight(const vector<XtCard>& card);
        bool isStraight(const vector<XtCard>& card);
        bool isThree0(const vector<XtCard>& card);
        bool isThree1(const vector<XtCard>& card);
        bool isThree2s(const vector<XtCard>& card);
        bool isPair(const vector<XtCard>& card);
        bool isSingle(const vector<XtCard>& card) const;

        //须降序队列, 不判断牌型
        bool compareBomb(const vector<XtCard>& card1, const vector<XtCard>& card2);        
        bool compareShuttle(const vector<XtCard>& card1, const vector<XtCard>& card2);        
        bool compareAircraft(const vector<XtCard>& card1, const vector<XtCard>& card2);        
        bool compare4and2(const vector<XtCard>& card1, const vector<XtCard>& card2);        
        bool compareDoubleStraight(const vector<XtCard>& card1, const vector<XtCard>& card2);
        bool compareStraight(const vector<XtCard>& card1, const vector<XtCard>& card2);
        bool compareThree(const vector<XtCard>& card1, const vector<XtCard>& card2);
        bool comparePair(const vector<XtCard>& card1, const vector<XtCard>& card2);
        bool compareSingle(const vector<XtCard>& card1, const vector<XtCard>& card2);

        //获取更大的牌, mine, other要降序, 不对other的牌型校验
        bool bigSingle(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);        
        bool bigPair(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);        
        bool bigThree2s(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);        
        bool bigThree1(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);        
        bool bigThree0(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);        
        bool bigStraight(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool bigDoubleStraight(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool big4and24(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool big4and22d(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool big4and22s(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool bigAircraft2s(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool bigAircraft1(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool bigAircraft0(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool bigShuttle2(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool bigShuttle0(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        //需要优化取最小的
        bool bigBomb(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        

        //保留相同点数的牌是N张的牌, result和card同序, 传入的card需排序（升或降）
        void keepN(vector<XtCard>& result, const vector<XtCard>& card, int nu);
        //是否是连续, 需要降序队列, 不判断n之间是否相同, n是连续相隔的数量，比如777888,n=2,  789,n=1
        bool isNContinue(const vector<XtCard>& card, int n) const;
        //比较M带N牌型
        bool compareMN(const vector<XtCard>& card, const vector<XtCard>& card1, int m);
        //整合所有相同的牌
        void delSame(const vector<XtCard>& card, vector<XtCard>& result) const;

	private:
		vector<XtCard> m_cards;
};

#endif /*_XT_SHULLE_DECK_H_*/ 
