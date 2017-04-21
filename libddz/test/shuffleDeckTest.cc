#include "XtShuffleDeck.h"
#include "XtHoleCards.h"
#include "table.h"

static int timeindex = 0;

void testShuttle(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard[] = {XtCard(0x04), XtCard(0x35), XtCard(0x26), XtCard(0x0B), XtCard(0x3B), XtCard(0x2B), XtCard(0x1B), XtCard(0x1C), XtCard(0x0C), XtCard(0x2C), XtCard(0x3C)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    //vector<XtCard> cards;
    //deck.getHoleCards(cards, 17);
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.isShuttle0(cards))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testAircraft(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard[] = {XtCard(0x0B), XtCard(0x3B), XtCard(0x2B), XtCard(0x1C), XtCard(0x0C), XtCard(0x2C)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    //vector<XtCard> cards;
    //deck.getHoleCards(cards, 17);
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.isAircraft0(cards))
    {
        printf("true!\n");
    }
    else
    {

        printf("false!\n");
    }
}

void test4and2(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard[] = {XtCard(0x01), XtCard(0x11), XtCard(0x2D), XtCard(0x3D), XtCard(0x3C), XtCard(0x1C), XtCard(0x0C), XtCard(0x2C)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.is4and22s(cards))
    {
        printf("true!\n");
    }
    else
    {

        printf("false!\n");
    }
}

void testDoubleStraight(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard[] = {XtCard(0x0A), XtCard(0x1A), XtCard(0x2A), XtCard(0x3A), XtCard(0x3B), XtCard(0x1B), XtCard(0x0C), XtCard(0x2C)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.isDoubleStraight(cards))
    {
        printf("true!\n");
    }
    else
    {

        printf("false!\n");
    }
}

void testStraight(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard[] = {XtCard(0x03), XtCard(0x14), XtCard(0x25), XtCard(0x36), XtCard(0x37), XtCard(0x18), XtCard(0x09), XtCard(0x29)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.isStraight(cards))
    {
        printf("true!\n");
    }
    else
    {

        printf("false!\n");
    }
}

void testThree(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard[] = { XtCard(0x19), XtCard(0x09), XtCard(0x29)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.isThree0(cards))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testPair(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard[] = { XtCard(0x00), XtCard(0x10) };
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.isPair(cards))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testCompareSingle(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard1[] = { XtCard(0x10)};
    vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    XtCard::sortByDescending(cards1);
    for(vector<XtCard>::const_iterator it = cards1.begin(); it != cards1.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    XtCard initCard2[] = { XtCard(0x00)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    XtCard::sortByDescending(cards2);
    for(vector<XtCard>::const_iterator it = cards2.begin(); it != cards2.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.compareSingle(cards1, cards2))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testCompareBomb(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard1[] = { XtCard(0x02), XtCard(0x12), XtCard(0x22), XtCard(0x32) };
    vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    XtCard::sortByDescending(cards1);
    for(vector<XtCard>::const_iterator it = cards1.begin(); it != cards1.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x33) };
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    XtCard::sortByDescending(cards2);
    for(vector<XtCard>::const_iterator it = cards2.begin(); it != cards2.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;


    if(deck.compareBomb(cards1, cards2))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testCompareShuttle(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard1[] = { XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x34), XtCard(0x05), XtCard(0x15), XtCard(0x25), XtCard(0x35) };
    vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    XtCard::sortByDescending(cards1);
    for(vector<XtCard>::const_iterator it = cards1.begin(); it != cards1.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x33), XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x34) };
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    XtCard::sortByDescending(cards2);
    for(vector<XtCard>::const_iterator it = cards2.begin(); it != cards2.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    //不检测牌型
    if(deck.compareShuttle(cards1, cards2))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testCompareAircraft(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard1[] = { XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x05), XtCard(0x15)};
    vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    XtCard::sortByDescending(cards1);
    for(vector<XtCard>::const_iterator it = cards1.begin(); it != cards1.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x04), XtCard(0x14)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    XtCard::sortByDescending(cards2);
    for(vector<XtCard>::const_iterator it = cards2.begin(); it != cards2.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    //不检测牌型
    if(deck.compareAircraft(cards1, cards2))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

static string printStr;
void show(const vector<XtCard>& card)
{
    printStr.clear();
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        //xt_log.debug("%s\n", it->getCardDescription());
        printStr.append(it->getCardDescriptionString());
        printStr.append(" ");
    }
    printf("%s\n", printStr.c_str());
}

void show(const set<int> card)
{
    for(set<int>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        printf("%d ", *it); 
    }
    printf("\n");
}

void testBigPair(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    //XtCard initCard1[] = { XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x05), XtCard(0x15), XtCard(0x06), XtCard(0x16)};
    //vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigPair(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testBigThree2s(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    //XtCard initCard1[] = { XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x05), XtCard(0x15), XtCard(0x06), XtCard(0x16)};
    //vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x04), XtCard(0x14)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigThree2s(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testBigThree1(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    /*
       XtCard initCard1[] = { XtCard(16), XtCard(0), XtCard(34), XtCard(49), XtCard(28), XtCard(27), XtCard(41), 
       XtCard(40), XtCard(24), XtCard(39), XtCard(6), XtCard(53), XtCard(5), XtCard(36), XtCard(35), XtCard(19), XtCard(3)};
       vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
       */
    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x04)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigThree1(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testBigThree0(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigThree0(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

bool testBigStraight(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x04), XtCard(0x25), XtCard(0x06), XtCard(0x07)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    deck.delCard(cards2, timeindex);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x18));
    cards1.push_back(XtCard(0x28));
    cards1.push_back(XtCard(0x19));
    cards1.push_back(XtCard(0x1A));
    cards1.push_back(XtCard(0x1B));
    cards1.push_back(XtCard(0x1C));
    deck.delCard(cards1, timeindex);
    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigStraight(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBigDoubleStraight(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x04), XtCard(0x24), XtCard(0x05), XtCard(0x15)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    deck.delCard(cards2, timeindex);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x14));
    cards1.push_back(XtCard(0x34));
    cards1.push_back(XtCard(0x25));
    cards1.push_back(XtCard(0x35));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x26));
    cards1.push_back(XtCard(0x07));
    cards1.push_back(XtCard(0x17));
    deck.delCard(cards1, timeindex);
    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigDoubleStraight(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBig4and24(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x33), XtCard(0x05), XtCard(0x15), XtCard(0x06), XtCard(0x16)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    deck.delCard(cards2, timeindex);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x04));
    cards1.push_back(XtCard(0x14));
    cards1.push_back(XtCard(0x24));
    cards1.push_back(XtCard(0x34));
    cards1.push_back(XtCard(0x07));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x27));
    cards1.push_back(XtCard(0x37));
    cards1.push_back(XtCard(0x08));
    cards1.push_back(XtCard(0x18));
    cards1.push_back(XtCard(0x09));
    cards1.push_back(XtCard(0x19));
    deck.delCard(cards1, timeindex);
    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.big4and24(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBig4and22d(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x33), XtCard(0x05), XtCard(0x06)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    deck.delCard(cards2, timeindex);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x04));
    cards1.push_back(XtCard(0x14));
    cards1.push_back(XtCard(0x24));
    cards1.push_back(XtCard(0x34));
    cards1.push_back(XtCard(0x07));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x27));
    cards1.push_back(XtCard(0x37));
    cards1.push_back(XtCard(0x08));
    cards1.push_back(XtCard(0x09));
    deck.delCard(cards1, timeindex);
    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.big4and22d(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBig4and22s(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x33), XtCard(0x05), XtCard(0x15)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    deck.delCard(cards2, timeindex);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x04));
    cards1.push_back(XtCard(0x14));
    cards1.push_back(XtCard(0x24));
    cards1.push_back(XtCard(0x34));
    cards1.push_back(XtCard(0x07));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x27));
    cards1.push_back(XtCard(0x37));
    cards1.push_back(XtCard(0x08));
    cards1.push_back(XtCard(0x18));
    deck.delCard(cards1, timeindex);
    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.big4and22s(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBigAircraft2s(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x25));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x26));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x27));
    cards1.push_back(XtCard(0x37));
    cards1.push_back(XtCard(0x18));
    cards1.push_back(XtCard(0x28));
    cards1.push_back(XtCard(0x1A));
    cards1.push_back(XtCard(0x2A));

    deck.delCard(cards1, timeindex);
    //XtCard initCard1[] = { XtCard(0x02), XtCard(0x12), XtCard(0x22), XtCard(0x32) };
    //vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));


    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23),  XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x12), XtCard(0x22), XtCard(0x19), XtCard(0x29)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    deck.delCard(cards2, timeindex);

    deck.getHoleCards(cards1, 7);

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigAircraft2s(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBigAircraft1(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x25));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x26));
    cards1.push_back(XtCard(0x07));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x27));
    cards1.push_back(XtCard(0x2A));
    cards1.push_back(XtCard(0x2B));

    deck.delCard(cards1, timeindex);
    //XtCard initCard1[] = { XtCard(0x02), XtCard(0x12), XtCard(0x22), XtCard(0x32) };
    //vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));


    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23),  XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x12),  XtCard(0x19)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    deck.delCard(cards2, timeindex);

    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigAircraft1(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBigAircraft0(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x25));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x26));
    cards1.push_back(XtCard(0x07));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x27));
    deck.delCard(cards1, timeindex);


    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23),  XtCard(0x04), XtCard(0x14), XtCard(0x24)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    deck.delCard(cards2, timeindex);

    //deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigAircraft0(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBigShuttle2(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x25));
    cards1.push_back(XtCard(0x35));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x26));
    cards1.push_back(XtCard(0x36));
    //cards1.push_back(XtCard(0x22));
    //cards1.push_back(XtCard(0x39));
    //cards1.push_back(XtCard(0x3A));
    //cards1.push_back(XtCard(0x3B));

    deck.delCard(cards1, timeindex);
    //XtCard initCard1[] = { XtCard(0x02), XtCard(0x12), XtCard(0x22), XtCard(0x32) };
    //vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));


    XtCard initCard2[] = { 
        XtCard(0x03), 
        XtCard(0x13), 
        XtCard(0x23),  
        XtCard(0x33),  
        XtCard(0x04), 
        XtCard(0x14), 
        XtCard(0x24),
        XtCard(0x34),
        XtCard(0x27),
        XtCard(0x2A),
        XtCard(0x2B),
        XtCard(0x37)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    deck.delCard(cards2, timeindex);

    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigShuttle2(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBigShuttle0(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x25));
    //cards1.push_back(XtCard(0x35));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x26));
    //cards1.push_back(XtCard(0x36));

    deck.delCard(cards1, timeindex);

    XtCard initCard2[] = { 
        XtCard(0x03), 
        XtCard(0x13), 
        XtCard(0x23),  
        XtCard(0x33),  
        XtCard(0x04), 
        XtCard(0x14), 
        XtCard(0x24),
        XtCard(0x34)
    };
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    deck.delCard(cards2, timeindex);

    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigShuttle0(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testBigBomb(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x25));
    cards1.push_back(XtCard(0x35));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x26));
    cards1.push_back(XtCard(0x36));

    deck.delCard(cards1, timeindex);

    XtCard initCard2[] = { 
        XtCard(0x03), 
        XtCard(0x13), 
        XtCard(0x23),  
        XtCard(0x33),  
    };
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    deck.delCard(cards2, timeindex);

    deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigBomb(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
        return true;
    }
    else
    {
        printf("false!\n");
        return false;
    }
}

bool testGetOut(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x10));
    deck.delCard(cards1, timeindex);

    XtCard initCard2[] = { 
        XtCard(0x00), 
    };
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));

    deck.delCard(cards2, timeindex);

    //deck.getHoleCards(cards1, 17 - cards1.size());

    XtCard::sortByDescending(cards1);
    XtCard::sortByDescending(cards2);

    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.getOut(cards1, cards2, result))
    {
        printf("true!\n");
        show(result);
        return true;
    }
    else
    {
        printf("false!\n");
        show(result);
        return false;
    }
}

int testGetBottomDouble(void)
{
    XtShuffleDeck m_deck;
    vector<XtCard> m_bottomCard;
    m_bottomCard.push_back(XtCard(0x05));
    m_bottomCard.push_back(XtCard(0x15));
    m_bottomCard.push_back(XtCard(0x25));
    XtCard::sortByDescending(m_bottomCard);
    show(m_bottomCard);

    bool littleJoke = false;
    bool bigJoke = false;
    set<int> suitlist; 
    set<int> facelist; 
    bool isContinue = m_deck.isNContinue(m_bottomCard, 1);
    for(vector<XtCard>::const_iterator it = m_bottomCard.begin(); it != m_bottomCard.end(); ++it)
    {
        if(it->m_value == 0x00) 
        {
            littleJoke = true;
        }
        else if(it->m_value == 0x10) 
        {
            bigJoke = true; 
        }
        suitlist.insert(it->m_suit);
        facelist.insert(it->m_face);
    }

    //火箭
    if(bigJoke && littleJoke)
    {        
        printf("火箭");
        return 4;
    }

    //大王
    if(bigJoke && !littleJoke)
    {
        printf("大王");
        return 2;
    }

    //小王
    if(!bigJoke && littleJoke)
    {
        printf("小王");
        return 2;
    }

    //同花
    if(!isContinue && suitlist.size() == 1)
    {
        printf("同花");
        return 3; 
    }

    //顺子
    if(isContinue && suitlist.size() != 1)
    {
        printf("顺子");
        return 3; 
    }

    //同花顺
    if(isContinue && suitlist.size() == 1)
    {
        printf("同花顺");
        return 4; 
    }

    //三同
    if(facelist.size() == 1)
    {
        printf("三同");
        return 4; 
    }
    return 0;
}

bool testCreateCard(void)
{
    XtShuffleDeck m_deck;
    m_deck.fill();
    m_deck.shuffle(timeindex++);

    unsigned int SEAT_NUM = 3;
    unsigned int BOTTON_CARD_NUM = 3;
    unsigned int HAND_CARD_NUM = 17;
    std::vector<XtCard>         m_bottomCard;                   //底牌
    XtHoleCards                 m_seatCard[SEAT_NUM];           //座位手牌
    m_bottomCard.clear();
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        m_seatCard[i].reset();
    }

    //底牌    
    if(!m_deck.getHoleCards(m_bottomCard, BOTTON_CARD_NUM))
    {
        return false;
    }
    //show(m_bottomCard);

    //手牌
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(!m_deck.getHoleCards(m_seatCard[i].m_cards, HAND_CARD_NUM))
        {
            return false;
        }
        //show(m_seatCard[i].m_cards);
    }

    if(m_deck.count() != 0)
    {
        return false;
    }

    set<int> settest;
    for(vector<XtCard>::const_iterator it = m_bottomCard.begin(); it != m_bottomCard.end(); ++it)
    {
        if(settest.find(it->m_value) == settest.end()) 
        {
            settest.insert(it->m_value);
        }
        else
        {
            printf("error card: m_face:%d, m_suit:%d\n", it->m_face, it->m_suit);
            return false; 
        }
    }

    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        for(vector<XtCard>::const_iterator it = m_seatCard[i].m_cards.begin(); it != m_seatCard[i].m_cards.end(); ++it)
        {
            if(settest.find(it->m_value) == settest.end()) 
            {
                settest.insert(it->m_value);
            }
            else
            {
                printf("error card: m_face:%d, m_suit:%d\n", it->m_face, it->m_suit);
                return false; 
            }
        }
    }

    if(settest.size() != 54)
    {
        printf("size:%d\n", static_cast<int>(settest.size()));
        return false;
    }

    return true;
}

bool testGetFirst(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);

    XtCard::sortByDescending(cards1);

    vector<XtCard> result;
    deck.getFirst(cards1, result);
    XtCard::sortByDescending(result);

    if(deck.getCardType(result) == CT_ERROR)
    {
        show(cards1);
        show(result);
        printf("card type error\n");
        return false;
    }
    else
    {
        return true;
    }
}

void testDivideCard3(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x25));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x26));
    cards1.push_back(XtCard(0x08));
    cards1.push_back(XtCard(0x18));
    cards1.push_back(XtCard(0x28));
    cards1.push_back(XtCard(0x0B));
    cards1.push_back(XtCard(0x1B));
    cards1.push_back(XtCard(0x2B));
    cards1.push_back(XtCard(0x0A));
    cards1.push_back(XtCard(0x1A));
    cards1.push_back(XtCard(0x2A));

    XtCard::sortByDescending(cards1);

    vector<XtCard> pure3;
    vector<XtCard> aircraft;
    deck.divideCard3(cards1, pure3, aircraft);

    show(cards1);
    show(pure3);
    show(aircraft);
}

void testDivideCard2(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x15));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x16));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x37));
    cards1.push_back(XtCard(0x19));
    cards1.push_back(XtCard(0x39));
    cards1.push_back(XtCard(0x00));
    cards1.push_back(XtCard(0x10));

    XtCard::sortByDescending(cards1);

    vector<XtCard> rocket;
    vector<XtCard> ds;
    vector<XtCard> pure2;

    deck.divideCard2(cards1, rocket, pure2, ds);
    show(cards1);
    show(rocket);
    show(pure2);
    show(ds);
}

void testDivideCard1(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x17));
    cards1.push_back(XtCard(0x18));
    cards1.push_back(XtCard(0x19));
    cards1.push_back(XtCard(0x1B));
    cards1.push_back(XtCard(0x00));

    XtCard::sortByDescending(cards1);

    vector<XtCard> jocker;
    vector<XtCard> straight;
    vector<XtCard> pure1;

    deck.divideCard1(cards1, jocker, pure1, straight);
    show(cards1);
    show(jocker);
    show(pure1);
    show(straight);
}

void testGetNContinue(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    cards1.push_back(XtCard(0x02));
    cards1.push_back(XtCard(0x03));
    cards1.push_back(XtCard(0x05));
    cards1.push_back(XtCard(0x06));
    cards1.push_back(XtCard(0x0A));

    XtCard::sortByDescending(cards1);

    set<int> result;
    deck.getNcontinue(cards1, 2, result);

    show(cards1);
    show(result);
}

const char* g_ctDesc[] = 
{ 
    "DT_4",            
    "DT_3",            
    "DT_2",            
    "DT_1",            
    "DT_ROCKET",            
    "DT_JOCKER",
    "DT_STRAITHT",
    "DT_DS",
    "DT_AIRCRAFT",
};
void testDivideCard(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(timeindex++);

    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    XtCard::sortByDescending(cards1);

    map<int, vector<XtCard> > result;
    deck.divideCard(cards1, result);

    show(cards1);
    printf("size:%d\n", cards1.size());

    int size = 0;
    for(map<int, vector<XtCard> >::const_iterator it = result.begin(); it != result.end(); ++it)
    {
        printf("%s\n", g_ctDesc[it->first]);
        show(it->second);
        size += it->second.size();
    }
    printf("size:%d\n", size);
}

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
    //testAircraft();
    //testDoubleStraight();
    //testStraight();
    //testPair();
    //testCompareBomb();
    //testCompareShuttle();
    //testCompareAircraft();
    //testCompareSingle();
    //testBigPair();
    //testBigThree2s();
    //testBigThree1();
    //testBigThree0();
    //testGetOut();
    //testGetBottomDouble();
    //testGetFirst();
    //testDivideCard3();
    //testDivideCard2();
    //testDivideCard1();
    testDivideCard();
    //testGetNContinue();
    //while(1)
    {
        //if(testBigStraight()) { break; };
        //if(testBigDoubleStraight()) { break; }
        //if(testBig4and24()) { break; }
        //if(testBig4and22d()) { break; }
        //if(testBig4and22s()) { break; }
        //if(testBigAircraft2s()) { break; }
        //if(testBigAircraft1()) { break; }
        //if(testBigAircraft0()) { break; }
        //if(testBigShuttle2()) { break; }
        //if(testBigShuttle0()) { break; }
        //if(testBigBomb()) { break; }
        //if(testGetOut()) { break; }
        //if(!testCreateCard()) { break; }
        //if(!testGetFirst()) { break; }
    }
    return 0;
}

