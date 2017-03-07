#include "XtShuffleDeck.h"
#include "XtHoleCards.h"

int main()
{
	XtShuffleDeck deck;
    deck.fill();
    //deck.shuffle(0);
    deck.showCards();
    printf("count:%d\n", deck.count());
	return 0;
}

