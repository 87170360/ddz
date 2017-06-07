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
            //癞子变化牌
            if(it2->m_value != it2->m_oldvalue)
            {
                if(it1->m_value == it2->m_oldvalue)
                {
                    find = true; 
                }
            }
            else//普通牌
            {
                if(it1->m_value == it2->m_value)
                {
                    find = true; 
                }
            }

        }
        if(!find)
        {
            newCard.push_back(*it1);    
        }
    }
    m_cards = newCard;
}
