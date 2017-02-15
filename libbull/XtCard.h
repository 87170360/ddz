#ifndef _CARD_H_
#define _CARD_H_

#include <string>
#include <iostream>
#include <cstdio>
#include <vector>
#include <map>
#include <algorithm>

#include "XtCardType.h"

using namespace std;

/**
 * suit  0  方块  1 梅花   2  红桃    3黑桃
 * 	
 0x01, 0x11, 0x21, 0x31,		//A 14
 0x02, 0x12, 0x22, 0x32,		//2 2
 0x03, 0x13, 0x23, 0x33,		//3 3
 0x04, 0x14, 0x24, 0x34,		//4 4
 0x05, 0x15, 0x25, 0x35,		//5 5
 0x06, 0x16, 0x26, 0x36,		//6 6
 0x07, 0x17, 0x27, 0x37,		//7 7
 0x08, 0x18, 0x28, 0x38,		//8 8
 0x09, 0x19, 0x29, 0x39,		//9 9
 0x0A, 0x1A, 0x2A, 0x3A,		//10 10
 0x0B, 0x1B, 0x2B, 0x3B,		//J 11
 0x0C, 0x1C, 0x2C, 0x3C,		//Q 12
 0x0D, 0x1D, 0x2D, 0x3D,		//K 13
 * @author luochuanting
 */
///---牌定义
class XtCard
{
	public:
		typedef enum {
			Two = 2,
			Three,
			Four,
			Five,
			Six,
			Seven,
			Eight,
			Nine,
			Ten,
			Jack,
			Queen,
			King,
			Ace,

			FirstFace = Two,
			LastFace = Ace
		} Face;   ///---牌面枚举常量

		typedef enum {
			Diamonds = 0,             ///---方块
			Clubs,                    ///---梅花
			Hearts,                   ///---红心
			Spades,                   ///---黑桃

			FirstSuit = Diamonds,
			LastSuit = Spades
		} Suit;   ///---花色枚举常量

	public:
		XtCard();
		XtCard(int val);

	public:

		void setValue(int val);

		std::string getCardDescription();

		bool operator <  (const XtCard &c) const{ return (m_face < c.m_face); };
		bool operator >  (const XtCard &c) const { return (m_face > c.m_face); };
		bool operator == (const XtCard &c) const { return (m_face == c.m_face); };

		static int compare(const XtCard &a, const XtCard &b)
		{
			if (a.m_face > b.m_face)         ///---先比牌面大小
			{
				return 1;
			}
			else if (a.m_face < b.m_face)
			{
				return -1;
			}
			else if (a.m_face == b.m_face)
			{
				if (a.m_suit > b.m_suit)       ///---再比花色大小
				{
					return 1;
				}
				else if (a.m_suit < b.m_suit)
				{
					return -1;
				}	
			}

			return 0;
		}


	public:
		int m_face;    ///---牌面值
		int m_suit;    ///---花色
		int m_value;   ///---花色与牌面的组合值




	public:


		static bool lesserCallback(const XtCard &a, const XtCard &b)
		{
			if (XtCard::compare(a, b) == -1)
				return true;
			else
				return false;
		}

		static bool greaterCallback(const XtCard &a, const XtCard &b)
		{
			if (XtCard::compare(a, b) == 1)
				return true;
			else
				return false;
		}

		static void sortByAscending(std::vector<XtCard> &v)
		{
			sort(v.begin(), v.end(), XtCard::lesserCallback);
		}

		static void sortByDescending(std::vector<XtCard> &v)
		{
			sort(v.begin(), v.end(), XtCard::greaterCallback);
		}

		static void dumpCards(std::vector<XtCard> &v, string str = "cards");
		static void dumpCards(std::map<int, XtCard> &m, string str = "cards");
};

#endif /* _CARD_H_ */
