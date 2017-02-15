#include "XtCard.h"

static char m_face_symbols[] = {    ///---牌面符号
	'2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
};

static char m_suit_symbols[] = {  ///---花色符号
	'd', 'c', 'h', 's'
};

XtCard::XtCard()
{
	m_face = m_suit = m_value = 0;   ///---初始化为0
}

XtCard::XtCard(int val)
{
	m_value = val;

	m_face = m_value & 0xF;
	m_suit = m_value >> 4;

	if (m_face < 2)
	{
		m_face += 13;
	}
}

void XtCard::setValue(int val)
{
	m_value = val;

	m_face = m_value & 0xF;
	m_suit = m_value >> 4;

	if (m_face < 2)
	{
		m_face += 13;
	}

	// printf("Face[%d] Suit[%d]\n", m_face, m_suit);	
}
///---得到牌面描述
string XtCard::getCardDescription()
{
	string card;

	/*
	   char buf[32];
	   snprintf(buf, sizeof(buf), "%d-%d", m_face, m_suit);
	   card.append(buf);
	   */

	card.append(1, m_face_symbols[m_face - 2]);
	card.append(1, m_suit_symbols[m_suit]);

	return card;
}



void XtCard::dumpCards(std::vector<XtCard> &v, string str )
{
	fprintf(stdout, "[%s]: [[ ", str.c_str());
	for (std::vector<XtCard>::iterator it = v.begin(); it != v.end(); it++)
		fprintf(stdout, "%s ", it->getCardDescription().c_str());

	fprintf(stdout, "]]\n");
}


void XtCard::dumpCards(std::map<int, XtCard> &m, string str)
{
	fprintf(stdout, "[%s]: [[ ", str.c_str());
	for (std::map<int, XtCard>::iterator it = m.begin(); it != m.end(); it++)
	{
		fprintf(stdout, "%s ", it->second.getCardDescription().c_str());
	}

	fprintf(stdout, "]]\n");
}









