#include <algorithm>

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
        
int XtShuffleDeck::getCardType(const std::vector<XtCard>& card) const
{
    return 0;
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

bool XtShuffleDeck::isShuttle(const vector<XtCard>& card) const
{
    if(card.size() < 8 || card.size() > 20)
    {
        return false;
    }

    if((card.size() % 4 != 0) && (card.size() % 6 != 0))
    {
        return false;
    }

    vector<XtCard> cardcp(card);
    XtCard::sortByDescending(cardcp);
    map<int, int> result;
    analyze(result, cardcp);

    //检查是4或者4+2
    
    //AAAABBBB
    if(result.size() == 1 && result.find(4) != result.end()) 
    {
    }

    //AAAA12BBBB34
    if(result.size() == 2 && result.find(4) != result.end() && result.find(2) != result.end()) 
    {
    
    }

    //是否连续

    return false;
}
        
void XtShuffleDeck::analyze(map<int, int>& result, const vector<XtCard>& card) const
{
    result.clear();
    map<int, int> faceCot;
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
       faceCot[it->m_face] += 1;  
    }

    for(map<int, int>::const_iterator it = faceCot.begin(); it != faceCot.end(); ++it)
    {
        result[it->second] += 1;
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

