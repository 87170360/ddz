#include <algorithm>
#include <set>
#include "XtShuffleDeck.h"

static int card_arr[] = {
	0x00, 0x10,                 //Joker 16: 0x00 little joker, 0x10 big joker
	0x01, 0x11, 0x21, 0x31,		//A 14 
	0x02, 0x12, 0x22, 0x32,		//2 15
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
};

XtShuffleDeck::XtShuffleDeck()
{
    m_bigfun[CT_ERROR]              = &XtShuffleDeck::bigError;
    m_bigfun[CT_ROCKET]             = &XtShuffleDeck::bigRocket;
    m_bigfun[CT_BOMB]               = &XtShuffleDeck::bigBomb;
    m_bigfun[CT_SHUTTLE_0]          = &XtShuffleDeck::bigShuttle0;
    m_bigfun[CT_SHUTTLE_2]          = &XtShuffleDeck::bigShuttle2;
    m_bigfun[CT_AIRCRAFT_0]         = &XtShuffleDeck::bigAircraft0;
    m_bigfun[CT_AIRCRAFT_1]         = &XtShuffleDeck::bigAircraft1;
    m_bigfun[CT_AIRCRAFT_2S]        = &XtShuffleDeck::bigAircraft2s;
    m_bigfun[CT_4AND2_2S]           = &XtShuffleDeck::big4and22s;
    m_bigfun[CT_4AND2_2D]           = &XtShuffleDeck::big4and22d;
    m_bigfun[CT_4AND2_4]            = &XtShuffleDeck::big4and24;
    m_bigfun[CT_DOUBLE_STRAIGHT]    = &XtShuffleDeck::bigDoubleStraight;
    m_bigfun[CT_STRAIGHT]           = &XtShuffleDeck::bigStraight;
    m_bigfun[CT_THREE_0]            = &XtShuffleDeck::bigThree0;
    m_bigfun[CT_THREE_1]            = &XtShuffleDeck::bigThree1;
    m_bigfun[CT_THREE_2S]           = &XtShuffleDeck::bigThree2s;
    m_bigfun[CT_PAIR]               = &XtShuffleDeck::bigPair;
    m_bigfun[CT_SINGLE]             = &XtShuffleDeck::bigSingle;

}

XtShuffleDeck::~XtShuffleDeck()
{
}

void XtShuffleDeck::fill()
{
	m_cards.clear();

	for (int i = 0; i < 54; i++)
	{
		XtCard c(card_arr[i]);
		push(c);
	}
}

void XtShuffleDeck::empty()
{
	m_cards.clear();
}

int XtShuffleDeck::count() const
{
	return m_cards.size();
}

bool XtShuffleDeck::push(const XtCard& card)
{
	m_cards.push_back(card);
	return true;
}

bool XtShuffleDeck::pop(XtCard& card)
{
	if(m_cards.empty())
	{
		return false;
	}

    card.setValue(m_cards.back().m_value);
	m_cards.pop_back();
	return true;
}

void XtShuffleDeck::shuffle(int seed)
{
	srand(time(NULL) + seed);
	random_shuffle(m_cards.begin(), m_cards.end());
}

bool XtShuffleDeck::getHoleCards(std::vector<XtCard>& card, unsigned int num)
{
    if(m_cards.size() < num)
    {
        return false;
    }

	XtCard tmp;
	for (unsigned int i = 0; i < num; i++)
	{
		pop(tmp);
        card.push_back(tmp);
        
	}
	return true;
}

int XtShuffleDeck::changeHoleCards(int pos, XtHoleCards* holecards)
{
	if(m_cards.size()==0)
	{
		return -1;
	}

	XtCard card;
	holecards->m_cards.erase(holecards->m_cards.begin() + pos);
	pop(card);
	holecards->m_cards.push_back(card);

	return 0;
}

void XtShuffleDeck::showCards(void) const
{
    for(vector<XtCard>::const_iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
       printf("%s\n", it->getCardDescription());
    }
}
        
int XtShuffleDeck::getCardType(const std::vector<XtCard>& card) 
{
    if(isRocket(card))
    {
        return CT_ROCKET;
    }

    if(isBomb(card))
    {
        return CT_BOMB;
    }

    if(isShuttle0(card))
    {
        return CT_SHUTTLE_0;
    }

    if(isShuttle2(card))
    {
        return CT_SHUTTLE_2;
    }

    if(isAircraft0(card))
    {
        return CT_AIRCRAFT_0;
    }

    if(isAircraft1(card))
    {
        return CT_AIRCRAFT_1;
    }

    if(isAircraft2s(card))
    {
        return CT_AIRCRAFT_2S;
    }

    if(is4and22s(card))
    {
        return CT_4AND2_2S;
    }

    if(is4and22d(card))
    {
        return CT_4AND2_2D;
    }

    if(is4and24(card))
    {
        return CT_4AND2_4;
    }

    if(isDoubleStraight(card))
    {
        return CT_DOUBLE_STRAIGHT;
    }

    if(isStraight(card))
    {
        return CT_STRAIGHT;
    }

    if(isThree0(card))
    {
        return CT_THREE_0;
    }

    if(isThree1(card))
    {
        return CT_THREE_1;
    }

    if(isThree2s(card))
    {
        return CT_THREE_2S;
    }

    if(isPair(card))
    {
        return CT_PAIR;
    }

    if(isSingle(card))
    {
        return CT_SINGLE;
    }

    return CT_ERROR;
}
        
bool XtShuffleDeck::compareCard(const vector<XtCard>& card1, const vector<XtCard>& card2)
{
    int type1 = getCardType(card1);
    int type2 = getCardType(card2);
    if(type1 != type2)
    {
        return false;
    }

    switch(type1)
    {
        case CT_ROCKET: 
            return true;
        case CT_BOMB:   
            return compareBomb(card1, card2);
        case CT_SHUTTLE_0:   
        case CT_SHUTTLE_2:   
            return compareShuttle(card1, card2);
        case CT_AIRCRAFT_0:           
        case CT_AIRCRAFT_1:           
        case CT_AIRCRAFT_2S:         
            return compareAircraft(card1, card2);
        case CT_4AND2_2S:         
        case CT_4AND2_2D:         
        case CT_4AND2_4:         
            return compare4and2(card1, card2);
        case CT_DOUBLE_STRAIGHT:         
            return compareDoubleStraight(card1, card2);
        case CT_STRAIGHT:         
            return compareStraight(card1, card2);
        case CT_THREE_0:         
        case CT_THREE_1:         
        case CT_THREE_2S:         
            return compareThree(card1, card2);
        case CT_PAIR:         
            return comparePair(card1, card2);
        case CT_SINGLE:         
            return compareSingle(card1, card2);
    }

    return false;
}
        
void XtShuffleDeck::delCard(const vector<XtCard>& card, int seed)
{
   map<int, XtCard> tmap;
   for(vector<XtCard>::iterator it = m_cards.begin(); it != m_cards.end(); ++it)
   {
        tmap[it->m_value] = *it;  
   }

   map<int, XtCard>::iterator findit;
   for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
   {
      findit = tmap.find(it->m_value); 
      if(findit != tmap.end())
      {
          tmap.erase(findit); 
      }
   }

   m_cards.clear();

   for(map<int, XtCard>::const_iterator it = tmap.begin(); it != tmap.end(); ++it)
   {
       m_cards.push_back(it->second); 
   }
    
   shuffle(seed);
}
        
bool XtShuffleDeck::getOut(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& result)
{
   int cardtype = getCardType(other); 
   map<int, pBigfun>::const_iterator it = m_bigfun.find(cardtype);
   if(it == m_bigfun.end())
   {
        printf("not found big function!, cardtype:%d", cardtype);
        return false;
   }

   return (this->*(it->second))(mine, other, result);
}
        
bool XtShuffleDeck::isRocket(const vector<XtCard>& card) const
{
    if(card.size() != 2)
    {
        return false;
    }

    if(card[0].m_face == card[1].m_face && card[0].m_face == 16)
    {
        return true;
    }

    return false;
}

bool XtShuffleDeck::isBomb(const vector<XtCard>& card) const
{
    if(card.size() != 4)
    {
        return false;
    }

    if(card[0].m_face == card[1].m_face && card[1].m_face == card[2].m_face && card[2].m_face == card[3].m_face)
    {
        return true;
    }

    return false;
}

bool XtShuffleDeck::isShuttle0(const vector<XtCard>& card)
{
    if(card.size() < 8 || card.size() > 20)
    {
        return false;
    }

    if((card.size() % 4 != 0))
    {
        return false;
    }

    //取出全4的组合
    vector<XtCard> vecFour;
    keepN(vecFour, card, 4);

    //翼数量
    unsigned int wingNu = card.size() - vecFour.size(); 
    if(wingNu != 0)
    {
        return false;
    }

    //是否连续
    if(!isNContinue(vecFour, 4))
    {
       return false;  
    }

    return true;
}

bool XtShuffleDeck::isShuttle2(const vector<XtCard>& card)
{
    if(card.size() < 12 || card.size() > 20)
    {
        return false;
    }

    if((card.size() % 6 != 0))
    {
        return false;
    }

    //取出全4的组合
    vector<XtCard> vecFour;
    keepN(vecFour, card, 4);

    //翼数量
    unsigned int wingNu = card.size() - vecFour.size(); 
    if(wingNu * 2 != vecFour.size())
    {
        return false;
    }

    //是否连续
    if(!isNContinue(vecFour, 4))
    {
       return false;  
    }

    return true;
}
        
bool XtShuffleDeck::isAircraft0(const vector<XtCard>& card)
{
    if(card.size() < 6 || card.size() > 18)
    {
        return false;
    }

    //取出全3的组合
    vector<XtCard> vecThree;
    keepN(vecThree, card, 3);

    //翼数量 0 
    unsigned int wingNu = card.size() - vecThree.size(); 
    if(wingNu != 0)
    {
        return false;
    }

   //是否连续
    if(!isNContinue(vecThree, 3))
    {
       return false;  
    }

    return true;
}

bool XtShuffleDeck::isAircraft1(const vector<XtCard>& card)
{
    if(card.size() < 8 || card.size() > 20)
    {
        return false;
    }

    //取出全3的组合
    vector<XtCard> vecThree;
    keepN(vecThree, card, 3);

    //翼数量 1 
    unsigned int wingNu = card.size() - vecThree.size(); 
    if(wingNu * 3 != vecThree.size())
    {
        return false;
    }

   //是否连续
    if(!isNContinue(vecThree, 3))
    {
       return false;  
    }

    return true;
}

bool XtShuffleDeck::isAircraft2s(const vector<XtCard>& card)
{
    if(card.size() < 10 || card.size() > 20)
    {
        return false;
    }

    //取出全3的组合
    vector<XtCard> vecThree;
    keepN(vecThree, card, 3);

    //是否连续
    if(!isNContinue(vecThree, 3))
    {
       return false;  
    }

    //翼和飞机比例
    vector<XtCard> vecTwo;
    keepN(vecTwo, card, 2);

    if(vecTwo.size() + vecThree.size() != card.size())
    {
        return false;
    }

    if(vecTwo.size() * 3 == vecThree.size() * 2)
    {
        return true;
    }

    return false;
}
        
bool XtShuffleDeck::is4and22s(const vector<XtCard>& card)
{
    if(card.size() != 6)
    {
        return false;
    }

    //取出全4的组合
    vector<XtCard> vecFour;
    keepN(vecFour, card, 4);

    //取出全2的组合
    vector<XtCard> vecTwo;
    keepN(vecTwo, card, 2);

    //翼比例
    if(vecFour.size() == vecTwo.size() * 2)
    {
        return true; 
    }

    return false;
}

bool XtShuffleDeck::is4and22d(const vector<XtCard>& card)
{
    if(card.size() != 6)
    {
        return false;
    }

    //取出全4的组合
    vector<XtCard> vecFour;
    keepN(vecFour, card, 4);

    //取出全1的组合
    vector<XtCard> vecOne;
    keepN(vecOne, card, 1);

    //翼比例
    if(vecFour.size() == vecOne.size() * 2)
    {
        return true; 
    }

    return false;
}

bool XtShuffleDeck::is4and24(const vector<XtCard>& card)
{
    if(card.size() != 8)
    {
        return false;
    }

    //取出全4的组合
    vector<XtCard> vecFour;
    keepN(vecFour, card, 4);

    //取出全2的组合
    vector<XtCard> vecTwo;
    keepN(vecTwo, card, 2);

    //翼比例
    if(vecFour.size() == vecTwo.size())
    {
        return true; 
    }

    return false;
}
        
bool XtShuffleDeck::isDoubleStraight(const vector<XtCard>& card) 
{
    if(card.size() < 6 || card.size() > 20)
    {
        return false;
    }

    //取出全2的组合
    vector<XtCard> vecTwo;
    keepN(vecTwo, card, 2);

    if(vecTwo.size() != card.size())
    {
        return false; 
    }

    //连续
    if(isNContinue(vecTwo, 2))
    {
        return true; 
    }

    return false;
}
        
bool XtShuffleDeck::isStraight(const vector<XtCard>& card)
{
    if(card.size() < 5 || card.size() > 12)
    {
        return false;
    }

    //取出全1的组合
    vector<XtCard> vecOne;
    keepN(vecOne, card, 1);

    if(vecOne.size() != card.size())
    {
        return false; 
    }

    //连续
    if(isNContinue(vecOne, 1))
    {
        return true; 
    }

    return false;
}

bool XtShuffleDeck::isThree0(const vector<XtCard>& card)
{
    if(card.size() != 3)
    {
        return false;
    }

    //取出全3的组合
    vector<XtCard> vecThree;
    keepN(vecThree, card, 3);

    //翼数量 0
    unsigned int wingNu = card.size() - vecThree.size(); 
    if(wingNu != 0)
    {
        return false;
    }

    return true;
}
        
bool XtShuffleDeck::isThree1(const vector<XtCard>& card)
{
    if(card.size() != 4)
    {
        return false;
    }

    //取出全3的组合
    vector<XtCard> vecThree;
    keepN(vecThree, card, 3);

    //翼数量 0
    unsigned int wingNu = card.size() - vecThree.size(); 
    if(wingNu != 1)
    {
        return false;
    }

    return true;
}

bool XtShuffleDeck::isThree2s(const vector<XtCard>& card)
{
    if(card.size() != 5)
    {
        return false;
    }

    //取出全3的组合
    vector<XtCard> vecThree;
    keepN(vecThree, card, 3);
    //取出全2的组合
    vector<XtCard> vecTwo;
    keepN(vecTwo, card, 2);

    if(vecThree.size() + vecTwo.size() != card.size())
    {
        return false;
    }

    return true;
}

bool XtShuffleDeck::isPair(const vector<XtCard>& card)
{
    if(card.size() != 2)
    {
        return false;
    }

    if(card[0].m_face != card[1].m_face)
    {
        return false;
    }

    if(card[0].isJoker())
    {
        return false; 
    }

    return true;
}
        
bool XtShuffleDeck::isSingle(const vector<XtCard>& card) const
{
    return card.size() == 1;
}
        
bool XtShuffleDeck::compareBomb(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    return card1[0].m_face > card2[0].m_face;
}
        
bool XtShuffleDeck::compareShuttle(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    return compareMN(card1, card2, 4);
}

bool XtShuffleDeck::compareAircraft(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    return compareMN(card1, card2, 3);
}

bool XtShuffleDeck::compare4and2(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    return compareMN(card1, card2, 4);
}

bool XtShuffleDeck::compareDoubleStraight(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    return compareMN(card1, card2, 2);
}

bool XtShuffleDeck::compareStraight(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    if(card1.size() != card2.size())
    {
        return false;
    }
   
    return card1[0].m_face > card2[0].m_face;
}

bool XtShuffleDeck::compareThree(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    return compareMN(card1, card2, 3);
}

bool XtShuffleDeck::comparePair(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    return card1[0].m_face > card2[0].m_face;
}

bool XtShuffleDeck::compareSingle(const vector<XtCard>& card1, const vector<XtCard>& card2)        
{
    if(card1[0].m_face == 16 && card1[1].m_face == 16)
    {
        return card1[0].m_value > card2[0].m_value;
    }
    return card1[0].m_face > card2[0].m_face;
}
        
bool XtShuffleDeck::bigSingle(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)        
{
    for(vector<XtCard>::const_reverse_iterator it = mine.rbegin(); it != mine.rend(); ++it)
    {
        if((*it).m_face > other[0].m_face) 
        {
            out.push_back(*it);
            return true;
        }
    }
    return false;
}

bool XtShuffleDeck::bigPair(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)        
{
    if(mine.size() < 2)
    {
        return false;
    }
    vector<XtCard> vecTwo;
    keepN(vecTwo, mine, 2);
    for(size_t i = vecTwo.size() - 1; i > 2; i -= 2)
    {
        if(vecTwo[i].m_face > other[0].m_face) 
        {
            out.push_back(vecTwo[i]);
            out.push_back(vecTwo[i-1]);
            return true;
        }
    }
    return false;
}
        
bool XtShuffleDeck::bigThree2s(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)        
{
    if(mine.size() < 5)
    {
        return false;
    }

    vector<XtCard> v3;
    keepN(v3, mine, 3);

    vector<XtCard> v2;
    keepN(v2, mine, 2);

    if(v3.empty() || v2.empty())
    {
        return false;
    }

    for(size_t i = v3.size() - 1; i > 3; i -= 3)
    {
        if(v3[i].m_face > other[0].m_face) 
        {
            out.push_back(v3[i]); 
            out.push_back(v3[i - 1]); 
            out.push_back(v3[i - 2]); 
            out.push_back(v2[v2.size() - 1]); 
            out.push_back(v2[v2.size() - 2]); 
            return true;
        }
    }
    return false;
}

bool XtShuffleDeck::bigThree1(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)        
{
    if(mine.size() < 4)
    {
        return false;
    }

    vector<XtCard> v3;
    keepN(v3, mine, 3);

    vector<XtCard> v1;
    keepN(v1, mine, 1);

    if(v3.empty() || v1.empty())
    {
        return false;
    }

    for(size_t i = v3.size() - 1; i > 3; i -= 3)
    {
        if(v3[i].m_face > other[0].m_face) 
        {
            out.push_back(v3[i]); 
            out.push_back(v3[i - 1]); 
            out.push_back(v3[i - 2]); 
            out.push_back(v1.back()); 
            return true;
        }
    }
    return false;
}

bool XtShuffleDeck::bigThree0(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)        
{
    if(mine.size() < 3)
    {
        return false;
    }

    vector<XtCard> v3;
    keepN(v3, mine, 3);

    if(v3.empty())
    {
        return false;
    }

    for(size_t i = v3.size() - 1; i > 3; i -= 3)
    {
        if(v3[i].m_face > other[0].m_face) 
        {
            out.push_back(v3[i]); 
            out.push_back(v3[i - 1]); 
            out.push_back(v3[i - 2]); 
            return true;
        }
    }
    return false;
}
        
bool XtShuffleDeck::bigStraight(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    //会拆牌
    if(mine.size() < other.size())
    {
        return false;
    }

    //遍历所有该数量的顺子，然后分别比较
    
    vector<XtCard> vecdiff;
    delSame(mine, vecdiff);

    if(vecdiff.size() < other.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(int i = static_cast<int>(vecdiff.size() - other.size()); i >= 0; --i)
    {
        vectmp.clear();
        for(size_t j = 0; j < other.size(); ++j) 
        {
           vectmp.push_back(vecdiff[j + i]); 
        }
        if(isStraight(vectmp) && compareStraight(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }

    return false;
}
        
bool XtShuffleDeck::bigDoubleStraight(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    //不拆牌
    if(mine.size() < other.size())
    {
        return false;
    }

    //遍历所有该数量的双顺，然后分别比较
    
    vector<XtCard> v2;
    keepN(v2, mine, 2);

    if(v2.size() < other.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(int i = static_cast<int>(v2.size() - other.size()); i >= 0; i -= 2)
    {
        vectmp.clear();
        for(size_t j = 0; j < other.size(); ++j) 
        {
           vectmp.push_back(v2[j + i]); 
        }
        if(isDoubleStraight(vectmp) && compareDoubleStraight(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }

    return false;
}
        
bool XtShuffleDeck::big4and24(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v2;
    keepN(v2, mine, 2);

    vector<XtCard> v4;
    keepN(v4, mine, 4);


    vector<XtCard> v4o;
    keepN(v4o, other, 4);
    if(v4.size() < v4o.size())
    {
        return false;
    }

    vector<XtCard> v2o;
    keepN(v2o, other, 2);
    if(v4.size() < v2o.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(int i = static_cast<int>(v4.size() - v4o.size()); i >= 0; i -= 4)
    {
        vectmp.clear();
        for(size_t j = 0; j < v4o.size(); ++j) 
        {
           vectmp.push_back(v4[j + i]); 
        }
           
        vectmp.push_back(v2.back()); 
        vectmp.push_back(v2[v2.size() - 2]); 
        vectmp.push_back(v2[v2.size() - 3]); 
        vectmp.push_back(v2[v2.size() - 4]); 
        if(is4and24(vectmp) && compare4and2(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 
}

bool XtShuffleDeck::big4and22d(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v1;
    keepN(v1, mine, 1);

    vector<XtCard> v4;
    keepN(v4, mine, 4);


    vector<XtCard> v4o;
    keepN(v4o, other, 4);
    if(v4.size() < v4o.size())
    {
        return false;
    }

    vector<XtCard> v1o;
    keepN(v1o, other, 1);
    if(v1.size() < v1o.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(int i = static_cast<int>(v4.size() - v4o.size()); i >= 0; i -= 4)
    {
        vectmp.clear();
        for(size_t j = 0; j < v4o.size(); ++j) 
        {
           vectmp.push_back(v4[j + i]); 
        }
           
        vectmp.push_back(v1.back()); 
        vectmp.push_back(v1[v1.size() - 2]); 
        if(is4and22d(vectmp) && compare4and2(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 
}

bool XtShuffleDeck::big4and22s(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v2;
    keepN(v2, mine, 2);

    vector<XtCard> v4;
    keepN(v4, mine, 4);


    vector<XtCard> v4o;
    keepN(v4o, other, 4);
    if(v4.size() < v4o.size())
    {
        return false;
    }

    vector<XtCard> v2o;
    keepN(v2o, other, 2);
    if(v2.size() < v2o.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(int i = static_cast<int>(v4.size() - v4o.size()); i >= 0; i -= 4)
    {
        vectmp.clear();
        for(size_t j = 0; j < v4o.size(); ++j) 
        {
           vectmp.push_back(v4[j + i]); 
        }
           
        vectmp.push_back(v2.back()); 
        vectmp.push_back(v2[v2.size() - 2]); 
        if(is4and22s(vectmp) && compare4and2(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 
}

bool XtShuffleDeck::bigAircraft2s(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v2;
    keepN(v2, mine, 2);

    vector<XtCard> v3;
    keepN(v3, mine, 3);


    vector<XtCard> v3o;
    keepN(v3o, other, 3);
    if(v3.size() < v3o.size())
    {
        return false;
    }

    vector<XtCard> v2o;
    keepN(v2o, other, 2);
    if(v2.size() < v2o.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(int i = static_cast<int>(v3.size() - v3o.size()); i >= 0; i -= 3)
    {
        vectmp.clear();
        for(size_t j = 0; j < v3o.size(); ++j) 
        {
           vectmp.push_back(v3[j + i]); 
        }
           
        for(size_t k = 0; k < v2o.size(); ++k) 
        {
           vectmp.push_back(v2[v2.size() - 1 - k]); 
        }

        if(isAircraft2s(vectmp) && compareAircraft(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 
}

bool XtShuffleDeck::bigAircraft1(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    //带的翅膀，只选单牌的
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v1;
    keepN(v1, mine, 1);

    vector<XtCard> v3;
    keepN(v3, mine, 3);


    vector<XtCard> v3o;
    keepN(v3o, other, 3);
    if(v3.size() < v3o.size())
    {
        return false;
    }

    vector<XtCard> v1o;
    keepN(v1o, other, 1);
    if(v1.size() < v1o.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(int i = static_cast<int>(v3.size() - v3o.size()); i >= 0; i -= 3)
    {
        vectmp.clear();
        for(size_t j = 0; j < v3o.size(); ++j) 
        {
           vectmp.push_back(v3[j + i]); 
        }
           
        for(size_t k = 0; k < v1o.size(); ++k) 
        {
           vectmp.push_back(v1[v1.size() - 1 - k]); 
        }

        if(isAircraft1(vectmp) && compareAircraft(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 
}

bool XtShuffleDeck::bigAircraft0(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v3;
    keepN(v3, mine, 3);

    vector<XtCard> v3o;
    keepN(v3o, other, 3);
    if(v3.size() < v3o.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(int i = static_cast<int>(v3.size() - v3o.size()); i >= 0; i -= 3)
    {
        vectmp.clear();
        for(size_t j = 0; j < v3o.size(); ++j) 
        {
           vectmp.push_back(v3[j + i]); 
        }

        if(isAircraft0(vectmp) && compareAircraft(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 
}

bool XtShuffleDeck::bigShuttle2(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    //翅膀只单牌
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v4;
    keepN(v4, mine, 4);

    vector<XtCard> v4o;
    keepN(v4o, other, 4);
    if(v4.size() < v4o.size())
    {
        return false;
    }

    vector<XtCard> v1;
    keepN(v1, mine, 1);

    if(v1.size() < other.size() - v4o.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(size_t i = 0; i <= v4.size() - v4o.size(); i += 4)
    {
        vectmp.clear();
        for(size_t j = 0; j < v4o.size(); ++j) 
        {
           vectmp.push_back(v4[j + i]); 
        }

        for(size_t k = 0; k < other.size() - v4o.size(); ++k) 
        {
           vectmp.push_back(v1[v1.size() - 1 - k]); 
        }

        if(isShuttle2(vectmp) && compareShuttle(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 
}

bool XtShuffleDeck::bigShuttle0(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v4;
    keepN(v4, mine, 4);

    vector<XtCard> v4o;
    keepN(v4o, other, 4);
    if(v4.size() < v4o.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(size_t i = 0; i <= v4.size() - v4o.size(); i += 4)
    {
        vectmp.clear();
        for(size_t j = 0; j < v4o.size(); ++j) 
        {
           vectmp.push_back(v4[j + i]); 
        }

        if(isShuttle0(vectmp) && compareShuttle(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 
}
        
bool XtShuffleDeck::bigBomb(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    if(mine.size() < other.size())
    {
        return false;
    }

    vector<XtCard> v4;
    keepN(v4, mine, 4);

    if(v4.size() < other.size())
    {
        return false;
    }

    vector<XtCard> vectmp;
    for(size_t i = v4.size() - 1; i > 4 ; i -= 4)
    {
        vectmp.clear();
        for(size_t j = 0; j < other.size() && j <= i; ++j) 
        {
           vectmp.push_back(v4[i - j]); 
        }

        if(isBomb(vectmp) && compareBomb(vectmp, other))
        {
            out = vectmp;
            return true;
        }
    }
    return false; 

}
        
bool XtShuffleDeck::bigRocket(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    return false;
}

bool XtShuffleDeck::bigError(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)
{
    return false;
}

void XtShuffleDeck::keepN(vector<XtCard>& result, const vector<XtCard>& card, int nu)
{
    result.clear();
    map<int, int> faceCot;
    map<int, int>::const_iterator findit;
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
       faceCot[it->m_face] += 1;  
    }

    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        findit = faceCot.find(it->m_face);
        if(findit != faceCot.end() && findit->second == nu) 
        {
            result.push_back(*it); 
        }
    }
}
        
bool XtShuffleDeck::isNContinue(const vector<XtCard>& card, int n) const
{
    if(card.size() % n != 0)
    {
        return false;
    }

    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        //大小王和2不能算
        if(!it->isContinuCard())
        {
            return false;
        }
    }

    for(size_t i = 0; i + n < card.size(); i += n)
    {
        if(card[i].m_face != card[i + n].m_face + 1)
        {
            return false;
        }
    }
    return true;
}

bool XtShuffleDeck::compareMN(const vector<XtCard>& card1, const vector<XtCard>& card2, int m) 
{
    if(card1.size() != card2.size())
    {
        return false;
    }

    vector<XtCard> card1keep;
    vector<XtCard> card2keep;
    keepN(card1keep, card1, m);
    keepN(card2keep, card2, m);

    if(card1keep.size() != card2keep.size())
    {
        return false;
    }

    if(card1keep[0].m_face > card2keep[0].m_face)
    {
       return true; 
    }

    return false;
}
        
void XtShuffleDeck::delSame(const vector<XtCard>& card, vector<XtCard>& result) const
{
    std::set<int> setdata;
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        if(setdata.find((*it).m_face) == setdata.end())
        {
            result.push_back(*it);
            setdata.insert((*it).m_face);
        }
    }
}
