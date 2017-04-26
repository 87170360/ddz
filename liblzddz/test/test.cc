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


int main()
{
    //testShuffle();
    //testGetHoldCard();
    //testDivide();
    //testGetCardType();
    return 0;
}

