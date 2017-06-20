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

void split(const std::string& s, const std::string& delim, std::vector<std::string>& ret)  
{  
    size_t last = 0;  
    size_t index=s.find_first_of(delim,last);  
    while (index!=std::string::npos)  
    {  
        ret.push_back(s.substr(last,index-last));  
        last=index+1;  
        index=s.find_first_of(delim,last);  
    }  
    if (index-last>0)  
    {  
        ret.push_back(s.substr(last,index-last));  
    }  
}  

void getCardByDesc(vector<Card>& result, string desc)
{
    //printf("string :\n   %s\n", desc.c_str());
    g_deck.shuffle(timeindex++);

    vector<Card> all;
    map<std::string, Card> index;
    map<std::string, Card>::const_iterator index_it;

    g_deck.getHoldCard(all, 54);
    for(vector<Card>::const_iterator it = all.begin(); it != all.end(); ++it)
    {
        index[it->getCardDescriptionString()] = *it;
    }
    //printf("%ld\n", index.size()); 

    vector<std::string> name;    
    split(desc, " ", name);
    for(vector<std::string>::const_iterator it = name.begin(); it != name.end(); ++it)
    {
        //printf("%s\n", it->c_str()); 
        index_it = index.find(*it);
        if(index_it != index.end())
        {
            result.push_back(index_it->second);
        }
        else
        {
            printf("not find %s\n", it->c_str()); 
        }
    }
    //printf("%ld\n", name.size()); 
}

void testGetCardByDesc(void)
{
    vector<Card> result;
    getCardByDesc(result, "JOKERc 2s 2h 2c 2d Ac Ks Kc Qh Qd Jh 10h 9d 7h 7d 6s 6d 5s 5h 4s");
    show(result, "result");
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
    g_deck.setLZ(15);
    vector<Card> holdcard1;
    holdcard1.push_back(Card(0x08));
    holdcard1.push_back(Card(0x28));
    //holdcard1.back().changeFace(14);
    show(holdcard1, "holdcard1:");

    vector<Card> holdcard2;
    holdcard2.push_back(Card(0x27));
    holdcard2.push_back(Card(0x37));
    show(holdcard2, "holdcard2:");

    bool isBig = g_deck.compare(holdcard1, holdcard2);
    printf("isBig:%s\n", isBig ? "true" : "false");
}

void testLZArray(void)
{
    g_deck.shuffle(timeindex++);
    g_deck.setLZ(15);
    vector<Card> holdcard1;
    holdcard1.push_back(Card(0x02));
    holdcard1.back().changeFace(14);

    holdcard1.push_back(Card(0x12));
    //holdcard1.back().changeFace(14);

    holdcard1.push_back(Card(0x22));
    //holdcard1.back().changeFace(14);

    holdcard1.push_back(Card(0x31));
    //show(holdcard1, "holdcard1:");

    bool isBig = g_deck.isLZArray(holdcard1);
    printf("isLZArray:%s\n", isBig ? "true" : "false");
}

void testGetLZFollow(void)
{
    g_deck.shuffle(timeindex++);
    g_deck.setLZ(15);

    vector<Card> holdcard2;
    holdcard2.push_back(Card(0x03));
    holdcard2.push_back(Card(0x04));
    holdcard2.push_back(Card(0x05));
    holdcard2.push_back(Card(0x16));
    holdcard2.push_back(Card(0x17));
    holdcard2.push_back(Card(0x08));
    holdcard2.push_back(Card(0x19));
    holdcard2.push_back(Card(0x1A));
    holdcard2.push_back(Card(0x1B));
    Card::sortByDescending(holdcard2);
    show(holdcard2, "other:");
    g_deck.delCard(holdcard2);

    vector<Card> holdcard1;
    getCardByDesc(holdcard1, "JOKERc 2s 2h 2c 2d Ac Ks Kc Qh Qd Jh 10h 9d 7h 7d 6s 6d 5s 5h 4s");
    //g_deck.getHoldCard(holdcard1, 20 - holdcard1.size());
    //Card::sortByDescending(holdcard1);
    show(holdcard1, "mine:");


    vector<Card> result;
    vector<Card> change;
    g_deck.getLZFollow(holdcard1, holdcard2, result, change);
    Card::sortByDescending(result);
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

void testGetFirst(void)
{
    g_deck.shuffle(timeindex++);
    vector<Card> holdcard1;
    holdcard1.push_back(Card(0x14));
    holdcard1.push_back(Card(0x12));
    holdcard1.push_back(Card(0x03));
    holdcard1.push_back(Card(0x11));
    holdcard1.push_back(Card(0x0D));
    holdcard1.push_back(Card(0x3B));
    holdcard1.push_back(Card(0x2A));
    holdcard1.push_back(Card(0x36));
    holdcard1.push_back(Card(0x21));
    holdcard1.push_back(Card(0x17));
    holdcard1.push_back(Card(0x15));
    holdcard1.push_back(Card(0x07));
    holdcard1.push_back(Card(0x04));
    holdcard1.push_back(Card(0x01));
    holdcard1.push_back(Card(0x09));
    holdcard1.push_back(Card(0x24));
    holdcard1.push_back(Card(0x35));
    holdcard1.push_back(Card(0x2C));
    holdcard1.push_back(Card(0x16));
    holdcard1.push_back(Card(0x22));

    Card::sortByDescending(holdcard1);
    show(holdcard1, "origin:");

    vector<Card> result;
    g_deck.getFirst(holdcard1, result);
    show(result, "result:");
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
    //testLZArray();
    testGetLZFollow();
    //testChangeRecover();
    //testGetFirst();
    //testGetCardByDesc(); 
    return 0;
}

