#include "XtShuffleDeck.h"
#include "XtHoleCards.h"

int main()
{
	XtShuffleDeck deck;
    deck.fill();
    //deck.shuffle(0);
    //deck.showCards();
    //printf("count:%d\n", deck.count());
    
    vector<XtCard> bottom;
    deck.getHoleCards(bottom, 3);
    for(vector<XtCard>::const_iterator it = bottom.begin(); it != bottom.end(); ++it)
    {
        printf("%s", it->getCardDescription()); 
    }

    //vector<XtCard> hand;

	return 0;
}

