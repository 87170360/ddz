#ifndef _XT_SHULLE_DECK_H_
#define _XT_SHULLE_DECK_H_

#include "XtCard.h"
#include "XtHoleCards.h"
#include <set>

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
        //要降序
        int getCardType(const std::vector<XtCard>& card);
        //要降序
        bool compareCard(const vector<XtCard>& card1, const vector<XtCard>& card2);
        void delCard(const vector<XtCard>& card, int seed);
        //获取可出的牌, 降序
        bool getOut(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& result);
        //获取首轮出牌, 降序
        bool getFirst(const vector<XtCard>& mine, vector<XtCard>& result);

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

        //须降序队列, 不判断牌型, card1 比 card2 大 return true
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
        bool bigBomb(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool bigRocket(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);
        bool bigError(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out);

        //按牌型划分, 传入card需降序, key = DivideType
        void divideCard(const vector<XtCard>& card, map<int, vector<XtCard> >& result);
        //三张组里，区分飞机和其他三张, card3降序, aircraft 是可以组成飞机的牌，比如333444777888 333444555
        void divideCard3(const vector<XtCard>& card3, vector<XtCard>& pure3, vector<XtCard>& aircraft);
        //两张组里，区分火箭, 纯对子和双顺, card2降序, ds是可以组成双顺的牌，比如 334455778899 33445566
        void divideCard2(const vector<XtCard>& card2, vector<XtCard>& rocket, vector<XtCard>& pure2, vector<XtCard>& ds);
        //单张组里，区分纯单张和单顺和大小王 card1降序
        void divideCard1(const vector<XtCard>& card1, vector<XtCard>& joker, vector<XtCard>& pure1, vector<XtCard>& straight); 

        //保留相同点数的牌是N张的牌, result和card同序, 传入的card需排序（升或降）
        void keepN(vector<XtCard>& result, const vector<XtCard>& card, int nu);
        //是否是连续, 需要降序队列, 不判断n之间是否相同, n是连续相隔的数量, 不包括大小王和2, 比如777888,n=2,  789,n=1
        bool isNContinue(const vector<XtCard>& card, int n) const;
        //比较M带N牌型
        bool compareMN(const vector<XtCard>& card, const vector<XtCard>& card1, int m);
        //整合所有相同的牌
        void delSame(const vector<XtCard>& card, vector<XtCard>& result) const;
        //从单张的牌组中取至少连续N张的部分, n>=2, 各部分不一定连续，card降序且单牌队列, 不包括大小王和2, 比如n=3 345 789 , n=2 34 78
        void getNcontinue(const vector<XtCard>& card1, unsigned int n, std::set<int>& result);

	private:
		vector<XtCard> m_cards;

        typedef bool (XtShuffleDeck::*pBigfun) (const vector<XtCard>&, const vector<XtCard>&, vector<XtCard>&);
        map<int, pBigfun> m_bigfun;
};

#endif /*_XT_SHULLE_DECK_H_*/ 
