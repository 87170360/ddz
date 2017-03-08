#include "XtCard.h"
#include "XtCardType.h"
#include "XtHoleCards.h"

static const char *card_type_str[] = 
{
	(char *)"CARD_TYPE_ERROR",
	(char *)"CARD_TYPE_BAOZI",
	(char *)"CARD_TYPE_SHUNJIN",
	(char *)"CARD_TYPE_JINHUA",
	(char *)"CARD_TYPE_SHUNZI",
	(char *)"CARD_TYPE_DUIZI",		
	(char *)"CARD_TYPE_DANPAI",
	(char *)"CARD_TYPE_TESHU"
};

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

void XtHoleCards::analysis()
{
	XtCard::sortByDescending(m_cards);

	// is baozi
	if (m_cards[0].m_face == m_cards[1].m_face 
			&& m_cards[1].m_face == m_cards[2].m_face)
	{
		m_cardType =  CARD_TYPE_BAOZI;
		return;	
	}

	
	// is tonghua
	if (m_cards[0].m_suit == m_cards[1].m_suit 
		&& m_cards[1].m_suit == m_cards[2].m_suit)
	{
		// is shunzi
		if ((m_cards[0].m_face - m_cards[1].m_face) == 1
		&& (m_cards[1].m_face - m_cards[2].m_face) == 1)
		{
			m_cardType =  CARD_TYPE_SHUNJIN;
			return;	
		}
		else
		{
			// is 321
			if (m_cards[0].m_face == 14
				&& m_cards[1].m_face == 3
				&& m_cards[2].m_face == 2)
			{
				int temp = m_cards[0].m_value;
				m_cards[0].setValue(m_cards[1].m_value);
				m_cards[1].setValue(m_cards[2].m_value);
				m_cards[2].setValue(temp);
				m_cardType =  CARD_TYPE_SHUNJIN;
				return;	
			}
			m_cardType = CARD_TYPE_JINHUA;
			return;
		}
	}
	
	// is shunzi
	if ((m_cards[0].m_face - m_cards[1].m_face) == 1
	&& (m_cards[1].m_face - m_cards[2].m_face) == 1)
	{
		m_cardType = CARD_TYPE_SHUNZI;
		return;	
	}
	
	// is 321
	if (m_cards[0].m_face == 14
		&& m_cards[1].m_face == 3
		&& m_cards[2].m_face == 2)
	{
		int temp = m_cards[0].m_value;
		m_cards[0].setValue(m_cards[1].m_value);
		m_cards[1].setValue(m_cards[2].m_value);
		m_cards[2].setValue(temp);
		m_cardType = CARD_TYPE_SHUNZI;
		return;	
	}
	
	if (m_cards[0].m_face == m_cards[1].m_face)
	{
		m_cardType = CARD_TYPE_DUIZI;
		return;
	}
	
	if (m_cards[1].m_face == m_cards[2].m_face)
	{
		int temp = m_cards[0].m_value;
		m_cards[0].setValue(m_cards[1].m_value);
		m_cards[1].setValue(m_cards[2].m_value);
		m_cards[2].setValue(temp);
		m_cardType = CARD_TYPE_DUIZI;
		return;
	}
	
	if (m_cards[0].m_face == 5
		&& m_cards[1].m_face == 3
		&& m_cards[2].m_face == 2)
	{
		m_isTeshu = CARD_TYPE_TESHU;
		m_cardType = CARD_TYPE_DANPAI;
		return;
	}
	
	m_cardType = CARD_TYPE_DANPAI;
	return;
}

int XtHoleCards::compare(XtHoleCards &hc)
{
	if (m_cardType == CARD_TYPE_BAOZI && hc.m_isTeshu == CARD_TYPE_TESHU)
	{
		return -1;
	}
	
	if (m_isTeshu == CARD_TYPE_TESHU && hc.m_cardType == CARD_TYPE_BAOZI)
	{
		return 1;
	}
	
	if (m_cardType < hc.m_cardType)
	{
		return 1;
	}
	else if (m_cardType > hc.m_cardType)
	{
		return -1;
	}
	
	// eq
	
	if (m_cards[0].m_face > hc.m_cards[0].m_face)
	{
		return 1;

	}
	else if (m_cards[0].m_face < hc.m_cards[0].m_face)
	{
		return -1;
	}
	
	if (m_cards[1].m_face > hc.m_cards[1].m_face)
	{
		return 1;
	}
	else if (m_cards[1].m_face < hc.m_cards[1].m_face)
	{
		return -1;
	}
	
	if (m_cards[2].m_face > hc.m_cards[2].m_face)
	{
		return 1;
	}
	else if (m_cards[2].m_face < hc.m_cards[2].m_face)
	{
		return -1;
	}
	
	return 0;
}



int XtHoleCards::getCardType()
{
	return m_cardType;
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
	analysis();

	XtCard::dumpCards(m_cards);

	printf("%s\n", card_type_str[m_cardType]);
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

