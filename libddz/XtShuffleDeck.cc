#include <algorithm>
#include "XtShuffleDeck.h"

static const int card_arr[] = {
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

static const int dt_weight[] = 
{
    10,          //DT_4
    100,         //DT_3
    100,         //DT_2
    100,         //DT_1
    2,           //DT_ROCKET
    100,         //DT_STRAITHT
    100,         //DT_DS
    50,          //DT_AIRCRAFT
};

static std::map<string, int> name2value;

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
        if(pop(tmp))
        {
            card.push_back(tmp);
        }
        else
        {
            return false; 
        }

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

    if(type1 == CT_ROCKET)
    {
        return true;
    }

    int type2 = getCardType(card2);
    if(type1 == CT_BOMB && type2 != CT_BOMB && type2 != CT_ROCKET)
    {
        return true;
    }

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

    //找到更大牌型
    bool findbig = (this->*(it->second))(mine, other, result);

    //对方出的是特殊牌
    if(cardtype == CT_BOMB || cardtype == CT_ROCKET)
    {
        return findbig; 
    }

    //找特殊牌型
    map<int, vector<XtCard> > divide;
    divideCard(mine, divide); 
    bool findbomb = !divide[DT_4].empty(); 
    bool findrocket = !divide[DT_ROCKET].empty(); 
    bool findSingle = divide[DT_1].size() > 3;

    if((findbomb && findbig && rand() % 2 > 0) || (!findbig && findbomb))
    {
        if(!findSingle)  
        {
            result.clear();
            result.assign(divide[DT_4].rbegin(), divide[DT_4].rbegin() + 4); 
            return true;
        }
    }

    if((findrocket && findbig && rand() % 2 > 0) || (!findbig && findrocket))
    {
        if(!findSingle)  
        {
            result.clear();
            result.assign(divide[DT_ROCKET].begin(), divide[DT_ROCKET].end()); 
            return true;
        }
    }

    return findbig;
}

bool XtShuffleDeck::getFirst(const vector<XtCard>& mine, vector<XtCard>& result)
{
    if(mine.empty())
    {
        return false;
    }
    //划分手牌
    map<int, vector<XtCard> > dvec;
    divideCard(mine, dvec);

    //计算概率
    int totalweight = 0;
    for(map<int, vector<XtCard> >::const_iterator it = dvec.begin(); it != dvec.end(); ++it)
    {
        if(!it->second.empty())    
        {
            totalweight += dt_weight[it->first]; 
        }
    }
    int randseed = rand() % totalweight;
    int count = 0;
    int select = DT_1;
    for(map<int, vector<XtCard> >::const_iterator it = dvec.begin(); it != dvec.end(); ++it)
    {
        if(it->second.empty())
        {
            continue;
        }
        count += dt_weight[it->first];
        if(count >= randseed) 
        {
            select = it->first;
            break;
        }
    }

    switch(select)
    {
        case DT_4: 
            {
                result.assign(dvec[select].rbegin(), dvec[select].rbegin() + 4);
            }
            break;
        case DT_3: 
            {
                result.assign(dvec[select].rbegin(), dvec[select].rbegin() + 3);
            }
            break;
        case DT_2: 
            {
                result.assign(dvec[select].rbegin(), dvec[select].rbegin() + 2);
            }
            break;
        case DT_1: 
            {
                result.assign(dvec[select].rbegin(), dvec[select].rbegin() + 1);
            }
            break;
        case DT_ROCKET: 
            {
                result = dvec[select];
            }
            break;
        case DT_STRAITHT:
            {
                result.assign(dvec[select].rbegin(), dvec[select].rbegin() + 5);
            }
            break;
        case DT_DS: 
            {
                result.assign(dvec[select].rbegin(), dvec[select].rbegin() + 6);
            }
            break;
        case DT_AIRCRAFT: 
            {
                result.assign(dvec[select].rbegin(), dvec[select].rbegin() + 6);
            }
            break;
    }

    //3带1
    if(select == DT_3 && !dvec[DT_1].empty())
    {
        result.push_back(dvec[DT_1].back()); 
    }
    else if(select == DT_3 && dvec[DT_2].size() >= 2)
    {
        result.push_back(*(dvec[DT_2].rbegin()));
        result.push_back(*(dvec[DT_2].rbegin() + 1));
    }

    if(result.empty())
    {
        result.push_back(mine.back());
    }

    return true;
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

    if(vecFour.empty())
    {
        return false;
    }

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

    if(vecFour.empty())
    {
        return false;
    }

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

    if(vecThree.empty())
    {
        return false;
    }

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
    keepBigN(vecThree, card, 3);

    if(vecThree.empty())
    {
        return false;
    }

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

    if(vecThree.empty())
    {
        return false;
    }

    //是否连续
    if(!isNContinue(vecThree, 3))
    {
        return false;  
    }

    //翼和飞机比例
    vector<XtCard> vecTwo;
    keepN(vecTwo, card, 2);
    //翼可能是4张一样
    vector<XtCard> vecFour;
    keepN(vecFour, card, 4);

    if(vecTwo.size() + vecThree.size() + vecFour.size() != card.size())
    {
        return false;
    }

    if((vecTwo.size() + vecFour.size()) * 3 == vecThree.size() * 2)
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

    if(vecFour.empty() || vecTwo.empty())
    {
        return false;
    }

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

    if(vecFour.empty() || vecOne.size() < 2)
    {
        return false;
    }

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

    if(vecFour.empty() || vecTwo.empty())
    {
        return false;
    }

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

    if(vecThree.empty())
    {
        return false;
    }

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

    if(vecThree.empty())
    {
        return false;
    }

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

    if(vecThree.empty())
    {
        return false;
    }

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
    if(card1[0].m_face == 16 && card2[0].m_face == 16)
    {
        return card1[0].m_value > card2[0].m_value;
    }
    return card1[0].m_face > card2[0].m_face;
}

bool XtShuffleDeck::bigSingle(const vector<XtCard>& mine, const vector<XtCard>& other, vector<XtCard>& out)        
{
    //从单张中选
    map<int, vector<XtCard> > divide;
    divideCard(mine, divide); 
    if(divide[DT_1].size() > 0)
    {
        for(vector<XtCard>::const_reverse_iterator it = divide[DT_1].rbegin(); it != divide[DT_1].rend(); ++it)
        {
            if((*it).m_face > other[0].m_face) 
            {
                out.push_back(*it);
                return true;
            }
        }
    }

    //从全部手牌中选
    for(vector<XtCard>::const_reverse_iterator it = mine.rbegin(); it != mine.rend(); ++it)
    {
        //大小王比较
        if((*it).isBigJoker())
        {
            out.push_back(*it);
            return true;
        }
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
    if(vecTwo.size() < out.size())
    {
        return false;
    }

    if(other.size() != 2)
    {
        return false;
    }

    if(vecTwo.size() == 2)
    {
        if(vecTwo[0].m_face > other[0].m_face)
        {
            out.assign(vecTwo.begin(), vecTwo.end()); 
            return true;
        }
    }

    for(int i = static_cast<int>(vecTwo.size() - 1); i >= 1; i -= 2)
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

    vector<XtCard> v3o;
    keepN(v3o, other, 3);

    vector<XtCard> v2;
    keepN(v2, mine, 2);

    if(v3.empty() || v2.empty())
    {
        return false;
    }

    //去掉大小王
    if(v2.size() == 2 && v2[0].isJoker())
    {
        return false;
    }

    if(v3.size() == 3)
    {
        if(v3[0].m_face > v3o[0].m_face) 
        {
            out.assign(v3.begin(), v3.end()); 
            out.push_back(v2[0]); 
            out.push_back(v2[1]); 
            return true; 
        }
    }

    for(int i = static_cast<int>(v3.size() - 1); i >= 2; i -= 3)
    {
        if(v3[i].m_face > v3o[0].m_face) 
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

    vector<XtCard> v3o;
    keepN(v3o, other, 3);

    vector<XtCard> v1;
    keepN(v1, mine, 1);

    if(v3.empty() || v1.empty())
    {
        return false;
    }

    if(v3.size() == 3)
    {
        if(v3[0].m_face > v3o[0].m_face) 
        {
            out.assign(v3.begin(), v3.end()); 
            out.push_back(v1.back());
            return true; 
        }
    }

    for(int i = static_cast<int>(v3.size() - 1); i >= 2; i -= 3)
    {
        if(v3[i].m_face > v3o[0].m_face) 
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

    if(v3.size() == other.size() && v3.size() == 3)
    {
        if(v3[0].m_face > other[0].m_face) 
        {
            out.assign(v3.begin(), v3.end()); 
            return true; 
        }
    }

    for(int i = static_cast<int>(v3.size() - 1); i >= 2; i -= 3)
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

    vector<XtCard> v2old;
    keepN(v2old, mine, 2);

    //去掉大小王
    vector<XtCard> v2;
    for(size_t i = 0; i < v2old.size(); ++i)
    {
        if(v2old[i].isJoker() == false) 
        {
            v2.push_back(v2old[i]);
        }
    }

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
    for(int i = static_cast<int>(v4.size() - 1); i > 4 ; i -= 4)
    {
        vectmp.clear();
        for(int j = 0; j < static_cast<int>(other.size()) && j <= i; ++j) 
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

void XtShuffleDeck::divideCard(const vector<XtCard>& card, map<int, vector<XtCard> >& result)
{
    vector<XtCard> vec4;
    keepN(vec4, card, 4);
    result[DT_4] = vec4;

    //三张组里，区分飞机和其他三张
    vector<XtCard> vec3;
    keepN(vec3, card, 3);
    vector<XtCard> pure3;
    vector<XtCard> aircraft;
    divideCard3(vec3, pure3, aircraft);
    result[DT_3] = pure3;
    result[DT_AIRCRAFT] = aircraft;

    //两张组里，区分火箭, 纯对子和双顺
    vector<XtCard> vec2;
    keepN(vec2, card, 2);
    vector<XtCard> rocket;
    vector<XtCard> ds;
    vector<XtCard> pure2;
    divideCard2(vec2, rocket, pure2, ds);
    result[DT_2] = pure2;
    result[DT_ROCKET] = rocket;
    result[DT_DS] = ds;

    //单张组里，区分纯单张和单顺
    vector<XtCard> vec1;
    keepN(vec1, card, 1);
    vector<XtCard> straight;
    vector<XtCard> pure1;
    divideCard1(vec1, pure1, straight);
    result[DT_1] = pure1;
    result[DT_STRAITHT] = straight;
}


void XtShuffleDeck::divideCard3(const vector<XtCard>& card3, vector<XtCard>& pure3, vector<XtCard>& aircraft)
{
    if(card3.size() < 6)
    {
        pure3 = card3;
        return;
    }

    vector<XtCard> vec1; 
    delSame(card3, vec1);
    if(vec1.size() < 2)
    {
        pure3 = card3;
        return;
    }

    //获取连续部分
    set<int> tmpface;
    getNcontinue(vec1, 2, tmpface);

    for(vector<XtCard>::const_iterator it = card3.begin(); it != card3.end(); ++it)
    {
        if(tmpface.find((*it).m_face) == tmpface.end()) 
        {
            pure3.push_back(*it); 
        }
        else
        {
            aircraft.push_back(*it); 
        }
    }
}

void XtShuffleDeck::divideCard2(const vector<XtCard>& card2, vector<XtCard>& rocket, vector<XtCard>& pure2, vector<XtCard>& ds)
{
    vector<XtCard> tmpcard;
    //获取火箭
    for(vector<XtCard>::const_iterator it = card2.begin(); it != card2.end(); ++it)
    {
        if((*it).isJoker())     
        {
            rocket.push_back(*it);
        }
        else
        {
            tmpcard.push_back(*it);
        }
    }

    //双顺
    //保留单张 
    vector<XtCard> vec1; 
    delSame(tmpcard, vec1);
    if(vec1.size() < 3)
    {//数量不够, 没有双顺
        pure2 = tmpcard;
        return;
    }

    //获取连续3张部分
    set<int> tmpface;
    getNcontinue(vec1, 3, tmpface);

    for(vector<XtCard>::const_iterator it = tmpcard.begin(); it != tmpcard.end(); ++it)
    {
        if(tmpface.find((*it).m_face) == tmpface.end()) 
        {
            pure2.push_back(*it); 
        }
        else
        {
            ds.push_back(*it); 
        }
    }
}

void XtShuffleDeck::divideCard1(const vector<XtCard>& card1, vector<XtCard>& pure1, vector<XtCard>& straight) 
{
    vector<XtCard> vec1; 
    delSame(card1, vec1);
    //没有单顺
    if(vec1.size() < 5)
    {
        pure1 = card1;
        return;
    }

    //获取连续>5张部分
    set<int> tmpface;
    getNcontinue(vec1, 5, tmpface);

    for(vector<XtCard>::const_iterator it = card1.begin(); it != card1.end(); ++it)
    {
        if(tmpface.find((*it).m_face) == tmpface.end()) 
        {
            pure1.push_back(*it); 
        }
        else
        {
            straight.push_back(*it); 
        }
    }
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

void XtShuffleDeck::keepBigN(vector<XtCard>& result, const vector<XtCard>& card, int nu)
{
    result.clear();
    map<int, int> faceCot;
    map<int, int>::const_iterator findit;
    map<int, int> faceGet;
    map<int, int>::const_iterator findit2;
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        faceCot[it->m_face] += 1;  
        faceGet[it->m_face] = 0;  
    }

    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        findit = faceCot.find(it->m_face);
        if(findit != faceCot.end() && findit->second >= nu) 
        {
            findit2 = faceGet.find(it->m_face);
            if(findit2 != faceGet.end() && findit2->second < nu)
            {
                result.push_back(*it); 
                faceGet[it->m_face] += 1;
            }
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

void XtShuffleDeck::getNcontinue(const vector<XtCard>& card1, unsigned int n, std::set<int>& result)
{
    if(card1.empty() || n < 2)
    {
        return;
    }
    set<int> tmpds;
    set<int> tmpface;
    for(size_t i = 1; i < card1.size(); ++i)
    {
        if((card1[i - 1].m_face == card1[i].m_face + 1) && card1[i - 1].isContinuCard() && card1[i].isContinuCard()) 
        {
            tmpface.insert(card1[i - 1].m_face);    
            tmpface.insert(card1[i].m_face);    
        }
        else
        {
            //检查前面部分数量是否够n张 
            if(tmpface.size() >= n)
            {
                for(set<int>::iterator it = tmpface.begin(); it != tmpface.end(); ++it)
                {
                    tmpds.insert(*it);  
                }
            }
            tmpface.clear();
        }
    }
    if(tmpface.size() >= n)
    {
        for(set<int>::iterator it = tmpface.begin(); it != tmpface.end(); ++it)
        {
            tmpds.insert(*it);  
        }
    }

    for(vector<XtCard>::const_iterator it = card1.begin(); it != card1.end(); ++it)
    {
        if(tmpds.find((*it).m_face) != tmpds.end()) 
        {
            result.insert((*it).m_face);
        }
    }
}

const map<string, int>& XtShuffleDeck::getName2Value(void)
{
    if(name2value.empty())
    {
        name2value["JOKERc"]     = 16; 
        name2value["JOKERd"]     = 0; 
        name2value["2s"]         = 50;  name2value["2h"]         = 34;  name2value["2c"]         = 18;  name2value["2d"]         = 2;
        name2value["As"]         = 49;  name2value["Ah"]         = 33;  name2value["Ac"]         = 17;  name2value["Ad"]         = 1;
        name2value["Ks"]         = 61;  name2value["Kh"]         = 45;  name2value["Kc"]         = 29;  name2value["Kd"]         = 13;
        name2value["Qs"]         = 60;  name2value["Qh"]         = 44;  name2value["Qc"]         = 28;  name2value["Qd"]         = 12;
        name2value["Js"]         = 59;  name2value["Jh"]         = 43;  name2value["Jc"]         = 27;  name2value["Jd"]         = 11;
        name2value["10s"]        = 58;  name2value["10h"]        = 42;  name2value["10c"]        = 26;  name2value["10d"]        = 10;
        name2value["9s"]         = 57;  name2value["9h"]         = 41;  name2value["9c"]         = 25;  name2value["9d"]         = 9;
        name2value["8s"]         = 56;  name2value["8h"]         = 40;  name2value["8c"]         = 24;  name2value["8d"]         = 8;
        name2value["7s"]         = 55;  name2value["7h"]         = 39;  name2value["7c"]         = 23;  name2value["7d"]         = 7;
        name2value["6s"]         = 54;  name2value["6h"]         = 38;  name2value["6c"]         = 22;  name2value["6d"]         = 6;
        name2value["5s"]         = 53;  name2value["5h"]         = 37;  name2value["5c"]         = 21;  name2value["5d"]         = 5;
        name2value["4s"]         = 52;  name2value["4h"]         = 36;  name2value["4c"]         = 20;  name2value["4d"]         = 4;
        name2value["3s"]         = 51;  name2value["3h"]         = 35;  name2value["3c"]         = 19;  name2value["3d"]         = 3;
    }
    return name2value;
}

void XtShuffleDeck::split(const std::string& s, const std::string& delim, std::vector<std::string>& ret)  
{  
    size_t last = 0;  
    size_t index=s.find_first_of(delim,last);  
    while (index!=std::string::npos)  
    {  
        ret.push_back(s.substr(last,index-last));  
        last=index+1;  
        index=s.find_first_of(delim,last);  
    }  
    if (index-last>0)  
    {  
        ret.push_back(s.substr(last,index-last));  
    }  
}  
        
void XtShuffleDeck::getConfigCard(vector<XtCard>& card, const string& config) 
{
    vector<std::string> name;    
    split(config, "-", name);

    map<string, int> _name2value = getName2Value();
    map<string, int>::const_iterator nameit;

    for(vector<std::string>::const_iterator it = name.begin(); it != name.end(); ++it)
    {
        nameit = _name2value.find(*it);
        if(nameit != _name2value.end())
        {
            card.push_back(XtCard(nameit->second));
        }
        else
        {
            printf("not find %s\n", it->c_str()); 
        }
    }
}
