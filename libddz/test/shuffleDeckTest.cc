#include "XtShuffleDeck.h"
#include "XtHoleCards.h"
#include "table.h"
/*
static int card_arr[] = {
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
};
 */
int main()
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    //XtCard initCard[] = { XtCard(0x10), XtCard(0x32), XtCard(0x02) };
    //vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    vector<XtCard> cards;
    deck.getHoleCards(cards, 17);
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    
    map<int, int> result;
    deck.analyze(result, cards);
    for(map<int, int>::const_iterator it = result.begin(); it != result.end(); ++it)
    {
        printf("type:%d, num:%d\n", it->first, it->second); 
    }

    vector<XtCard> four;
    deck.keep4(four, cards);
    for(vector<XtCard>::const_iterator it = four.begin(); it != four.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }


    if(deck.isShuttle(cards))
    {
        //printf("true!\n");
    }
    else
    {
    
        //printf("false!\n");
    }
    return 0;
}

