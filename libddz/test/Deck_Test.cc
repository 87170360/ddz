#include "XtTypeDeck.h"

int main()
{

	int time_i=0;
	int time_j=0;
	XtTypeDeck deck;
	while(true)
	{
		XtHoleCards left_card;
		XtHoleCards right_card;

		deck.fill();


		int card_type1=deck.getHoleCards(&left_card);
		int card_type2=deck.getHoleCards(&right_card);

		left_card.analysis();
		right_card.analysis();

		if(card_type1!=left_card.getCardType())
		{
			printf("card_type Error\n");
			printf("card_type1=%d,%d\n",card_type1,left_card.getCardType());
			left_card.debug();

			exit(0);
		}

		if(card_type2!=right_card.getCardType())
		{
			printf("card_type Error\n");
			printf("card_type2=%d,%d\n",card_type1,right_card.getCardType());
			right_card.debug();

			exit(0);
		}

		if(left_card.m_cards.size()!=3)
		{
			printf("Left Card_SizeError %d",(int)left_card.m_cards.size());
			exit(0);
		}

		if(right_card.m_cards.size()!=3)
		{
			printf("Right Card_SizeError %d",(int)right_card.m_cards.size());
			exit(0);
		}


		for(unsigned int i=0;i<left_card.m_cards.size();i++)
		{
			for(unsigned int j=0;j<right_card.m_cards.size();j++)
			{
				if(left_card.m_cards[i].m_value==right_card.m_cards[j].m_value)
				{
					printf("Card Value Error\n");
					printf("card_type1=%d,%d\n",card_type1,left_card.getCardType());
					left_card.debug();

					printf("card_type Error\n");
					printf("card_type2=%d,%d\n",card_type1,right_card.getCardType());
					right_card.debug();
					exit(0);
				}

			}
		}

		if(time_i>100000)
		{
			time_i=0;
			time_j++;
			printf("Testing(%d)...\n",time_j);
		}
		time_i++;
	}

	return 0;
}

