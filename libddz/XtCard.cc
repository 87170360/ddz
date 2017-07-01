#include "XtCard.h"

/*
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
 */

static string m_face_symbols[] = {
	"3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A", "2", "JOKER"
};

static string m_suit_symbols[] = {
	"d", "c", "h", "s"
};

XtCard::XtCard()
{
	m_face = m_suit = m_value = 0;
}

XtCard::XtCard(int val)
{
	m_value = val;

	m_face = m_value & 0xF;
	m_suit = m_value >> 4;

    if(m_face == 0)
    {
        m_face = 16; 
    }
	else if(m_face <= 2)
	{
		m_face += 13;
	}
}

void XtCard::setValue(int val)
{
	m_value = val;

	m_face = m_value & 0xF;
	m_suit = m_value >> 4;

    if(m_face == 0)
    {
        m_face = 16; 
    }
	else if(m_face <= 2)
	{
		m_face += 13;
	}

	// printf("Face[%d] Suit[%d]\n", m_face, m_suit);	
}

const char* XtCard::getCardDescription() const
{
	string card;

	/*
	   char buf[32];
	   snprintf(buf, sizeof(buf), "%d-%d", m_face, m_suit);
	   card.append(buf);
	   */

	card.append(m_face_symbols[m_face - 3]);
	card.append(m_suit_symbols[m_suit]);

	return card.c_str();
}

string XtCard::getCardDescriptionString() const
{
	string card;
	card.append(m_face_symbols[m_face - 3]);
	card.append(m_suit_symbols[m_suit]);

	return card;
}

void XtCard::dumpCards(std::vector<XtCard> &v, string str )
{
	fprintf(stdout, "[%s]: [[ ", str.c_str());
	for (std::vector<XtCard>::iterator it = v.begin(); it != v.end(); it++)
		fprintf(stdout, "%s ", it->getCardDescription());

	fprintf(stdout, "]]\n");
}


void XtCard::dumpCards(std::map<int, XtCard> &m, string str)
{
	fprintf(stdout, "[%s]: [[ ", str.c_str());
	for (std::map<int, XtCard>::iterator it = m.begin(); it != m.end(); it++)
	{
		fprintf(stdout, "%s ", it->second.getCardDescription());
	}

	fprintf(stdout, "]]\n");
}
        
int XtCard::getCardFace(void) const
{
    if(isBigJoker()) 
    {
        return 17;
    }
    if(isLittleJoker())
    {
        return 16;
    }
    return m_face;
}
