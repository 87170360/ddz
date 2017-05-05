#include "card.h"
#include "cardtype.h"
#include "holdcard.h"

Holdcard::Holdcard(void)
{

}

Holdcard::~Holdcard(void)
{

}
        
void Holdcard::reset(void)
{
    m_cards.clear();
    m_divide.clear();
}
        
void Holdcard::popCard(const std::vector<Card>& out) 
{
    if(out.empty())
    {
        return;
    }

    vector<Card> newCard;
    bool find = false;
    for(std::vector<Card>::const_iterator it1 = m_cards.begin(); it1 != m_cards.end(); ++it1)
    {
        find = false;
        for(std::vector<Card>::const_iterator it2 = out.begin(); it2 != out.end(); ++it2)    
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
