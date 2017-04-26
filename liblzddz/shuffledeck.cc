#include <algorithm>
#include "shuffledeck.h"

static const int CARD_ARR[] = {
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

static const int CARD_NUM = 54;

Shuffledeck::Shuffledeck(void) : m_lz(-1)
{
}

Shuffledeck::~Shuffledeck(void)
{
}

void Shuffledeck::shuffle(int seed)
{
    //处理牌
    m_card.clear(); 
    for (int i = 0; i < CARD_NUM; i++)
    {
        m_card.push_back(Card(CARD_ARR[i]));
    }

    srand(time(NULL) + seed);
    random_shuffle(m_card.begin(), m_card.end());

    //产生癞子
    for(vector<Card>::const_iterator it = m_card.begin(); it != m_card.end(); ++it)
    {
        if(!it->isJoker())  
        {
            m_lz = it->m_face;
            //printf("lz:%d\n", it->m_face);
            break;
        }
    }
}
		
bool Shuffledeck::getHoldCard(vector<Card>& card, unsigned int num)
{
    if(num > m_card.size())
    {
        return false;
    }

    for(unsigned int i = 0; i < num; ++i)
    {
        card.push_back(m_card.back());
        m_card.pop_back();
    }
    return true;
}
        
void Shuffledeck::divide(const vector<Card>& card, map<int, vector<Card> >& result)
{
    //不包含lz
    vector<Card> vectmp;
    //癞子组
    vector<Card> veclz;
    for(vector<Card>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        if(it->m_face == m_lz) 
        {
            veclz.push_back(*it);
        }
        else
        {
            vectmp.push_back(*it);
        }
    }
    result[DT_LZ] = veclz;
    
    
    //四张组
    vector<Card> vec4;
    keepN(vec4, vectmp, 4);
    result[DT_4] = vec4;

    //三张组里，区分飞机和其他三张
    vector<Card> vec3;
    keepN(vec3, vectmp, 3);
    vector<Card> pure3;
    vector<Card> aircraft;
    divideCard3(vec3, pure3, aircraft);
    result[DT_3] = pure3;
    result[DT_AIRCRAFT] = aircraft;

    //两张组里，区分火箭, 纯对子和双顺
    vector<Card> vec2;
    keepN(vec2, vectmp, 2);
    vector<Card> rocket;
    vector<Card> ds;
    vector<Card> pure2;
    divideCard2(vec2, rocket, pure2, ds);
    result[DT_2] = pure2;
    result[DT_ROCKET] = rocket;
    result[DT_DS] = ds;

    //单张组里，区分纯单张和单顺
    vector<Card> vec1;
    keepN(vec1, vectmp, 1);
    vector<Card> straight;
    vector<Card> pure1;
    divideCard1(vec1, pure1, straight);
    result[DT_1] = pure1;
    result[DT_STRAITHT] = straight;
}

void Shuffledeck::keepN(vector<Card>& result, const vector<Card>& card, int nu)
{
    result.clear();
    map<int, int> faceCot;
    map<int, int>::const_iterator findit;
    for(vector<Card>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        faceCot[it->m_face] += 1;  
    }

    for(vector<Card>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        findit = faceCot.find(it->m_face);
        if(findit != faceCot.end() && findit->second == nu) 
        {
            result.push_back(*it); 
        }
    }
}

void Shuffledeck::divideCard3(const vector<Card>& card3, vector<Card>& pure3, vector<Card>& aircraft)
{
    if(card3.size() < 6)
    {
        pure3 = card3;
        return;
    }

    vector<Card> vec1; 
    delSame(card3, vec1);
    if(vec1.size() < 2)
    {
        pure3 = card3;
        return;
    }

    //获取连续部分
    set<int> tmpface;
    getNcontinue(vec1, 2, tmpface);

    for(vector<Card>::const_iterator it = card3.begin(); it != card3.end(); ++it)
    {
        if(tmpface.find(it->m_face) == tmpface.end()) 
        {
            pure3.push_back(*it); 
        }
        else
        {
            aircraft.push_back(*it); 
        }
    }
}

void Shuffledeck::divideCard2(const vector<Card>& card2, vector<Card>& rocket, vector<Card>& pure2, vector<Card>& ds)
{
    vector<Card> tmpcard;
    //获取火箭
    for(vector<Card>::const_iterator it = card2.begin(); it != card2.end(); ++it)
    {
        if(it->isJoker())     
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
    vector<Card> vec1; 
    delSame(tmpcard, vec1);
    if(vec1.size() < 3)
    {//数量不够, 没有双顺
        pure2 = tmpcard;
        return;
    }

    //获取连续3张部分
    set<int> tmpface;
    getNcontinue(vec1, 3, tmpface);

    for(vector<Card>::const_iterator it = tmpcard.begin(); it != tmpcard.end(); ++it)
    {
        if(tmpface.find(it->m_face) == tmpface.end()) 
        {
            pure2.push_back(*it); 
        }
        else
        {
            ds.push_back(*it); 
        }
    }
}

void Shuffledeck::divideCard1(const vector<Card>& card1, vector<Card>& pure1, vector<Card>& straight) 
{
    vector<Card> vec1; 
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

    for(vector<Card>::const_iterator it = card1.begin(); it != card1.end(); ++it)
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

void Shuffledeck::delSame(const vector<Card>& card, vector<Card>& result) const
{
    std::set<int> setdata;
    for(vector<Card>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        if(setdata.find(it->m_face) == setdata.end())
        {
            result.push_back(*it);
            setdata.insert((*it).m_face);
        }
    }
}

void Shuffledeck::getNcontinue(const vector<Card>& card1, unsigned int n, std::set<int>& result)
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

    for(vector<Card>::const_iterator it = card1.begin(); it != card1.end(); ++it)
    {
        if(tmpds.find((*it).m_face) != tmpds.end()) 
        {
            result.insert((*it).m_face);
        }
    }
}
