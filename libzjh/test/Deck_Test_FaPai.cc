#include "XtTypeDeck.h"



int main()
{

	int time_i=0;
	int time_j=0;

	XtTypeDeck deck;

	while(true)
	{
		XtHoleCards left_card;

		srand(time_i);
		deck.fill();

		std::map<int,int> card_used;


		printf("Begin FaPai\n");

		int i=0;
		while(true)
		{
			int c_type=rand()%6+1;

			int ret=deck.getHoleCards(&left_card,c_type);


			if(ret<0)
			{
				printf("Max Type of Card: %d End With(%d),Rand(%d)\n",i,c_type,time_i);
				break;
			}


			left_card.analysis();

			if(c_type!=left_card.getCardType())
			{
				printf("card_type Error\n");
				printf("card_type1=%d,%d\n",c_type,left_card.getCardType());
				left_card.debug();
				exit(0);
			}




			if(left_card.m_cards.size()!=3)
			{
				printf("Left Card_SizeError %d",(int)left_card.m_cards.size());
				exit(0);
			}

			for(unsigned int j=0;j<left_card.m_cards.size();j++)
			{
				int card_value=left_card.m_cards[j].m_value;
				if(card_used.find(card_value) != card_used.end())
				{
					printf("Card Error Duble Card,(f %d,s %d) \n",left_card.m_cards[j].m_face,left_card.m_cards[j].m_suit);
					exit(0);
				}
				else 
				{
					card_used[card_value]=1;
				}
			}

			//left_card.debug();
			i++;

		}

		printf("card_used is: ");
		std::map<int,int>::iterator iter;
		for(iter=card_used.begin();iter!=card_used.end();++iter)
		{

			int card_value=iter->first;
			XtCard c(card_value);
			printf("%s ",c.getCardDescription().c_str());
		}

		printf("\n");



		printf("End FaPai\n");
		if(time_i%100000==0)
		{
			time_j++;
			printf("Testing(%d)...\n",time_j);
		}
		time_i++;
	}

	return 0;
}

