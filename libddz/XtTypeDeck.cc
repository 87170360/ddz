#include <vector>
#include <algorithm>
#include <assert.h>

#include "XtTypeDeck.h"

// 0  方块  1 梅花   2  红桃    3黑桃


static int s_card_d[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
static int s_card_c[] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};
static int s_card_h[] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D};
static int s_card_s[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D};

static int* s_cards[]={s_card_d,s_card_c,s_card_h,s_card_s};



class E_CardType 
{
	public:
		enum 
		{
			ERROR=0,
			BAOZHI=1,
			SHUNJIN=2,
			JINHUA=3,
			SHUNZI=4,
			DUIZI=5,
			DANPAI=6,
			TESHU=7,
		};
};



XtTypeDeck::XtTypeDeck()
{
	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		for(int j=0;j<XT_CARD_SINGLE_SUIT_NU;j++)
		{
			m_cardMask[i][j]=0;

		}
	}


	for(int i=0;i<XT_MAX_CARD_TYPE;i++)
	{
		m_typeWeight[i]=1;
	}

	m_typeTotal=6;
}


XtTypeDeck::~XtTypeDeck()
{

}


void XtTypeDeck::setTypeWeight(int* weight)
{
	m_typeTotal=0;
	for(int i=0;i<XT_MAX_CARD_TYPE;i++)
	{
		m_typeWeight[i]=weight[i];
		m_typeTotal+=weight[i];
	}
}



int XtTypeDeck::cardIsShunZi(int a,int b, int c)
{
	//printf("%d,%d,%d\n",a,b,c);
	std::vector<int> v_face;

	v_face.push_back(a);
	v_face.push_back(b);
	v_face.push_back(c);

	std::sort(v_face.begin(),v_face.end(),std::less<int>());


	if((v_face[1]-v_face[0]==1)&&(v_face[2]-v_face[1]==1))
	{
		return true;
	}

	if(v_face[0]==0&&v_face[1]==11&&v_face[2]==12)
	{
		return true;
	}

	return false;
}



int XtTypeDeck::shuffle(int seed)
{
	srand(time(NULL)+seed);
	//srand(seed);
	return 0;
}


void XtTypeDeck::fill()
{
	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		for(int j=0;j<XT_CARD_SINGLE_SUIT_NU;j++)
		{
			m_cardMask[i][j]=0;

		}
	}
}

int XtTypeDeck::count() const
{
	int value=0;
	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		for(int j=0;j<XT_CARD_SINGLE_SUIT_NU;j++)
		{
			if(m_cardMask[i][j]==0)
			{
				value++;
			}
		}
	}

	return value;
}



void XtTypeDeck::empty()
{
	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		for(int j=0;j<XT_CARD_SINGLE_SUIT_NU;j++)
		{
			m_cardMask[i][j]=1;

		}
	}
}


int XtTypeDeck::randomNum(int start,int end)
{
	if(start==end)
	{
		return start;
	}

	return rand()%(end-start)+start;
}


int XtTypeDeck::randomType()
{
	int value=rand()%m_typeTotal;


	for(int i=0;i<XT_MAX_CARD_TYPE;i++)
	{

		value-=m_typeWeight[i];


		if(value<0)
		{
			return i;
		}
	}

	assert(0);
	return -1;
}



int XtTypeDeck::randomCard(int start,int end,int suit)
{

	int value=randomNum(start,end);

	for(int i=0;i<(end-start);i++)
	{
		if(!m_cardMask[suit][value])
		{
			return value;
		}
		value++;
		if(value>=end)
		{
			value=value-end+start;
		}
	}

	return -1;

}

int XtTypeDeck::randomCardWithMark(int start,int end,int suit,unsigned int mark)
{
	int value=randomNum(start,end);

	//printf("mark:%x\n",mark);

	for(int i=0;i<(end-start);i++)
	{
		if((!m_cardMask[suit][value])&&(!(mark&(1<<value))))
		{
			return value;
		}
		value++;
		if(value>=end)
		{
			value=value-end+start;
		}
	}

	return -1;
}



int XtTypeDeck::getHoleCards(XtHoleCards* card,int card_type)
{

	int ret=-1;
	card->clear();

	switch(card_type)
	{
		case E_CardType::BAOZHI:
			ret=getBaoZi(card);
			break;

		case E_CardType::SHUNJIN:
			ret=getShunJin(card);
			break;

		case E_CardType::JINHUA:
			ret=getJinHua(card);
			break;

		case E_CardType::SHUNZI:
			ret=getShunZi(card);
			break;

		case E_CardType::DUIZI:
			ret=getDuiZhi(card);
			break;

		case E_CardType::DANPAI:
			ret=getGaoPai(card);
			break;

		default:
			assert(0);
			break;

	}
	return ret;
}



int XtTypeDeck::getHoleCards(XtHoleCards* card)
{

	int card_type=randomType()+1;


	int ret=getHoleCards(card,card_type);

	if(ret<0)
	{
		return -1;
	}

	return card_type;
}


int XtTypeDeck::changeHoleCards(int pos,XtHoleCards* holecards)
{

	XtCard card;
	if(pop(&card)<0)
	{
		return -1;
	}


	holecards->m_cards.erase(holecards->m_cards.begin() + pos);
	holecards->m_cards.push_back(card);
	holecards->analysis();

	return 0;
}






int XtTypeDeck::getBaoZi(XtHoleCards* card)
{
	card->clear();

	int face=randomNum(0,XT_CARD_SINGLE_SUIT_NU);
	int suit=randomNum(0,XT_CARD_SUIT_NU);

	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		for(int j=0;j<XT_CARD_SINGLE_SUIT_NU;j++)
		{
			int ret=getBaoZiByCard(card,suit,face);
			if(ret==0)
			{
				return 0;
			}

			face=(face+1)%XT_CARD_SINGLE_SUIT_NU;
		}
		suit=(suit+1)%XT_CARD_SUIT_NU;
	}

	return -1;
}

int XtTypeDeck::getBaoZiByCard(XtHoleCards* card,int suit,int face)
{
	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		if(suit==i)
		{
			continue;
		}

//		printf("card:%d\n",m_cardMask[i][face]);
		if(m_cardMask[i][face])
		{
			return -1;
		}
	}


	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		if(suit==i)
		{
			continue;
		}

		m_cardMask[i][face]=1;
		card->addCard(s_cards[i][face]);
	}

	return 0;
}




int XtTypeDeck::getShunJin(XtHoleCards* card)
{
	card->clear();

	int face=randomNum(1,XT_CARD_SINGLE_SUIT_NU);
	int suit=randomNum(0,XT_CARD_SUIT_NU);


	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		for(int j=0;j<XT_CARD_SINGLE_SUIT_NU;j++)
		{
			int ret=getShunJinByCard(card,suit,face);
			if(ret==0)
			{
				return 0;
			}
			face=(face+1)%XT_CARD_SINGLE_SUIT_NU;
		}
		suit=(suit+1)%XT_CARD_SUIT_NU;
	}
	return -1;
}


int XtTypeDeck::getShunJinByCard(XtHoleCards* card,int suit,int face)
{
	if(face==0)
	{
		return -1;
	}

	for(int i=-1;i<=1;i++)
	{
		if(m_cardMask[suit][(face+i)%XT_CARD_SINGLE_SUIT_NU])
		{
			return -1;
		}
	}


	for(int i=-1;i<=1;i++)
	{
		m_cardMask[suit][(face+i)%XT_CARD_SINGLE_SUIT_NU]=1;
		card->addCard(s_cards[suit][(face+i)%XT_CARD_SINGLE_SUIT_NU]);
	}

	return 0;
}


int XtTypeDeck::getJinHua(XtHoleCards* card)
{
	card->clear();

	int suit=randomNum(0,XT_CARD_SUIT_NU);

	int face1=-1;
	int face2=-1;
	int face3=-1;

	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		face1=-1;
		face2=-1;
		face3=-1;

		int face_1=randomNum(0,XT_CARD_SINGLE_SUIT_NU);


		for(int i=0;(i<XT_CARD_SINGLE_SUIT_NU);i++)
		{
			face1=(face_1+i)%XT_CARD_SINGLE_SUIT_NU;
			if(m_cardMask[suit][face1])
			{
				continue;
			}


			face2=randomCardWithMark(0,XT_CARD_SINGLE_SUIT_NU,suit,1<<face1);
			if(face2<0)
			{
				break;
			}


			face3=randomCardWithMark(0,XT_CARD_SINGLE_SUIT_NU,suit,(1<<face1)|(1<<face2));
			if(face3<0)
			{
				break;
			}

			if(cardIsShunZi(face1,face2,face3));
			{
				int face4=randomCardWithMark(0,XT_CARD_SINGLE_SUIT_NU,suit,(1<<face1)|(1<<face2)|(1<<face3));
				if(face4<0)
				{
					break;
				}


				if(!cardIsShunZi(face2,face3,face4))
				{
					face1=face4;

					m_cardMask[suit][face1]=1;
					card->addCard(s_cards[suit][face1]);

					m_cardMask[suit][face2]=1;
					card->addCard(s_cards[suit][face2]);

					m_cardMask[suit][face3]=1;
					card->addCard(s_cards[suit][face3]);

					return 0;


				}

				if(!cardIsShunZi(face1,face3,face4))
				{
					face2=face4;
					

					m_cardMask[suit][face1]=1;
					card->addCard(s_cards[suit][face1]);

					m_cardMask[suit][face2]=1;
					card->addCard(s_cards[suit][face2]);

					m_cardMask[suit][face3]=1;
					card->addCard(s_cards[suit][face3]);

					return 0;
				}

				if(!cardIsShunZi(face1,face2,face4))
				{
					face3=face4;

					m_cardMask[suit][face1]=1;
					card->addCard(s_cards[suit][face1]);

					m_cardMask[suit][face2]=1;
					card->addCard(s_cards[suit][face2]);

					m_cardMask[suit][face3]=1;
					card->addCard(s_cards[suit][face3]);

					return 0;
				}
			}
		}
		suit=(suit+1)%XT_CARD_SUIT_NU;
	}

	return -1;

}


int XtTypeDeck::getShunZi(XtHoleCards* card)
{
	card->clear();
	int face=randomNum(0,XT_CARD_SINGLE_SUIT_NU);

	for(int i=0;i<XT_CARD_SINGLE_SUIT_NU;i++)
	{
		int ret=getShunZiByCard(card,face);
		if(ret==0)
		{
			return 0;
		}
		face=(face+1)%XT_CARD_SINGLE_SUIT_NU;
	}
	return -1;
}

int XtTypeDeck::getShunZiByCard(XtHoleCards* card,int face)
{
	if(face==0)
	{
		return -1;
	}

	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		if(m_cardMask[i][face-1])
		{
			continue;
		}

		for(int j=0;j<XT_CARD_SUIT_NU;j++)
		{
			if(i==j)
			{
				continue;
			}

			if(m_cardMask[j][face])
			{
				continue;
			}


			for(int k=0;k<XT_CARD_SUIT_NU;k++)
			{
				if(k==i||k==j)
				{
					continue;
				}

				if(m_cardMask[k][(face+1)%XT_CARD_SINGLE_SUIT_NU])
				{
					continue;
				}

				m_cardMask[i][face-1]=1;
				card->addCard(s_cards[i][face-1]);

				m_cardMask[j][face]=1;
				card->addCard(s_cards[j][face]);

				m_cardMask[k][(face+1)%XT_CARD_SINGLE_SUIT_NU]=1;
				card->addCard(s_cards[k][(face+1)%XT_CARD_SINGLE_SUIT_NU]);
				return 0;
			}
		}
	}


	return -1;

}



int XtTypeDeck::getDuiZhi(XtHoleCards* card)
{
	card->clear();

	int face1=randomNum(0,XT_CARD_SINGLE_SUIT_NU);

	for(int i=0;i<XT_CARD_SINGLE_SUIT_NU;i++)
	{
		int ret=getDuiZhiByCard(card,face1);
		face1=(face1+1)%XT_CARD_SINGLE_SUIT_NU;

		if(ret==0)
		{
			return 0;
		}
	}

	return -1;
}

int XtTypeDeck::getDuiZhiByCard(XtHoleCards* card,int face)
{
	vector<int> suits;

	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		if(!m_cardMask[i][face])
		{
			suits.push_back(i);
		}
	}

	if(suits.size()<2)
	{
		return -1;
	}

	std::random_shuffle(suits.begin(),suits.end());


	int face2= randomNum(0,XT_CARD_SINGLE_SUIT_NU);

	for(int i=0;i<XT_CARD_SINGLE_SUIT_NU;i++)
	{
		if(face2==face)
		{
			face2=(face2+1)%XT_CARD_SINGLE_SUIT_NU;
			continue;
		}

		int face2_suit=randomNum(0,XT_CARD_SUIT_NU);

		for(int j=0;j<XT_CARD_SUIT_NU;j++)
		{

			if(m_cardMask[(face2_suit+j)%XT_CARD_SUIT_NU][face2])
			{
				continue;
			}

			m_cardMask[suits[0]][face]=1;
			m_cardMask[suits[1]][face]=1;
			m_cardMask[(face2_suit+j)%XT_CARD_SUIT_NU][face2]=1;

			card->addCard(s_cards[suits[0]][face]);
			card->addCard(s_cards[suits[1]][face]);
			card->addCard(s_cards[(face2_suit+j)%XT_CARD_SUIT_NU][face2]);

			return 0;
		}

		face2=(face2+1)%XT_CARD_SINGLE_SUIT_NU;
	}

	return -1;
}


int XtTypeDeck::getGaoPai(XtHoleCards* card)
{
	card->clear();

	int suit_1=randomNum(0,XT_CARD_SUIT_NU);
	int suit_2=randomNum(0,XT_CARD_SUIT_NU);
	int suit_3=randomNum(0,XT_CARD_SUIT_NU);


	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		int suit_i=(i+suit_1)%XT_CARD_SUIT_NU;
		int face1=randomCard(0,XT_CARD_SINGLE_SUIT_NU,suit_i);

		if(face1<0)
		{
			continue;
		}

		for(int j=0;j<XT_CARD_SUIT_NU;j++)
		{
			int suit_j=(j+suit_2)%XT_CARD_SUIT_NU;
			int face2=randomCardWithMark(0,XT_CARD_SINGLE_SUIT_NU,suit_j,1<<face1);

			if(face2<0)
			{
				continue;
			}


			for(int k=0;k<XT_CARD_SUIT_NU;k++)
			{
				int suit_k=(k+suit_3)%XT_CARD_SUIT_NU;

				if(suit_k==suit_i&&suit_k==suit_j)
				{
					continue;
				}


				int face3=randomCardWithMark(0,XT_CARD_SINGLE_SUIT_NU,(suit_k)%XT_CARD_SUIT_NU,(1<<face1)|(1<<face2));
				if(face3<0)
				{
					continue;
				}

				if(cardIsShunZi(face1,face2,face3))
				{
					int face4=randomCardWithMark(0,XT_CARD_SINGLE_SUIT_NU,(suit_k)%XT_CARD_SUIT_NU,(1<<face1) | (1<<face2) |(1<<face3));
					//printf("face4 is %d\n",face4);

					if(face4<0)
					{
						continue;
					}

					if(cardIsShunZi(face1,face2,face4))
					{
						int face5=randomCardWithMark(0,XT_CARD_SINGLE_SUIT_NU,(suit_k)%XT_CARD_SUIT_NU,(1<<face1) | (1<<face2) |(1<<face3)|(1<<face4));

						//	printf("face5 is %d\n",face5);
						if(face5<0)
						{
							continue;
						}

						face3=face5;
					}
					else 
					{
						face3=face4;
					}
				}




				m_cardMask[suit_i][face1]=1;
				m_cardMask[suit_j][face2]=1;
				m_cardMask[suit_k][face3]=1;


				card->addCard(s_cards[suit_i][face1]);
				card->addCard(s_cards[suit_j][face2]);
				card->addCard(s_cards[suit_k][face3]);

				return 0;
			}
		}
	}
	return -1;
}


int XtTypeDeck::pop(XtCard* card)
{
	int suit=randomNum(0,XT_CARD_SUIT_NU);
	int face=randomNum(0,XT_CARD_SINGLE_SUIT_NU);


	for(int i=0;i<XT_CARD_SUIT_NU;i++)
	{
		int suit_i=(suit+i)%XT_CARD_SUIT_NU;

		for(int j=0;j<XT_CARD_SINGLE_SUIT_NU;j++)
		{
			int face_j=(face+j)%XT_CARD_SINGLE_SUIT_NU;

			if(m_cardMask[suit_i][face_j])
			{
				continue;
			}

			card->setValue(s_cards[suit_i][face_j]);

			m_cardMask[suit_i][face_j]=1;

			return 0;
		}
	}

	return -1;

}

class sortCardListFunc
{
	public:
		int operator() (XtHoleCards l, XtHoleCards r)
		{
			return l.compare(r) > 0;
		}
};

void XtTypeDeck::getSortHoleCardList(XtHoleCards cardlist[], unsigned int num)
{
    for(unsigned int i = 0; i < num; ++i)
    {
        getHoleCards(&cardlist[i]);   
        cardlist[i].analysis();
    }
    
    std::sort(cardlist, cardlist + num, sortCardListFunc());
}


