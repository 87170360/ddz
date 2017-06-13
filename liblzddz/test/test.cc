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

const char* g_dtDesc[] = 
{ 
    "DT_4",            
    "DT_3",            
    "DT_2",            
    "DT_1",            
    "DT_ROCKET",            
    "DT_STRAITHT",
    "DT_DS",
    "DT_AIRCRAFT",
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
        show(it->second, g_dtDesc[it->first]);
        size += it->second.size();
    }
    printf("size:%d\n", size);
}

const char* g_ctDesc[] = 
{ 
	"CT_ERROR",			// 错误类型
	"CT_ROCKET",			// 火箭
	"CT_BOMB",			// 炸弹
	"CT_SHUTTLE_0",			// 航天飞机0翼
	"CT_SHUTTLE_2",			// 航天飞机2翼(同或异)
	"CT_AIRCRAFT_0",			// 飞机0翼
	"CT_AIRCRAFT_1",			// 飞机1翼
	"CT_AIRCRAFT_2S",			// 飞机2同翼
	"CT_4AND2_2S",			// 四带二2同翼
	"CT_4AND2_2D",			// 四带二2异翼
	"CT_4AND2_4",			// 四带二4翼(2对)
	"CT_DOUBLE_STRAIGHT",			// 双顺 
	"CT_STRAIGHT",			// 单顺 
	"CT_THREE_0",			// 三带0翼
	"CT_THREE_1",			// 三带1翼
	"CT_THREE_2S",			// 三带2同翼
	"CT_PAIR",			// 对子
	"CT_SINGLE",			// 单牌
};

void testGetCardType(void)
{
    g_deck.shuffle(timeindex++);
    vector<Card> holdcard;
    g_deck.getHoldCard(holdcard, 20);
    show(holdcard, "holdcard:");
    printf("size:%d\n", (int)holdcard.size());

    Card::sortByDescending(holdcard);
    map<int, vector<Card> > result;
    g_deck.divide(holdcard, result);

    int cardtype =  CT_ERROR;
    for(map<int, vector<Card> >::const_iterator it = result.begin(); it != result.end(); ++it)
    {
        if(!it->second.empty())
        {
            show(it->second, g_dtDesc[it->first]);
            cardtype = g_deck.getCardType(it->second);
            printf("cardtype:%s\n", g_ctDesc[cardtype]);
        }
    }
}

void testChangeCard(void)
{
    g_deck.shuffle(timeindex++);
    vector<Card> holdcard;
    g_deck.getHoldCard(holdcard, 20);
    show(holdcard, "holdcard:");

    vector<Card> lzcard;
    lzcard.push_back(g_deck.getLZ());
    show(lzcard, "lzcard:");

    vector<int> lzface;
    lzface.push_back(3);
    lzface.push_back(3);

    g_deck.changeCard(holdcard, lzface);
    show(holdcard, "changecard:");
}

void testCompare(void)
{
    g_deck.shuffle(timeindex++);
    vector<Card> holdcard1;
    holdcard1.push_back(Card(0x34));
    holdcard1.push_back(Card(0x14));
    show(holdcard1, "holdcard1:");

    vector<Card> holdcard2;
    holdcard2.push_back(Card(0x33));
    holdcard2.push_back(Card(0x13));
    show(holdcard2, "holdcard2:");

    bool isBig = g_deck.compare(holdcard1, holdcard2);
    printf("isBig:%s\n", isBig ? "true" : "false");
}

void testGetLZFollow(void)
{
    g_deck.shuffle(timeindex++);
    g_deck.setLZ(15);

    vector<Card> holdcard1;
    holdcard1.push_back(Card(0x02));
    holdcard1.push_back(Card(0x1B));
    holdcard1.push_back(Card(0x2B));
    holdcard1.push_back(Card(0x0A));
    holdcard1.push_back(Card(0x1A));

    Card::sortByDescending(holdcard1);
    show(holdcard1, "mine:");

    vector<Card> holdcard2;
    holdcard2.push_back(Card(0x33));
    holdcard2.push_back(Card(0x23));
    holdcard2.push_back(Card(0x13));
    holdcard2.push_back(Card(0x04));
    holdcard2.push_back(Card(0x14));
    show(holdcard2, "other:");
    Card::sortByDescending(holdcard2);

    vector<Card> result;
    vector<Card> change;
    g_deck.getLZFollow(holdcard1, holdcard2, result, change);
    show(result, "result");
    show(change, "change");
}

void testChangeRecover(void)
{
    vector<Card> holdcard1;
    holdcard1.push_back(Card(0x02));
    show(holdcard1, "origin:");

    holdcard1[0].changeFace(3);
    show(holdcard1, "change:");

    holdcard1[0].recover();
    show(holdcard1, "recover:");
}

/*
	"d", "c", "h", "s"

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
 */

int main()
{
    //testShuffle();
    //testGetHoldCard();
    //testDivide();
    //testGetCardType();
    //testChangeCard();
    //testCompare();
    testGetLZFollow();
    //testChangeRecover();
    return 0;
}

