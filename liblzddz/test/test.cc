#include "shuffledeck.h"
#include "holdcard.h"

static int timeindex = 0;
Shuffledeck g_deck;

static string printStr;
void show(const vector<Card>& card, const char* desc = NULL)
{
    printf("%s\n", desc);
    printStr.clear();
    for(vector<Card>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        printStr.append(it->getCardDescriptionString());
        printStr.append(" ");
    }
    printf("   %s\n", printStr.c_str());
}

void testShuffle(void)
{
    g_deck.shuffle(timeindex++);
    show(g_deck.getCard(), "deck cards:");
}

void testGetHoldCard(void)
{
    g_deck.shuffle(timeindex++);
    vector<Card> holdcard;
    g_deck.getHoldCard(holdcard, 20);
    show(holdcard, "holdcard:");
    printf("size:%d\n", (int)holdcard.size());
}

const char* g_ctDesc[] = 
{ 
    "DT_4",            
    "DT_3",            
    "DT_2",            
    "DT_1",            
    "DT_ROCKET",            
    "DT_STRAITHT",
    "DT_DS",
    "DT_AIRCRAFT",
    "DT_LZ",
};

void testDivide(void)
{
    g_deck.shuffle(timeindex++);
    vector<Card> holdcard;
    g_deck.getHoldCard(holdcard, 20);
    show(holdcard, "holdcard:");
    printf("size:%d\n", (int)holdcard.size());

    Card::sortByDescending(holdcard);
    map<int, vector<Card> > result;
    g_deck.divide(holdcard, result);

    int size = 0;
    for(map<int, vector<Card> >::const_iterator it = result.begin(); it != result.end(); ++it)
    {
        show(it->second, g_ctDesc[it->first]);
        size += it->second.size();
    }
    printf("size:%d\n", size);
}

int main()
{
    //testShuffle();
    //testGetHoldCard();
    testDivide();
    return 0;
}

