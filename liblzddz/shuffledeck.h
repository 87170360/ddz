#ifndef _XT_SHULLE_DECK_H_
#define _XT_SHULLE_DECK_H_

#include "card.h"
#include <vector>
#include <map>
#include <set>

class Shuffledeck
{
	public:
		Shuffledeck(void);
		~Shuffledeck(void);

        //洗牌
        void shuffle(int seed);
        //发手牌
		bool getHoldCard(vector<Card>& card, unsigned int num);
        //按牌型划分, 传入card需降序, key = DivideType
        void divide(const vector<Card>& card, map<int, vector<Card> >& result);

        const vector<Card>& getCard(void) { return m_card; }

	private:
        //保留相同点数的牌是N张的牌, result和card同序, 传入的card需排序（升或降）
        void keepN(vector<Card>& result, const vector<Card>& card, int nu);
        //三张组里，区分飞机和其他三张, card3降序, aircraft 是可以组成飞机的牌，比如333444777888 333444555
        void divideCard3(const vector<Card>& card3, vector<Card>& pure3, vector<Card>& aircraft);
        //两张组里，区分火箭, 纯对子和双顺, card2降序, ds是可以组成双顺的牌，比如 334455778899 33445566
        void divideCard2(const vector<Card>& card2, vector<Card>& rocket, vector<Card>& pure2, vector<Card>& ds);
        //单张组里，区分纯单张和单顺 card1降序
        void divideCard1(const vector<Card>& card1, vector<Card>& pure1, vector<Card>& straight); 
        //整合所有相同的牌
        void delSame(const vector<Card>& card, vector<Card>& result) const;
        //从单张的牌组中取至少连续N张的部分, n>=2, 各部分不一定连续，card降序且单牌队列, 不包括大小王和2, 比如n=3 345 789 , n=2 34 78
        void getNcontinue(const vector<Card>& card1, unsigned int n, std::set<int>& result);

        //癞子点数
        int m_lz;
		vector<Card> m_card;
};

#endif /*_XT_SHULLE_DECK_H_*/ 
