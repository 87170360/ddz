#include "XtCard.h"
#include "XtCardType.h"
#include "XtHoleCards.h"

XtHoleCards::XtHoleCards()
{

}

void XtHoleCards::addCard(XtCard card)
{
	m_cards.push_back(card);
}

void XtHoleCards::sort()
{
	XtCard::sortByDescending(m_cards);
}

void XtHoleCards::copyCards(std::vector<XtCard> &v)
{
	v = m_cards;
}

void XtHoleCards::copyCards(std::vector<int> &v)
{
	for (unsigned int i = 0; i < m_cards.size(); i++)
	{
		v.push_back(m_cards[i].m_value);
	}
}

void XtHoleCards::debug()
{
	XtCard::dumpCards(m_cards);
}

bool XtHoleCards::same(XtHoleCards &hc)
{
    if(m_cards.size() != hc.m_cards.size())
    {
        return false;
    }

    for(int i = 0; i < static_cast<int>(m_cards.size()); ++i)
    {
        //printf("check card, face1:%d, face2:%d, suit1:%d, suit2:%d\n", m_cards[i].m_face, hc.m_cards[i].m_face, m_cards[i].m_suit, hc.m_cards[i].m_suit);
       if(m_cards[i].m_face == hc.m_cards[i].m_face && m_cards[i].m_suit == hc.m_cards[i].m_suit)  
       {
            return true; 
       }

    }
    return false;
}

void XtHoleCards::reset(void)
{
    m_cards.clear();
}
        
void XtHoleCards::popCard(const std::vector<XtCard>& out) 
{
    if(out.empty())
    {
        return;
    }

    vector<XtCard> newCard;
    bool find = false;
    for(std::vector<XtCard>::const_iterator it1 = m_cards.begin(); it1 != m_cards.end(); ++it1)
    {
        find = false;
        for(std::vector<XtCard>::const_iterator it2 = out.begin(); it2 != out.end(); ++it2)    
        {
            if(it1->m_face == it2->m_face && it1->m_suit == it2->m_suit)
            {
                find = true; 
            }
        }
        if(!find)
        {
            newCard.push_back(*it1);    
        }
    }
    m_cards = newCard;
}
