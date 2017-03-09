#include "XtShuffleDeck.h"
#include "XtHoleCards.h"
#include "table.h"

int main()
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);
    printf("count:%d\n", deck.count());

    XtCard card1(0x00);
    XtCard card2(0x10);
    
    printf("card1:%s\n", card1.getCardDescription());
    printf("card2:%s\n", card2.getCardDescription());
    vector<XtCard> cards;
    cards.push_back(card1);
    cards.push_back(card2);
    if(deck.isRocket(cards))
    {
        printf("is Rocket!\n");
    }
    else
    {
    
        printf("not Rocket!\n");
    }

    return 0;
}

