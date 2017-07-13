#include "XtTypeDeck.h"

int main()
{

	int time_i=0;
	int time_j=0;

	XtTypeDeck deck;


	//int weight[]= {520,480,10960,7200,37440,164000};
	int weight[]= {520,480,10960,7200,37440,164000};

	deck.setTypeWeight(weight);


	while(true)
	{


		printf("\n\nBegin FaPai\n");


		srand(time_i);
		deck.fill();
		XtHoleCards left_card;

		int cards[7]={0,0,0,0,0,0,0};



		std::map<int,int> card_used;

		int j=0;
		while(true)
		{
			int ret=deck.getHoleCards(&left_card);

			if(ret==-1||j==5)
			{

				printf("\tcard_used is: ");
				std::map<int,int>::iterator iter;
				for(iter=card_used.begin();iter!=card_used.end();++iter)
				{
					int card_value=iter->first;
					XtCard c(card_value);
					printf("%s ",c.getCardDescription().c_str());
				}

				printf("\n");


				printf("\tBaozhi  Value is %d\n",cards[1]);
				printf("\tShunJin Value is %d\n",cards[2]);
				printf("\tJinHua  Value is %d\n",cards[3]);
				printf("\tShunZhi Value is %d\n",cards[4]);
				printf("\tDunZhi  Value is %d\n",cards[5]);
				printf("\tGaoPai  Value is %d\n",cards[6]);

				printf("\tMax FaPai is [%d] \n",j);
				printf("\tSrand is %d\n",time_i);

				break;
			}
			j++;



			for(unsigned int j=0;j<left_card.m_cards.size();j++)
			{
				int card_value=left_card.m_cards[j].m_value;
				if(card_used.find(card_value) != card_used.end())
				{
					printf("\tCard Error Double Card,(f %d,s %d) \n",left_card.m_cards[j].m_face,left_card.m_cards[j].m_suit);
					exit(0);
				}
				else 
				{
					card_used[card_value]=1;
				}
			}


			cards[ret]++;


		}


		printf("End FaPai\n");
		if(time_i%100000==0)
		{
			time_j++;
			printf("Testing(%d)...\n\n\n",time_j);
		}
		time_i++;
	}

	return 0;
}

