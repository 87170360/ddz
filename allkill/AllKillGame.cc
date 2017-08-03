#include <assert.h>
#include <math.h>
#include "AllKillGame.h"
#include "AllKillMacros.h"
#include "AllKillPlayer.h"
#include "AllKillServer.h"

#include "log.h"

extern Log xt_log;

static DataBetPos m_DBP[AK_SEAT_ID_NU];
const static int CARDLISTNUM = 5;
static XtHoleCards m_sortCardList[CARDLISTNUM] = { XtHoleCards(), XtHoleCards(), XtHoleCards(), XtHoleCards(), XtHoleCards() };

bool compareDataBetPos(DataBetPos a,DataBetPos b)
{
      return a.m_bet < b.m_bet;
}

void AllKillGameBetting::dealEx(SysResult type)
{
    switch(type)
    {
        case SysResult_random:
        {
            m_deck.getHoleCards(&m_roleCard);
            m_roleCard.analysis();
            for(int i=0;i<AK_SEAT_ID_NU;i++)
            {
                m_deck.getHoleCards(&m_seatCard[i]);
                m_seatCard[i].analysis();
            }
        }
        break;
        case SysResult_lose:
        {
            createHolecards(false);
        }
        break;
        case SysResult_win:
        {
            createHolecards(true);
        }
        break;
    }
}

ResultType AllKillGameBetting::getResultType(bool sysWin) const
{
	for(int i = 0; i < AK_SEAT_ID_NU; ++i)
	{
		m_DBP[i].m_pos = i; 
		//m_DBP[i].m_bet = m_seatBet[i]; 
		m_DBP[i].m_bet = m_playerSeatBet[i]; 
	}

 	std::sort(m_DBP, m_DBP + AK_SEAT_ID_NU, compareDataBetPos);

    int randval = rand() % 100;
    if(randval < 5)
    {
        if(sysWin) 
        {
            return ResultType_0w;
        }
        else
        {
            return ResultType_4w;
        }
    }

    int top3 = 0;
	for(int i = 0; i < AK_SEAT_ID_NU - 1; ++i)
	{
		top3 += m_DBP[i].m_bet; 
	}

    if(top3 < m_DBP[AK_SEAT_ID_NU - 1].m_bet)
    {
        if(randval < 50) 
        {
            return ResultType_1w;
        }
        else
        {
            return ResultType_3w;
        }
    }
    else
    {
        return ResultType_2w;
    }
}

void AllKillGameBetting::createHolecards(bool sysWin)
{
   ResultType rt = getResultType(sysWin); 
   switch(rt)
   {
       case ResultType_0w:
           create0w(sysWin);
           break;
       case ResultType_1w:
           create1w(sysWin);
           break;
       case ResultType_2w:
           create2w(sysWin);
           break;
       case ResultType_3w:
           create3w(sysWin);
           break;
       case ResultType_4w:
           create4w(sysWin);
           break;
    }
}

void AllKillGameBetting::create0w(bool sysWin)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    m_sortCardList[0].copyCards(m_roleCard.m_cards);     
    m_roleCard.analysis();

	random_shuffle(m_sortCardList + 1, m_sortCardList + CARDLISTNUM);
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_sortCardList[i + 1].copyCards(m_seatCard[i].m_cards);     
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::create1w(bool sysWin)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    std::vector<int> otherPos;
    if(sysWin)
    {
        int minPos = m_DBP[0].m_pos;
        for(int i = 1; i < AK_SEAT_ID_NU; ++i)
        {
           otherPos.push_back(m_DBP[i].m_pos);
        }

        m_sortCardList[0].copyCards(m_seatCard[minPos].m_cards);
        m_sortCardList[1].copyCards(m_roleCard.m_cards);
    }
    else
    {
        int maxPos = m_DBP[AK_SEAT_ID_NU - 1].m_pos;
        for(int i = 0; i < AK_SEAT_ID_NU - 1; ++i)
        {
           otherPos.push_back(m_DBP[i].m_pos);
        }
        m_sortCardList[0].copyCards(m_seatCard[maxPos].m_cards);
        m_sortCardList[1].copyCards(m_roleCard.m_cards);
    }

	random_shuffle(m_sortCardList + 2, m_sortCardList + CARDLISTNUM);
    for(int i = 0; i < 3; ++i)
    {
        m_sortCardList[i + 2].copyCards(m_seatCard[otherPos[i]].m_cards);
    }

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::create2w(bool sysWin)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }

    int randval = rand() % 100;

    if(sysWin)
    {
        if(randval < 50)
        {
            m_sortCardList[0].copyCards(m_seatCard[allPos[0]].m_cards);
            m_sortCardList[1].copyCards(m_seatCard[allPos[1]].m_cards);
            m_sortCardList[2].copyCards(m_roleCard.m_cards);
            m_sortCardList[3].copyCards(m_seatCard[allPos[2]].m_cards);
            m_sortCardList[4].copyCards(m_seatCard[allPos[3]].m_cards);
        }
        else
        {
            m_sortCardList[0].copyCards(m_seatCard[allPos[0]].m_cards);
            m_sortCardList[1].copyCards(m_roleCard.m_cards);
            m_sortCardList[2].copyCards(m_seatCard[allPos[1]].m_cards);
            m_sortCardList[3].copyCards(m_seatCard[allPos[2]].m_cards);
            m_sortCardList[4].copyCards(m_seatCard[allPos[3]].m_cards);
        }
    }
    else
    {
        if(randval < 50)
        {
            m_sortCardList[0].copyCards(m_seatCard[allPos[2]].m_cards);
            m_sortCardList[1].copyCards(m_seatCard[allPos[3]].m_cards);
            m_sortCardList[2].copyCards(m_roleCard.m_cards);
            m_sortCardList[3].copyCards(m_seatCard[allPos[0]].m_cards);
            m_sortCardList[4].copyCards(m_seatCard[allPos[1]].m_cards);
        }
        else 
        {
            m_sortCardList[0].copyCards(m_seatCard[allPos[2]].m_cards);
            m_sortCardList[1].copyCards(m_seatCard[allPos[3]].m_cards);
            m_sortCardList[2].copyCards(m_seatCard[allPos[0]].m_cards);
            m_sortCardList[3].copyCards(m_roleCard.m_cards);
            m_sortCardList[4].copyCards(m_seatCard[allPos[1]].m_cards);
        }
    }

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}


void AllKillGameBetting::create3w(bool sysWin)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);

    std::vector<int> otherPos;
    if(sysWin)
    {
        int maxPos = m_DBP[AK_SEAT_ID_NU - 1].m_pos;
        for(int i = 0; i < AK_SEAT_ID_NU - 1; ++i)
        {
           otherPos.push_back(m_DBP[i].m_pos);
        }

        m_sortCardList[CARDLISTNUM - 1].copyCards(m_seatCard[maxPos].m_cards);
        m_sortCardList[CARDLISTNUM - 2].copyCards(m_roleCard.m_cards);
    }
    else
    {
        int minPos = m_DBP[0].m_pos;
        for(int i = 1; i < AK_SEAT_ID_NU; ++i)
        {
           otherPos.push_back(m_DBP[i].m_pos);
        }

        m_sortCardList[CARDLISTNUM - 1].copyCards(m_seatCard[minPos].m_cards);
        m_sortCardList[CARDLISTNUM - 2].copyCards(m_roleCard.m_cards);
    }

	random_shuffle(m_sortCardList, m_sortCardList + 3);
    for(int i = 0; i < 3; ++i)
    {
        m_sortCardList[i].copyCards(m_seatCard[otherPos[i]].m_cards);
    }

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::create4w(bool sysWin)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    m_sortCardList[CARDLISTNUM - 1].copyCards(m_roleCard.m_cards);     
    m_roleCard.analysis();

	random_shuffle(m_sortCardList, m_sortCardList + CARDLISTNUM - 1);
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_sortCardList[i].copyCards(m_seatCard[i].m_cards);     
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func1(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    m_sortCardList[0].copyCards(m_roleCard.m_cards);     
    m_roleCard.analysis();

	random_shuffle(m_sortCardList + 1, m_sortCardList + CARDLISTNUM);
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_sortCardList[i + 1].copyCards(m_seatCard[i].m_cards);     
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func2(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }

    m_sortCardList[0].copyCards(m_seatCard[allPos[0]].m_cards);
    m_sortCardList[1].copyCards(m_roleCard.m_cards);
    m_sortCardList[2].copyCards(m_seatCard[allPos[1]].m_cards);
    m_sortCardList[3].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[3]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func3(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }

    m_sortCardList[0].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[1].copyCards(m_seatCard[allPos[1]].m_cards);
    m_sortCardList[2].copyCards(m_roleCard.m_cards);
    m_sortCardList[3].copyCards(m_seatCard[allPos[3]].m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[0]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func4(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }

    m_sortCardList[0].copyCards(m_seatCard[allPos[1]].m_cards);
    m_sortCardList[1].copyCards(m_seatCard[allPos[0]].m_cards);
    m_sortCardList[2].copyCards(m_roleCard.m_cards);
    m_sortCardList[3].copyCards(m_seatCard[allPos[3]].m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[2]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func5(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }

    m_sortCardList[0].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[1].copyCards(m_seatCard[allPos[1]].m_cards);
    m_sortCardList[2].copyCards(m_seatCard[allPos[0]].m_cards);
    m_sortCardList[3].copyCards(m_roleCard.m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[3]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func6(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }

    m_sortCardList[0].copyCards(m_seatCard[allPos[0]].m_cards);
    m_sortCardList[1].copyCards(m_roleCard.m_cards);
    m_sortCardList[2].copyCards(m_seatCard[allPos[3]].m_cards);
    m_sortCardList[3].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[1]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func7(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
    m_sortCardList[4].copyCards(m_roleCard.m_cards);     

	random_shuffle(m_sortCardList, m_sortCardList + 4);
    
    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_sortCardList[i].copyCards(m_seatCard[i].m_cards);     
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func8(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);

    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }

    m_sortCardList[0].copyCards(m_seatCard[allPos[3]].m_cards);
    m_sortCardList[1].copyCards(m_roleCard.m_cards);
    m_sortCardList[2].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[3].copyCards(m_seatCard[allPos[1]].m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[0]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func9(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);

    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }

    m_sortCardList[0].copyCards(m_seatCard[allPos[3]].m_cards);
    m_sortCardList[1].copyCards(m_seatCard[allPos[1]].m_cards);
    m_sortCardList[2].copyCards(m_roleCard.m_cards);
    m_sortCardList[3].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[0]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func10(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);

    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }
    m_sortCardList[0].copyCards(m_seatCard[allPos[3]].m_cards);
    m_sortCardList[1].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[2].copyCards(m_roleCard.m_cards);
    m_sortCardList[3].copyCards(m_seatCard[allPos[1]].m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[0]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func11(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
	random_shuffle(m_sortCardList, m_sortCardList + 3);

    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }
    m_sortCardList[0].copyCards(m_seatCard[allPos[3]].m_cards);
    m_sortCardList[1].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[2].copyCards(m_seatCard[allPos[1]].m_cards);
    m_sortCardList[3].copyCards(m_roleCard.m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[0]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::func12(void)
{
    m_deck.getSortHoleCardList(m_sortCardList, CARDLISTNUM);
	random_shuffle(m_sortCardList, m_sortCardList + 3);

    int allPos[AK_SEAT_ID_NU];
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
       allPos[i] = m_DBP[i].m_pos;
    }
    m_sortCardList[0].copyCards(m_seatCard[allPos[3]].m_cards);
    m_sortCardList[1].copyCards(m_seatCard[allPos[2]].m_cards);
    m_sortCardList[2].copyCards(m_seatCard[allPos[0]].m_cards);
    m_sortCardList[3].copyCards(m_roleCard.m_cards);
    m_sortCardList[4].copyCards(m_seatCard[allPos[1]].m_cards);

    m_roleCard.analysis();
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        m_seatCard[i].analysis();
    }
}

BetType AllKillGameBetting::getBetType(void) const
{
	for(int i = 0; i < AK_SEAT_ID_NU; ++i)
	{
		m_DBP[i].m_pos = i; 
		m_DBP[i].m_bet = m_playerSeatBet[i]; 
		//m_DBP[i].m_bet = m_seatBet[i]; 
	}

 	std::sort(m_DBP, m_DBP + AK_SEAT_ID_NU, compareDataBetPos);

    int top3 = 0;
	for(int i = 0; i < AK_SEAT_ID_NU - 1; ++i)
	{
		top3 += m_DBP[i].m_bet; 
	}

    if(top3 < m_DBP[AK_SEAT_ID_NU - 1].m_bet)
    {
        return BetType_1;
    }

    if(m_DBP[0].m_bet == m_DBP[1].m_bet && m_DBP[1].m_bet == m_DBP[2].m_bet && m_DBP[2].m_bet == m_DBP[3].m_bet)
    {
       return BetType_2;     
    }

    if(m_DBP[2].m_bet + m_DBP[3].m_bet > m_DBP[0].m_bet + m_DBP[1].m_bet)
    {
       return BetType_3;     
    }

    return BetType_4;     
}
        
void AllKillGameBetting::funcRand(void)
{
    m_deck.getHoleCards(&m_roleCard);
    m_roleCard.analysis();
    for(int i=0;i<AK_SEAT_ID_NU;i++)
    {
        m_deck.getHoleCards(&m_seatCard[i]);
        m_seatCard[i].analysis();
    }
}

void AllKillGameBetting::deal3(SysResult type)
{
    BetType bt = getBetType();
    //xt_log.info("deal3 SysResult:%d, bt:%d.\n", type, bt);
    switch(type)
    {
        case SysResult_random:
        {
            funcRand();
            int role_win = 0;
            for(int i = 0; i < AK_SEAT_ID_NU; i++)
            {
                if(m_seatCard[i].compare(m_roleCard) <= 0)
                {
                    --role_win;
                }
                else 
                {
                    ++role_win;
                }
            }
            if(role_win == AK_SEAT_ID_NU or -role_win == AK_SEAT_ID_NU) 
            {
                funcRand(); 
            }
        }
        break;
        case SysResult_lose:
        {
           ResultType rt = getResultTypeLost4(); 
           if(!procCard(rt, bt, false))
           {
                rt = getResultTypeLost3(); 
                if(!procCard(rt, bt, false))
                {
                    rt = getResultTypeLost2(); 
                    if(!procCard(rt, bt, false))
                    {
                        funcRand();
                        xt_log.info("deal3error lost rt:%d, bt:%d.\n", rt, bt);
                    }
                }
           }
        }
        break;
        case SysResult_win:
        {
           ResultType rt = getResultTypeWin4(); 
           if(!procCard(rt, bt, true))
           {
                rt = getResultTypeWin3(); 
                if(!procCard(rt, bt, true))
                {
                    rt = getResultTypeWin2(); 
                    if(!procCard(rt, bt, true))
                    {
                        //funcRand();
                        //找不到合适的，就3输1赢
                        func2();
                        xt_log.info("deal3error win rt:%d, bt:%d.\n", rt, bt);
                    }
                }
           }
        }
        break;
    }
}

ResultType AllKillGameBetting::getResultTypeLost4(void) const
{
    std::map<int, int> weightmap;
    weightmap[ResultType_1w] = 9;
    weightmap[ResultType_2w] = 48;
    weightmap[ResultType_3w] = 36;
    weightmap[ResultType_4w] = 7;
    int total = 9 + 48 + 36 + 7;
    int randvalue = rand() % total;
    for(std::map<int, int>::iterator iter = weightmap.begin(); iter != weightmap.end(); ++iter)
    {
        randvalue -= iter->second;     
        if(randvalue <= 0)
        {
            return ResultType(iter->first);
        }
    }
    return ResultType_4w;
}

ResultType AllKillGameBetting::getResultTypeLost3(void) const
{
    std::map<int, int> weightmap;
    weightmap[ResultType_2w] = 51;
    weightmap[ResultType_3w] = 41;
    weightmap[ResultType_4w] = 8;
    int total = 51 + 41 + 8;
    int randvalue = rand() % total;
    for(std::map<int, int>::iterator iter = weightmap.begin(); iter != weightmap.end(); ++iter)
    {
        randvalue -= iter->second;     
        if(randvalue <= 0)
        {
            return ResultType(iter->first);
        }
    }
    return ResultType_4w;
}

ResultType AllKillGameBetting::getResultTypeLost2(void) const
{
    std::map<int, int> weightmap;
    weightmap[ResultType_3w] = 84;
    weightmap[ResultType_4w] = 16;
    int total = 84 + 16;
    int randvalue = rand() % total;
    for(std::map<int, int>::iterator iter = weightmap.begin(); iter != weightmap.end(); ++iter)
    {
        randvalue -= iter->second;     
        if(randvalue <= 0)
        {
            return ResultType(iter->first);
        }
    }
    return ResultType_4w;
}

ResultType AllKillGameBetting::getResultTypeWin4(void) const
{
    std::map<int, int> weightmap;
    weightmap[ResultType_0w] = 8;
    weightmap[ResultType_1w] = 10;
    weightmap[ResultType_2w] = 37;
    weightmap[ResultType_3w] = 45;
    int total = 8 + 10 + 37 + 45;
    int randvalue = rand() % total;
    for(std::map<int, int>::iterator iter = weightmap.begin(); iter != weightmap.end(); ++iter)
    {
        randvalue -= iter->second;     
        if(randvalue <= 0)
        {
            return ResultType(iter->first);
        }
    }
    return ResultType_0w;
}

ResultType AllKillGameBetting::getResultTypeWin3(void) const
{
    std::map<int, int> weightmap;
    weightmap[ResultType_0w] = 10;
    weightmap[ResultType_1w] = 52;
    weightmap[ResultType_2w] = 38;
    int total = 10 + 52 + 38;
    int randvalue = rand() % total;
    for(std::map<int, int>::iterator iter = weightmap.begin(); iter != weightmap.end(); ++iter)
    {
        randvalue -= iter->second;     
        if(randvalue <= 0)
        {
            return ResultType(iter->first);
        }
    }
    return ResultType_0w;
}

ResultType AllKillGameBetting::getResultTypeWin2(void) const
{
    std::map<int, int> weightmap;
    weightmap[ResultType_0w] = 18;
    weightmap[ResultType_1w] = 82;
    int total = 18 + 82;
    int randvalue = rand() % total;
    for(std::map<int, int>::iterator iter = weightmap.begin(); iter != weightmap.end(); ++iter)
    {
        randvalue -= iter->second;     
        if(randvalue <= 0)
        {
            return ResultType(iter->first);
        }
    }
    return ResultType_0w;
}
        
bool AllKillGameBetting::procCard(ResultType rt, BetType bt, bool SysWin) 
{
    if(SysWin)
    {
        if(rt == ResultType_0w) 
        {
            func1();
	        xt_log.info("proc card, func1, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_1w && bt == BetType_1) 
        {
            func2();
	        xt_log.info("proc card, func2, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_1w && bt == BetType_3) 
        {
            func6();
	        xt_log.info("proc card, func6, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_2w && bt == BetType_1) 
        {
            func3();
	        xt_log.info("proc card, func3, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_2w && bt == BetType_3) 
        {
            func4();
	        xt_log.info("proc card, func4, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_3w && bt == BetType_1) 
        {
            func5();
	        xt_log.info("proc card, func5, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
    }
    else
    {
        if(rt == ResultType_4w) 
        {
            func7();
	        xt_log.info("proc card, func7, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_1w && bt == BetType_1) 
        {
            func8();
	        xt_log.info("proc card, func8, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_2w && bt == BetType_1) 
        {
            func9();
	        xt_log.info("proc card, func9, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_2w && bt == BetType_3) 
        {
            func10();
	        xt_log.info("proc card, func10, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_3w && bt == BetType_1) 
        {
            func11();
	        xt_log.info("proc card, func11, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
        if(rt == ResultType_3w && bt == BetType_3) 
        {
            func12();
	        xt_log.info("proc card, func12, rt:%d, bt:%d.\n", rt, bt);
            return true;
        }
    }
	
    //xt_log.info("proc card, not find rt:%d, bt:%d.\n", rt, bt);
    return false;
}
        
bool AllKillGameBetting::isBetZero(void) const
{
    int tmp = 0;
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        tmp += m_playerSeatBet[i];
    }
    return (tmp > 0) ? false : true;
}

int AllKillGameBetting::totalBet(void) const
{
    int tmp = 0;
    for(int i = 0; i < AK_SEAT_ID_NU; ++i)
    {
        tmp += m_seatBet[i];
    }
    return tmp;
}
        
void AllKillGameBetting::addPlayerSeatBet(int seat_id, int benu)
{
    if(seat_id < 0 || seat_id > AK_SEAT_ID_NU)
    {
        return;
    }
    m_playerSeatBet[seat_id] += benu;
}

void AllKillGameBetting::deal( bool sys_role,int radio)
{

	xt_log.info("deal card ,role another card radio %d.\n",radio);

	m_deck.getHoleCards(&m_roleCard);
	m_roleCard.analysis();


	if(m_roleCard.m_cardType==CARD_TYPE_DANPAI && sys_role)
	{
		if(rand()%100<radio)
		{
			m_deck.getHoleCards(&m_roleCard);
			m_roleCard.analysis();
		}
	}

	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		m_deck.getHoleCards(&m_seatCard[i]);
		m_seatCard[i].analysis();
	}
	m_betRemainTime=AK_START_TIME;

}
        
void WinControl::setValueToRedis(const std::string &name, double value)
{
    if(!m_pServer)
    {
		xt_log.error("WinControl initValueToRedis error AllKillServer* is null.\n");
        return;
    }

    RedisClient* pCacheRedis = m_pServer->getCacheRedis();
    if(!pCacheRedis)
    {
		xt_log.error("WinControl initValueToRedis error RedisClient* is null.\n");
        return;
    }

    int ret = pCacheRedis->command("hset ak_info %s %.0f", name.c_str(), value);
    if(ret < 0)
    {
        xt_log.error("WinControl initValueToRedis error(set ak_info %s, %.0f).\n", name.c_str(), value);
    }
}

void WinControl::init(AllKillServer* pServer)
{
    m_pServer = pServer;

    if(!m_pServer)
    {
		xt_log.error("WinControl init error AllKillServer* is null.\n");
        return;
    }
}

double WinControl::getValueFromRedis(const std::string &str) const
{
    if(!m_pServer)
    {
		xt_log.error("WinControl getValueFromRedis, AllKillServer* is null, str:%s.\n", str.c_str());
        return 0;
    }

    RedisClient* pCacheRedis = m_pServer->getCacheRedis();
    if(!pCacheRedis)
    {
		xt_log.error("WinControl getValueFromRedis, RedisClient* is null, str:%s. \n", str.c_str());
        return 0;
    }

	int ret = pCacheRedis->command("hget ak_info %s", str.c_str());
	if(ret < 0)
	{
        xt_log.error("WinControl getValueFromRedis error(hget ak_info %s).\n", str.c_str());
        return 0;
	}
	return atof(pCacheRedis->reply->str);
}

SysResult WinControl::getPlayResult(bool sysRole, bool betZero) const
{
    double m_change_value       = getValueFromRedis("m_change_value"); 
    double m_win_act_lower      = getValueFromRedis("m_win_act_lower"); 
    double m_win_act_upper      = getValueFromRedis("m_win_act_upper"); 
    double m_lose_act_lower     = getValueFromRedis("m_lose_act_lower"); 
    double m_lose_act_upper     = getValueFromRedis("m_lose_act_upper"); 
    double m_proba              = getValueFromRedis("m_proba"); 
    double m_proba_win          = getValueFromRedis("m_proba_win"); 
    double player_role_win      = getValueFromRedis("player_role_win"); 

    if(sysRole)
    {
        if(betZero)
        {
            return SysResult_random;
        }
        else if(m_change_value < m_win_act_lower) 
        {
            return SysResult_win; 
        }
        else if(m_change_value > m_lose_act_upper) 
        {
            return SysResult_lose; 
        }
        else if(m_change_value >= m_win_act_lower && m_change_value <= m_win_act_upper && (rand() % 100 < m_proba_win)) 
        {
            return SysResult_win; 
        }
        else if(m_change_value >= m_lose_act_lower && m_change_value <= m_lose_act_upper && (rand() % 100 < m_proba)) 
        {
            return SysResult_lose; 
        }
    }
    else
    {
        return  (rand() % 100 < player_role_win) ? SysResult_win : SysResult_lose;
    }

    return SysResult_random;
}
        
void WinControl::resetValue(void)
{
    double m_change_value       = getValueFromRedis("m_change_value"); 
    double m_win_act_lower      = getValueFromRedis("m_win_act_lower"); 
    double m_lose_act_upper     = getValueFromRedis("m_lose_act_upper"); 
    if(m_change_value > m_lose_act_upper && m_win_act_lower != 0)
    {
       setValueToRedis("m_change_value", m_win_act_lower);
       xt_log.info("wincontral resetvalue %.0f", m_win_act_lower);
    }
}

void WinControl::updateToRedis(const std::string &name, double value)
{
    if(value == 0)
    {
        return;
    }
    RedisClient* pCacheRedis = m_pServer->getCacheRedis();
	int ret = pCacheRedis->command("hincrby ak_info %s %.0f", name.c_str(), value);
	if(ret < 0)
	{
		xt_log.error("cache redis error( hincrby ak_info %s %.0f).\n", name.c_str(), value);
	}
}

static int AK_GameGetCardBetTime(int card_type)
{
	int card_bet=1;
	switch(card_type)
	{
		case CARD_TYPE_BAOZI:
			card_bet=5;
			break;
		case CARD_TYPE_SHUNJIN:
			card_bet=4;
			break;
		case CARD_TYPE_JINHUA:
			card_bet=3;
			break;
		case CARD_TYPE_SHUNZI:
			card_bet=2;
			break;
	}
	return card_bet;
}


class AK_GamePlayerBetSortFunc
{
	public:
		AK_GamePlayerBetSortFunc(int seat_id)
		{
			m_seatId=seat_id;
		}

	public:
		int operator() (AllKillPlayer* l,AllKillPlayer* r)
		{
			return l->getSeatBet(m_seatId)>r->getSeatBet(m_seatId);
		}

	public:
		int m_seatId;
};



void AllKillGame::onReadyTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->readyTimer();
}

void AllKillGame::onStartTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->startTimer();
}


void AllKillGame::onEndGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->endGameTimer();
}

void AllKillGame::onUpdateGameTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*)w->data;
	game->updateGameTimer();
}

void AllKillGame::onChangeRoleTimer(struct ev_loop* loop,struct ev_timer* w,int revents)
{
	AllKillGame* game=(AllKillGame*) w->data;
	game->changeRoleTimer();
}




AllKillGame::AllKillGame()
{
	m_role=NULL;
	m_server=NULL;
	m_status=AK_Ready;
	m_evLoop=NULL;
	m_rottleTotalMoney=0;
    for(int i = 0; i < AK_DECKPLAYER_NU; ++i)
    {
        m_descPlayers[i] = 0; 
    }
}

AllKillGame::~AllKillGame()
{


}




AllKillPlayer* AllKillGame::getPlayer(int uid)
{
	std::map<int,AllKillPlayer*>::iterator iter= m_offlinePlayers.find(uid);
	if(iter!=m_offlinePlayers.end())
	{
		return iter->second;
	}

	iter=m_loginPlayers.find(uid);

	if(iter!=m_loginPlayers.end())
	{
		xt_log.error("player(%d) allready login\n",uid);
		return iter->second;
	}

	AllKillPlayer* player=new AllKillPlayer;
	player->setData(uid,m_server,this);

	m_offlinePlayers[uid]=player;

	return player;

}

AllKillPlayer* AllKillGame::getPlayerNoAdd(int uid)
{
	std::map<int,AllKillPlayer*>::iterator iter= m_offlinePlayers.find(uid);
	if(iter!=m_offlinePlayers.end())
	{
		return iter->second;
	}

	iter=m_loginPlayers.find(uid);
	if(iter!=m_loginPlayers.end())
	{
		return iter->second;
	}

	return NULL;
}


void AllKillGame::playerLogin(AllKillPlayer* player)
{
	assert(player!=NULL);
	int uid=player->getUid();

	std::map<int,AllKillPlayer*>::iterator iter;
	iter=m_loginPlayers.find(uid);
	if(iter!=m_loginPlayers.end())
	{
		xt_log.error("player(%d) all ready login\n",uid);
		return;
	}

	iter= m_offlinePlayers.find(uid);


	if(iter!=m_offlinePlayers.end())
	{
		m_loginPlayers[iter->first]=iter->second;
		m_offlinePlayers.erase(iter);
	}
	else 
	{
		xt_log.error("player find in offline\n");
	}


	sendLoginSuccess(player);
	sendGameInfo(player);
	sendPlayerChat(player,m_gameConfig.m_enterRoomChat);


}


void AllKillGame::playerBet(AllKillPlayer* player,Jpacket& package)
{
	int seat_id=package.val["seat_id"].asInt();
	int bet_nu=package.val["bet_nu"].asInt();


	if(m_status!=AK_Start)
	{
		sendBetError(player,AK_STATUS_ERR,"");
		return;
	}


	if(bet_nu<=0)
	{
		sendBetError(player,AK_BET_NU_ERR,"bet nu greater than 0");
		return;
	}

	if(seat_id<AK_SEAT_ID_START&&seat_id>AK_SEAT_ID_END)
	{
		sendBetError(player,AK_SEAT_ID_ERR,"");
		return;
	}

    if(isBetLimit())
    {
		sendBetError(player,AK_BET_LIMIT,"");
		return;
    }

	int money=player->getMoney();

	if(money<m_gameConfig.m_baseMoney*bet_nu)
	{
		money=player->updateMoney();
	}

	if(money<m_gameConfig.m_baseMoney*bet_nu)
	{
		sendBetError(player,AK_MONEY_NOT_ENOUGH,"");
		return ;
	}

	m_gameBetting.m_seatBet[seat_id-AK_SEAT_ID_START]+=bet_nu;

	player->decMoney(m_gameConfig.m_baseMoney*bet_nu);

	player->addSeatBet(seat_id,bet_nu);

	int uid=player->getUid();

    if(uid > VALID_CLIENT_UID_MIN)
    {
        m_gameBetting.addPlayerSeatBet(seat_id-AK_SEAT_ID_START, bet_nu);
    }

	m_betPlayers[uid]=player;
	sendBetSuccess(player,seat_id);

	CheckBroadcastDeskBetInfo(player,seat_id);
}

void AllKillGame::playerAskRole(AllKillPlayer* player,Jpacket& package)
{

	xt_log.info("player(%d) ask_role\n",player->getUid());

	int role_nu=m_askRoleList.size();

	for(int i=0;i<role_nu;i++)
	{
		if(m_askRoleList[i]==player)
		{
			sendAskRoleSuccess(player);
			return;
		}
	}

	if(m_role==player)
	{
		xt_log.error("player(%d) alread role\n",player->getUid());
		sendAskRoleSuccess(player);
		return;
	}

	int money=player->getMoney(); 
	if(money<m_gameConfig.m_askRoleMinMoney)
	{
		money=player->updateMoney();
	}

	if(money<m_gameConfig.m_askRoleMinMoney)
	{
		xt_log.error("player(%d) ask role, money(%d) not fit\n",player->getUid(),money);
		sendAskRoleError(player,AK_MONEY_NOT_ENOUGH,"");
		return;
	}

	m_askRoleList.push_back(player);
	sendAskRoleSuccess(player);

}

void AllKillGame::playerUnRole(AllKillPlayer* player,Jpacket& package)
{
	if(m_role==player)
	{
		if(m_status==AK_EndGame)
		{
			m_role=NULL;

			if(m_askRoleList.size()>0)
			{
				AllKillPlayer* player=m_askRoleList[0];
				player->setUnRole(false);
				m_askRoleList.erase(m_askRoleList.begin());
				m_role=player;
			}
			sendUnRoleSuccess(player);
			return;
		}

		m_role->setUnRole(true);
		sendUnRoleSuccess(player);

		return;
	}

	int role_nu=m_askRoleList.size();

	for(int i=0;i<role_nu;i++)
	{
		if(m_askRoleList[i]==player)
		{
			m_askRoleList.erase(m_askRoleList.begin()+i);
			sendUnRoleSuccess(player);
			return;
		}
	}
	sendUnRoleErr(player,AK_NOT_IN_ROLE_LIST,"");
}

void AllKillGame::playerDesk(AllKillPlayer* player,Jpacket& package)
{
	if (player == NULL)
	{
		xt_log.error("player is null\n");
		return;
	}

	/* 判断金币数量是否达标 */
	if (player->getMoney() < m_gameConfig.m_deskMinMoney)
	{
		if (player->updateMoney() < m_gameConfig.m_deskMinMoney)
		{
			xt_log.warn("player(%d) money not enough\n",player->getUid());
			
			Jpacket packet;
			packet.val["cmd"]	  = AK_DESK_RSP;
			packet.val["ret"]	  = (int)AK_MONEY_NOT_ENOUGH;
			packet.val["desc"]	  = "money not enough.";
			packet.end();
			m_server->unicast(player, packet.tostring());
			return;
		}
	}
	
	/* 判断目标位置是否合法 */
	int seatid = package.val["desk_seatid"].asInt();
	if (seatid >= AK_DECKPLAYER_NU || seatid < 0)
	{
		xt_log.error("player(%d) get desk fail, seatid error:%d \n",player->getUid(), seatid);
		
		Jpacket packet;
		packet.val["cmd"]	  = AK_DESK_RSP;
		packet.val["ret"]     = (int)AK_SEAT_ID_ERR;
		packet.val["desc"]    = "seatid error.";
		packet.end();
		m_server->unicast(player, packet.tostring());
		return;
	}
	
	/* 判断当前位置是否有人 */
	int iCurDeskSeatid = player->getDeskSeatid();
	if (m_descPlayers[seatid] != 0)
	{
		if (iCurDeskSeatid == seatid)
		{
			xt_log.info("player(%d) send same desk seatid:%d \n",player->getUid(), seatid);

			Jpacket packet;
			packet.val["cmd"]		= AK_DESK_RSP;
			packet.val["ret"]		= (int)AK_SUCCESS;
			packet.val["desc"]		= "success.";
			packet.end();
			m_server->unicast(player, packet.tostring());
		}
		else
		{
			xt_log.warn("player(%d) get desk fail, req seatid %d cur seatid:%d \n",player->getUid(), seatid, iCurDeskSeatid);

			Jpacket packet;
			packet.val["cmd"]	  = AK_DESK_RSP;
			packet.val["ret"]     = (int)AK_SEAT_HAVE_PLAYER;
			packet.val["desc"]    = "seat have player.";
			packet.end();
			m_server->unicast(player, packet.tostring());
		}
		
		return;
	}

	xt_log.info("player(%d) seat desk seatid:%d success.\n",player->getUid(), seatid);

	/* 如已经在桌上则将当前位子清空 */
	if (iCurDeskSeatid >= 0 && iCurDeskSeatid < AK_DECKPLAYER_NU)
	{
		m_descPlayers[iCurDeskSeatid] = 0;
	}

    m_descPlayers[seatid] = player->getUid();
	player->setDeskSeatid(seatid);

	/* 发送应答 */
	Jpacket packetResp;
	packetResp.val["cmd"]		= AK_DESK_RSP;
	packetResp.val["ret"]		= (int)AK_SUCCESS;
	packetResp.val["desc"]	  	= "success.";
	packetResp.end();
	m_server->unicast(player, packetResp.tostring());
	
	/* 发送上桌广播 */
    Jpacket packet;
    packet.val["cmd"]       = AK_DESK_SB;
    packet.val["name"]      = player->getName();
    packet.val["uid"]       = player->getUid();
    packet.val["avatar"]    = player->getAvatar();
    packet.val["money"]     = player->getMoney();
    packet.val["sex"]       = player->getSex();
    packet.val["desk_seatid"]    = seatid;
	packet.val["prev_seatid"]	 = iCurDeskSeatid;
    packet.end();
    m_server->broadcast(NULL, packet.tostring());
}

void AllKillGame::playerLogout(AllKillPlayer* player)
{
	int uid=player->getUid();
	if(m_status==AK_EndGame)
	{
		m_loginPlayers.erase(uid);
		if(player==m_role)
		{
			m_role=NULL;
		}
		sendPlayerLogoutSuccess(player);

		int role_nu=m_askRoleList.size();
		for(int i=0;i<role_nu;i++)
		{
			if(m_askRoleList[i]==player)
			{
				m_askRoleList.erase(m_askRoleList.begin()+i);
				broadcastAskRoleChange(player);
				break;
			}
		}

		if(m_betPlayers.find(uid)!=m_betPlayers.end())
		{
			m_offlinePlayers[uid]=player;
		}
		else 
		{
			delete player;
		}
		return;

	}

	if(m_status==AK_Ready)
	{
		if(player==m_role)
		{
			player->setUnRole(true);
			m_loginPlayers.erase(uid);
			m_offlinePlayers[uid]=player;
			sendPlayerLogoutSuccess(player);
		}
		else 
		{
			m_loginPlayers.erase(uid);
			sendPlayerLogoutSuccess(player);

			int role_nu=m_askRoleList.size();
			for(int i=0;i<role_nu;i++)
			{
				if(m_askRoleList[i]==player)
				{
					m_askRoleList.erase(m_askRoleList.begin()+i);
					broadcastAskRoleChange(player);
					break;
				}
			}
			delete player;
		}
		return;
	}

	if(m_status==AK_Start)
	{
		if(m_betPlayers.find(uid)!=m_betPlayers.end()||m_role==player)
		{
			m_loginPlayers.erase(uid);
			m_offlinePlayers[uid]=player;

			sendPlayerLogoutSuccess(player);

			if(m_role==player)
			{
				m_role->setUnRole(true);
			}
			else 
			{
				int role_nu=m_askRoleList.size();
				for(int i=0;i<role_nu;i++)
				{
					if(m_askRoleList[i]==player)
					{
						m_askRoleList.erase(m_askRoleList.begin()+i);
						broadcastAskRoleChange(player);
						break;
					}
				}
			}
		}
		else 
		{
			m_loginPlayers.erase(uid);
			sendPlayerLogoutSuccess(player);

			int role_nu=m_askRoleList.size();
			for(int i=0;i<role_nu;i++)
			{
				if(m_askRoleList[i]==player)
				{
					m_askRoleList.erase(m_askRoleList.begin()+i);
					broadcastAskRoleChange(player);
					break;
				}
			}
			delete player;
		}
	}
}

int AllKillGame::configGame(Json::Value& value)
{
	m_gameConfig.m_baseMoney=value["venue"]["base_money"].asInt();
	m_gameConfig.m_deskMinMoney = value["venue"]["desk_min_money"].asInt();
	m_gameConfig.m_askRoleMinMoney=value["venue"]["ask_role_min"].asInt();
	m_gameConfig.m_roleMinMoney=value["venue"]["un_role_limit"].asInt();
	m_gameConfig.m_roleAnotherCard=value["venue"]["role_another_radio"].asInt();

	m_gameConfig.m_sysFee=value["venue"]["sys_fee"].asDouble();

	m_gameConfig.m_sysRoleName=value["venue"]["sys_role"]["name"].asString();
	m_gameConfig.m_sysRoleAvatar=value["venue"]["sys_role"]["avatar"].asString();
	m_gameConfig.m_sysRoleUid=value["venue"]["sys_role"]["uid"].asInt();
	m_gameConfig.m_sysRoleMoney=value["venue"]["sys_role"]["money"].asInt();
	m_gameConfig.m_sysRoleSex=value["venue"]["sys_role"]["sex"].asInt();

	m_gameConfig.m_enterRoomChat=value["venue"]["enter_chat"].asString();

	m_gameConfig.m_chatRoleName=value["venue"]["chat_role"]["name"].asString();
	m_gameConfig.m_chatRoleAvatar=value["venue"]["chat_role"]["avatar"].asString();
	m_gameConfig.m_chatRoleUid=value["venue"]["chat_role"]["uid"].asInt();
	m_gameConfig.m_chatRoleSex=value["venue"]["chat_role"]["sex"].asInt();

	m_gameConfig.m_rottleBaoZhiRadio=value["venue"]["rottle_radio"]["bao_zhi"].asDouble();
	m_gameConfig.m_rottleShunJinRadio=value["venue"]["rottle_radio"]["shun_jin"].asDouble();
	m_gameConfig.m_rottleJinHuaRadio=value["venue"]["rottle_radio"]["jin_hua"].asDouble();
	m_gameConfig.m_winRottleFee=value["venue"]["rottle_radio"]["win_rottle_fee"].asDouble();
	m_gameConfig.m_vid=value["venue"]["id"].asInt();

	m_gameConfig.m_rottleMinOpenMoney=value["venue"]["rottle_min_open_money"].asInt();

	m_gameConfig.m_maxGameResultHistory=value["venue"]["game_history_nu"].asInt();

	m_rottleFirstReward.m_uid=m_gameConfig.m_sysRoleUid;
	m_rottleFirstReward.m_name=m_gameConfig.m_sysRoleName;
	m_rottleFirstReward.m_avatar=m_gameConfig.m_sysRoleAvatar;
	m_rottleFirstReward.m_sex=m_gameConfig.m_sysRoleSex;
	m_rottleFirstReward.m_rewardMoney=0;

	return 0;
}



int AllKillGame::start(AllKillServer* server,struct ev_loop* loop)
{
	m_status=AK_Ready;
	m_server=server;
	m_evLoop=loop;

	m_gameBetting.m_deck.fill();
    //禁用部分牌型
    //重置在GameBetting reset 
    vector<int> forbitface;
    forbitface.push_back(2);
    forbitface.push_back(3);
    forbitface.push_back(4);
    forbitface.push_back(5);
    forbitface.push_back(6);
	m_gameBetting.m_deck.forbitFace(forbitface);

	m_evReadyTimer.data=this;
	ev_timer_init(&m_evReadyTimer,AllKillGame::onReadyTimer,0,AK_READY_TIME);
	m_evStartTimer.data=this;
	ev_timer_init(&m_evStartTimer,AllKillGame::onStartTimer,0,AK_START_TIME);

	m_evEndGameTimer.data=this;
	ev_timer_init(&m_evEndGameTimer,AllKillGame::onEndGameTimer,0,AK_END_GAME_TIME);

	m_evUpdateTimer.data=this;
	ev_timer_init(&m_evUpdateTimer,AllKillGame::onUpdateGameTimer,0,AK_UPDATE_TIME);

	m_evChangeRoleTimer.data=this;
	ev_timer_init(&m_evChangeRoleTimer,AllKillGame::onChangeRoleTimer,0,AK_END_GAME_TIME-AK_SET_NEXT_ROLE_BEFORE_TIME);


	ev_timer_again(m_evLoop,&m_evReadyTimer);
	handleGameReady();


    m_win_control.init(server);
	return 0;
}

int AllKillGame::shutDown()
{
	ev_timer_stop(m_evLoop,&m_evReadyTimer);
	ev_timer_stop(m_evLoop,&m_evStartTimer);
	ev_timer_stop(m_evLoop,&m_evEndGameTimer);
	ev_timer_stop(m_evLoop,&m_evUpdateTimer);
	ev_timer_stop(m_evLoop,&m_evChangeRoleTimer);

	//printf("m_offlinePlayers.size()=%d,m_loginPlayers.size()=%d\n",(int)m_offlinePlayers.size(),(int)m_loginPlayers.size());


	for(std::map<int,AllKillPlayer*>::iterator iter=m_offlinePlayers.begin();iter!=m_offlinePlayers.end();++iter)
	{
		AllKillPlayer* player=iter->second;
		assert(player);
		delete player;
	}

	for(std::map<int,AllKillPlayer*>::iterator iter=m_loginPlayers.begin();iter!=m_loginPlayers.end();++iter)
	{
		AllKillPlayer* player=iter->second;
		//	printf("uid=%d,player_addr=%ld\n",iter->first,(long)iter->second);
		assert(player);

		delete player;
	}


	m_loginPlayers.clear();
	m_betPlayers.clear();

	m_askRoleList.clear();

	if(m_role)
	{
		m_role=NULL;
	}
	m_server=NULL;

	return 0;

}




void AllKillGame::readyTimer()
{
	ev_timer_stop(m_evLoop,&m_evReadyTimer);
	m_status=AK_Start;

	handleGameStart();

	ev_timer_again(m_evLoop,&m_evStartTimer);
	ev_timer_again(m_evLoop,&m_evUpdateTimer);
}
/*
static void (AllKillGameBetting::*g_funcList[13]) (void) =
{
    &AllKillGameBetting::func1,
    &AllKillGameBetting::func2,
    &AllKillGameBetting::func3,
    &AllKillGameBetting::func4,
    &AllKillGameBetting::func5,
    &AllKillGameBetting::func6,
    &AllKillGameBetting::func7,
    &AllKillGameBetting::func8,
    &AllKillGameBetting::func9,
    &AllKillGameBetting::func10,
    &AllKillGameBetting::func11,
    &AllKillGameBetting::func12,
    &AllKillGameBetting::funcRand,
};
*/

void AllKillGame::setCardForTest(void)
{
    /*
    void (AllKillGameBetting::*pFun) (void) = NULL;
    XtHoleCards tmpHole[5]; 
    for(int i = 0; i < 13; ++i)
    {
        printf("func:%d\n", i + 1);
        pFun = g_funcList[i];
        (m_gameBetting.*pFun)();
        m_gameBetting.m_roleCard.debug();
        m_gameBetting.m_roleCard.analysis();
        m_gameBetting.m_seatCard[0].debug();
        m_gameBetting.m_seatCard[0].analysis();
        m_gameBetting.m_seatCard[1].debug();
        m_gameBetting.m_seatCard[1].analysis();
        m_gameBetting.m_seatCard[2].debug();
        m_gameBetting.m_seatCard[2].analysis();
        m_gameBetting.m_seatCard[3].debug();
        m_gameBetting.m_seatCard[3].analysis();

        tmpHole[0] = m_gameBetting.m_roleCard; 
        tmpHole[1] = m_gameBetting.m_seatCard[0]; 
        tmpHole[2] = m_gameBetting.m_seatCard[1]; 
        tmpHole[3] = m_gameBetting.m_seatCard[2]; 
        tmpHole[4] = m_gameBetting.m_seatCard[3]; 

        for(int j = 0; j < 5; ++j)
        {
            for(int k = j + 1; k < 5; ++k) 
            {
                if(tmpHole[j].same(tmpHole[k])) 
                {
                    printf("<::::::::::::::::::::::::::::::::::::::::::::[=================o func:%d same!\n", i + 1);
                }
            }
        }

        m_gameBetting.reset(); 
    }
    */
// 0  方块  1 梅花   2  红桃    3黑桃
/*
static int s_card_d[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
static int s_card_c[] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};
static int s_card_h[] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D};
static int s_card_s[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D};
*/
    m_gameBetting.m_roleCard.m_cards[2] = 0x02;
    m_gameBetting.m_roleCard.m_cards[1] = 0x12;
    m_gameBetting.m_roleCard.m_cards[0] = 0x22;
    m_gameBetting.m_roleCard.debug();
    m_gameBetting.m_roleCard.analysis();

    m_gameBetting.m_seatCard[0].m_cards[2] = 0x32;
    m_gameBetting.m_seatCard[0].m_cards[1] = 0x23;
    m_gameBetting.m_seatCard[0].m_cards[0] = 0x34;
    m_gameBetting.m_seatCard[0].debug();
    m_gameBetting.m_seatCard[0].analysis();

    m_gameBetting.m_seatCard[1].m_cards[2] = 0x15;
    m_gameBetting.m_seatCard[1].m_cards[1] = 0x06;
    m_gameBetting.m_seatCard[1].m_cards[0] = 0x17;
    m_gameBetting.m_seatCard[1].debug();
    m_gameBetting.m_seatCard[1].analysis();

    m_gameBetting.m_seatCard[2].m_cards[2] = 0x3C;
    m_gameBetting.m_seatCard[2].m_cards[1] = 0x3B;
    m_gameBetting.m_seatCard[2].m_cards[0] = 0x29;
    m_gameBetting.m_seatCard[2].debug();
    m_gameBetting.m_seatCard[2].analysis();

    m_gameBetting.m_seatCard[3].m_cards[2] = 0x01;
    m_gameBetting.m_seatCard[3].m_cards[1] = 0x0C;
    m_gameBetting.m_seatCard[3].m_cards[0] = 0x2A;
    m_gameBetting.m_seatCard[3].debug();
    m_gameBetting.m_seatCard[3].analysis();
}

bool AllKillGame::isBetLimit(void)
{
    if(!m_role)
    {
        return false; 
    }
   int betMoney = m_gameBetting.totalBet() * m_gameConfig.m_baseMoney; 
   int roleMoney = m_role->getMoney();
   //xt_log.info("isBetLimit, betMoney:%d, roleMoney:%d\n", betMoney, roleMoney);
   return betMoney > roleMoney;
}

void AllKillGame::startTimer()
{
	ev_timer_stop(m_evLoop,&m_evStartTimer);

    SysResult srt = m_win_control.getPlayResult(m_role == NULL, m_gameBetting.isBetZero());
    //xt_log.info("play result type :%d\n", (int)srt);
	m_gameBetting.deal3(srt);

    //test
    //setCardForTest();

	m_status=AK_EndGame;
	handleGameEnd();

	ev_timer_stop(m_evLoop,&m_evUpdateTimer);

	if(m_rottleResult.m_hasOpenRottle)
	{
		ev_timer_set(&m_evEndGameTimer,0,AK_END_GAME_TIME+AK_ROTTLE_OPEN_TIME);
		ev_timer_again(m_evLoop,&m_evEndGameTimer);

		ev_timer_set(&m_evChangeRoleTimer,0,AK_END_GAME_TIME+AK_ROTTLE_OPEN_TIME-AK_SET_NEXT_ROLE_BEFORE_TIME);
		ev_timer_again(m_evLoop,&m_evChangeRoleTimer);
	}
	else 
	{
		ev_timer_set(&m_evEndGameTimer,0,AK_END_GAME_TIME);
		ev_timer_again(m_evLoop,&m_evEndGameTimer);

		ev_timer_set(&m_evChangeRoleTimer,0,AK_END_GAME_TIME-AK_SET_NEXT_ROLE_BEFORE_TIME);
		ev_timer_again(m_evLoop,&m_evChangeRoleTimer);
	}


}

void AllKillGame::endGameTimer()
{
	ev_timer_stop(m_evLoop,&m_evChangeRoleTimer);
	ev_timer_stop(m_evLoop,&m_evEndGameTimer);

	m_status=AK_Ready;

	handleGameReady();
	ev_timer_again(m_evLoop,&m_evReadyTimer);


	m_server->refreshPlayerNuToRedis();

}


void AllKillGame::updateGameTimer()
{
	ev_timer_again(m_evLoop,&m_evUpdateTimer);
	handleGameUpdate();
}

void AllKillGame::changeRoleTimer()
{
	if(m_status!=AK_EndGame)
	{
		xt_log.error("change role timer error not EndGameStatus.\n");
		return;
	}

	ev_timer_stop(m_evLoop,&m_evChangeRoleTimer);
	handleChangeRole();
}






void AllKillGame::handleGameReady()
{

	m_rottleResult.reset();
	m_roleResult.reset();

	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		m_seatResult[i].reset();
	}

	m_gameBetting.reset();

	m_mostRewardPlayer=NULL;

/*
	RedisClient* cache_rc=m_server->getCacheRedis();


	int ret=cache_rc->command("hgetall ak_conf");

	if(ret>=0)
	{
		if(cache_rc->is_array_return_ok()>=0)
		{
			radio=cache_rc->get_value_as_int("role_another_radio");
		}
	}
*/

	//int radio=m_gameConfig.m_roleAnotherCard; 
	//m_gameBetting.deal(m_role==NULL,radio);
    m_gameBetting.m_betRemainTime=AK_START_TIME;


	/* reset bet */
	for(std::map<int,AllKillPlayer*>::iterator iter=m_betPlayers.begin();
			iter!=m_betPlayers.end();++iter)
	{
		iter->second->resetSeatBet();
	}
	m_betPlayers.clear();

	/* offline */
	for(std::map<int,AllKillPlayer*>::iterator iter=m_offlinePlayers.begin();
			iter!=m_offlinePlayers.end();++iter)
	{
		delete iter->second;
	}

	m_offlinePlayers.clear();

	setNextRole();

	broadcastGameReady(NULL);
}

void AllKillGame::CheckClearDesk(AllKillPlayer* player)
{
	int iSeatid = player->getDeskSeatid();
	if (iSeatid >= 0 && iSeatid < AK_SEAT_ID_NU)
	{
		m_descPlayers[iSeatid] = 0;

		xt_log.info("play(%d) logout clear desk info\n", player->getUid());
		
		broadcastDeskPlayerLeave(player, LDR_LOGOUT);
	}
}

void AllKillGame::handleGameStart()
{
	broadcastGameStart(NULL);
}

void AllKillGame::handleChangeRole()
{
	int ret=setNextRole();

	if(ret)
	{
		broadcastAskRoleChange(NULL);
	}
}

void AllKillGame::handlePlayerBetResult()
{

	/*  card info */
	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		if(m_gameBetting.m_seatCard[i].compare(m_gameBetting.m_roleCard)<=0)
		{
			m_seatResult[i].m_isWin=0;
		}
		else 
		{
			m_seatResult[i].m_isWin=1;
		}

		m_seatResult[i].m_cardType=m_gameBetting.m_seatCard[i].m_cardType;
		m_seatResult[i].m_betTime=AK_GameGetCardBetTime(m_gameBetting.m_seatCard[i].m_cardType);
		m_seatResult[i].m_totalBetNu=m_gameBetting.m_seatBet[i];
	}


	/* player bet list */
	for(std::map<int,AllKillPlayer*>::iterator iter=m_betPlayers.begin();iter!=m_betPlayers.end();++iter)
	{
		AllKillPlayer* player=iter->second;

		for(int i=0;i<AK_SEAT_ID_NU;i++)
		{
			int player_bet_nu=player->getSeatBet(i+AK_SEAT_ID_START);
			if(player_bet_nu>0)
			{
				m_seatResult[i].m_betPlayerList.push_back(player);
			}
		}
	}

	/* sort player bet */
	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		std::sort(m_seatResult[i].m_betPlayerList.begin(),
				m_seatResult[i].m_betPlayerList.end(),
				AK_GamePlayerBetSortFunc(i+AK_SEAT_ID_START)
				);
	}


	/* handle role result  */
	m_roleResult.m_cardType=m_gameBetting.m_roleCard.m_cardType;
	m_roleResult.m_betTime=AK_GameGetCardBetTime(m_roleResult.m_cardType);


	m_gameResultInfo.push_back(GameResultInfo(
				m_seatResult[0].m_isWin,
				m_seatResult[1].m_isWin,
				m_seatResult[2].m_isWin,
				m_seatResult[3].m_isWin));


	if(m_gameResultInfo.size()>(unsigned int) m_gameConfig.m_maxGameResultHistory)
	{
		m_gameResultInfo.pop_front();
	}
}

void AllKillGame::handleRottleResult()
{
	m_rottleResult.m_hasOpenRottle=false;
	if(m_rottleTotalMoney<m_gameConfig.m_rottleMinOpenMoney)
	{
		return ;
	}

	int card_types[]={CARD_TYPE_BAOZI,CARD_TYPE_SHUNJIN,CARD_TYPE_JINHUA};

	for(unsigned int i=0;i<sizeof(card_types)/sizeof(int);i++)
	{
		int num=getCardTypeNu(card_types[i]);
		if(num==0)
		{
			continue;
		}
		m_rottleResult.m_hasOpenRottle=true;
		handleRottleResultByCardType(card_types[i],num);
	}

	if(m_rottleResult.m_hasOpenRottle||m_role)
	{
		m_rottleFirstReward.m_rewardMoney=0;
	}
	else 
	{
		return ;
	}

	/* handle role rottle reward */
	if(m_role)
	{
		m_role->incMoney(m_rottleResult.m_roleRottleMoney);
		m_server->sendRottleFlow(m_gameConfig.m_vid,m_role->getUid(),m_rottleResult.m_roleRottleMoney,m_role->getMoney());
		m_rottleFirstReward.setIfMostReward(m_rottleResult.m_roleRottleMoney,m_role);
	}

	/* handle seat rottle reward */
	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		if(m_rottleResult.m_seatRottleMoney[i]==0)
		{
			continue;
		}

		for(std::vector<AllKillPlayer*>::iterator iter=m_seatResult[i].m_betPlayerList.begin();
				iter!=m_seatResult[i].m_betPlayerList.end();
				++iter)
		{
			AllKillPlayer* player=*iter;

			int bet_nu=player->getSeatBet(i+AK_SEAT_ID_START);
			assert(bet_nu>0);
			int rottle_money=floor(m_rottleResult.m_seatRottleMoney[i]*(float)bet_nu/(float)m_gameBetting.m_seatBet[i]);
			player->incMoney(rottle_money);
			m_rottleFirstReward.setIfMostReward(rottle_money,player);
			m_server->sendRottleFlow(m_gameConfig.m_vid,player->getUid(),rottle_money,player->getMoney());
		}
	}
}

int AllKillGame::getCardTypeNu(int card_type)
{
	int ret=0;
	if(m_roleResult.m_cardType==card_type)
	{
		ret++;
	}

	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		if(m_seatResult[i].m_cardType==card_type&&m_seatResult[i].m_totalBetNu>0)
		{
			ret++;
		}
	}
	return ret;
}


float AllKillGame::getCardTypeRottleRadio(int card_type)
{
	float rottle_radio=0.0;
	if(card_type==CARD_TYPE_BAOZI)
	{
		rottle_radio=m_gameConfig.m_rottleBaoZhiRadio;
	}
	else if (card_type==CARD_TYPE_SHUNJIN)
	{
		rottle_radio=m_gameConfig.m_rottleShunJinRadio;
	}
	else if(card_type==CARD_TYPE_JINHUA)
	{
		rottle_radio=m_gameConfig.m_rottleJinHuaRadio;
	}
	return rottle_radio;
}

void AllKillGame::handleRottleResultByCardType(int card_type,int num)
{
	assert(num>0);

	float rottle_radio=getCardTypeRottleRadio(card_type);
	int rottle_money=m_rottleTotalMoney*rottle_radio;
	m_rottleTotalMoney-=rottle_money;

	rottle_money=rottle_money/num;

	if(m_roleResult.m_cardType==card_type)
	{
		m_rottleResult.m_roleRottleMoney=rottle_money;
	}

	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		if(m_seatResult[i].m_cardType==card_type)
		{
			m_rottleResult.m_seatRottleMoney[i]=rottle_money;
		}
	}
}

void AllKillGame::handleMoneyResult()
{
	/* handle player bet info */
	for(std::map<int,AllKillPlayer*>::iterator iter=m_betPlayers.begin();iter!=m_betPlayers.end();++iter)
	{
		AllKillPlayer* player=iter->second;
		player->updateMoney();

		for(int i=0;i<AK_SEAT_ID_NU;i++)
		{
			int bet_nu=player->getSeatBet(i+AK_SEAT_ID_START);
			/* add player already bet money */
			player->incMoney(bet_nu*m_gameConfig.m_baseMoney);

			if(m_seatResult[i].m_isWin)
			{
				int win_money=bet_nu*m_gameConfig.m_baseMoney*m_seatResult[i].m_betTime;
				/* add bet result */
				player->addBetMoneyResult(win_money);
			}
			else 
			{
				int lose_money=bet_nu*(m_roleResult.m_betTime)*m_gameConfig.m_baseMoney;

				player->decBetMoneyResult(lose_money);
			}
		}
		m_server->saveBetNum(player->getUid(), player->getAllKillTotalBet() * m_gameConfig.m_baseMoney);
	}

    //玩家金币变动
    double roleMoneyChange = 0;
    //系统抽水
    double sysFee = 0;
	/* handle player lose */
	for(std::map<int,AllKillPlayer*>::iterator iter=m_betPlayers.begin();iter!=m_betPlayers.end();++iter)
	{
		AllKillPlayer* player=iter->second;
		int bet_money=player->getBetMoneyResult();
		if(bet_money>0)
		{//玩家输
			m_roleResult.m_roleLostMoney+=bet_money;
            if(iter->first > VALID_CLIENT_UID_MIN)
            {//机器人不算
                roleMoneyChange += (-bet_money);
                m_roleResult.m_humanResult += bet_money;
            }
			continue;
		}
		m_roleResult.m_roleWinMoney+=-bet_money;

        //lose 1% 进入彩金
		m_rottleTotalMoney += (-bet_money) * 0.01;

		int player_money=player->getMoney();
		if(player_money<0)
		{
			xt_log.error("player(%d) money(%d) is negative\n",player->getUid(),player_money);
			player_money=0;
		}

		if(player_money<-bet_money)
		{
			bet_money=-player_money;
		}

		player->setBetMoneyResult(bet_money);

		m_roleResult.m_roleRealWinMoney+=-bet_money;
        if(iter->first > VALID_CLIENT_UID_MIN)
        {//机器人不算
            roleMoneyChange += (-bet_money);
            m_roleResult.m_humanResult -= bet_money;
        }

		player->decMoney(-bet_money);
		m_server->sendBetFlow(m_gameConfig.m_vid,player->getUid(),bet_money,player->getMoney());
	}

	/* handle role win */
	float role_pay_radio=1.0f;
	int role_result_money=m_roleResult.m_roleRealWinMoney-m_roleResult.m_roleLostMoney;

	m_roleResult.m_roleResultBetMoney=role_result_money;

	if(m_role)
	{
		m_role->updateMoney();
		int role_money=m_role->getMoney();
		if(role_result_money>0)
		{
			int rottle_money=floor(role_result_money*m_gameConfig.m_winRottleFee);
			m_rottleTotalMoney+=rottle_money;

			xt_log.info("role money %d, add role money %d,rottle_money %d,real_money %d \n",role_money,role_result_money,rottle_money,role_result_money-rottle_money);

			int sys_fee=floor(role_result_money*m_gameConfig.m_sysFee);
			m_role->incMoney(role_result_money-rottle_money-sys_fee);
			m_server->sendBetFlow(m_gameConfig.m_vid,m_role->getUid(),role_result_money-rottle_money-sys_fee,m_role->getMoney());
            sysFee += sys_fee;
		}
		else 
		{
			if(role_money >= -role_result_money)
			{
				m_role->decMoney(-role_result_money);
				m_server->sendBetFlow(m_gameConfig.m_vid,m_role->getUid(),role_result_money,m_role->getMoney());
				xt_log.info("role money %d, dec role money %d,rottle_money %d,real_money %d \n",role_money,role_result_money);
			}
			else 
			{
				m_role->decMoney(role_money);
				m_server->sendBetFlow(m_gameConfig.m_vid,m_role->getUid(),-role_money,m_role->getMoney());
				xt_log.info("role money %d, dec role money %d,rottle_money %d,real_money %d \n",role_money,role_result_money);

				role_pay_radio=float(role_money)/float(-role_result_money);
			}
		}
		//xt_log.info("role_money is %d \n",m_role->updateMoney());
	}

	m_mostRewardPlayer=NULL;

	/* handle player win */
	for(std::map<int, AllKillPlayer*>::iterator iter=m_betPlayers.begin();
			iter!=m_betPlayers.end();
			++iter)
	{
		AllKillPlayer* player=iter->second;
		int bet_money=player->getBetMoneyResult();
		if(bet_money<0)
		{
			continue;
		}

		int result_money=floor(bet_money*role_pay_radio);
		player->setBetMoneyResult(result_money);

		int rottle_money=floor(result_money*m_gameConfig.m_winRottleFee);
		m_rottleTotalMoney+=rottle_money;

		int sys_fee=floor(result_money*m_gameConfig.m_sysFee);
        sysFee += sys_fee;
		player->incMoney(result_money-rottle_money-sys_fee);
		m_server->sendBetFlow(m_gameConfig.m_vid,player->getUid(),result_money-rottle_money-sys_fee,player->getMoney());

		if(m_mostRewardPlayer==NULL)
		{
			m_mostRewardPlayer=player;
		}
		else 
		{
			if(result_money>m_mostRewardPlayer->getBetMoneyResult())
			{
				m_mostRewardPlayer=player;
			}
		}
	}

    //暗池计算
    int m_player_proba = m_win_control.getValueFromRedis("m_player_proba");
    double val1 = roleMoneyChange * (m_player_proba / 100.0);
    m_win_control.updateToRedis("m_change_value", sysFee + val1);

    if(!m_role)
    {
        double val2 = roleMoneyChange - val1;
        m_win_control.updateToRedis("m_player_change", val2);
    }
    m_win_control.resetValue();
}

void AllKillGame::handleDeskChange()
{
	for (int i = 0; i < AK_DECKPLAYER_NU; i++)
	{
		int uid = m_descPlayers[i];
		if (uid != 0)
		{
			AllKillPlayer* pPlayer = getPlayerNoAdd(uid);
			if (pPlayer == NULL)
			{
				xt_log.warn("desk player info error %d\n.", uid);
				m_descPlayers[i] = 0;
				continue;
			}
			if (pPlayer->getDeskSeatid() > 0 && pPlayer->getDeskSeatid() < AK_DECKPLAYER_NU && pPlayer->getMoney() < m_gameConfig.m_deskMinMoney)
			{
				broadcastDeskPlayerLeave(pPlayer, LDR_NOT_ENOUGH_MONEY);
				
				m_descPlayers[pPlayer->getDeskSeatid()] = 0;
				pPlayer->setDeskSeatid(INVALID_SEATID);
			}
		}
	}
}

void AllKillGame::handleGameEnd()
{
	m_gameBetting.m_endGameTime=time(NULL);
	handlePlayerBetResult();
	handleMoneyResult();
	handleRottleResult();
	handleDeskChange();
	broadcastGameUpdate(NULL);
	broadcastGameEnd(NULL);
	sendGameInfoToSpeaker();
	sendBetPlayerResult();
	saveGameResultToSql();
}

void AllKillGame::handleGameUpdate()
{
	m_gameBetting.m_betRemainTime--;
	if(m_gameBetting.m_betRemainTime<0)
	{
		m_gameBetting.m_betRemainTime=0;
	}

	//broadcastGameUpdate(NULL);
}

int AllKillGame::setNextRole()
{
	int change_role=false;
	if(m_role)
	{
		if(m_role->getIsUnRole())
		{
			m_role=NULL;
			change_role=true;
		}
	}

	if(m_role)
	{
		int money=m_role->getMoney();
		if(money<m_gameConfig.m_roleMinMoney)
		{
			xt_log.warn("role(%d) money(%d) not enough, unrole it\n",m_role->getUid(),money);
			m_role=NULL;
			change_role=true;
		}
	}

	if(!m_role)
	{
		while(m_askRoleList.size()>0)
		{
			AllKillPlayer* player=m_askRoleList[0];
			if(player->updateMoney()<m_gameConfig.m_askRoleMinMoney)
			{
				m_askRoleList.erase(m_askRoleList.begin());
				continue;
			}
			player->setUnRole(false);
			m_role=player;
			m_askRoleList.erase(m_askRoleList.begin());
			change_role=true;
			break;
		}
	}

	if(m_role)
	{
		m_role->setTimeLock(60);
	}

	return change_role;
}




void AllKillGame::sendLoginSuccess(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"]=AK_LOGIN_SUCC_SU;
	packet.val["ret"]=0;

	packet.end();

	m_server->unicast(player,packet.tostring());

}

void AllKillGame::sendGameInfo(AllKillPlayer* player)
{

	Jpacket packet;
	packet.val["cmd"]=AK_GAME_INFO_SU;

	formatRottleFirstReward(&packet);


	packet.val["config"]["bet_base"]=m_gameConfig.m_baseMoney;
	packet.val["config"]["ask_role_min"]=m_gameConfig.m_askRoleMinMoney;
	packet.val["config"]["un_role_limit"]=m_gameConfig.m_roleMinMoney;

	packet.val["config"]["rottle_bao_zhi"]=m_gameConfig.m_rottleBaoZhiRadio;
	packet.val["config"]["rottle_shun_jin"]=m_gameConfig.m_rottleShunJinRadio;
	packet.val["config"]["rottle_jin_hua"]=m_gameConfig.m_rottleJinHuaRadio;
	packet.val["config"]["win_rottle_fee"]=m_gameConfig.m_winRottleFee;
	packet.val["config"]["rottle_min_open_money"]=m_gameConfig.m_rottleMinOpenMoney;


	packet.val["rottle_money"]=m_rottleTotalMoney;

	packet.val["status"]=m_status;


	formatAskRoleList(&packet);
	formatRole(&packet);
    formatDeskPlayer(packet);

	/* send seat infor */
	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		/*
		   packet.val["seats"][i]["name"]="test";
		   packet.val["seats"][i]["uid"]=100000+i;
		   packet.val["seats"][i]["avatar"]="gg.png";
		   */

		packet.val["seats"][i]["seat_id"]=AK_SEAT_ID_START+i;
	}

	/* player info */
	packet.val["your"]["uid"]=player->getUid();
	packet.val["your"]["money"]=player->getMoney();
	packet.val["your"]["avatar"]=player->getAvatar();
	packet.val["your"]["name"]=player->getName();
	packet.val["your"]["sex"]=player->getSex();


	if(m_status==AK_Ready)
	{
		/*do nothing */
	}
	else if(m_status==AK_Start)
	{
		for(int i=0;i<AK_SEAT_ID_NU;i++)
		{
			packet.val["bet_info"][i]["seat_id"]=i+AK_SEAT_ID_START;
			packet.val["bet_info"][i]["seat_bet_nu"]=m_gameBetting.m_seatBet[i];
			packet.val["bet_info"][i]["your_bet_nu"]=player->getSeatBet(i+AK_SEAT_ID_START);
		}

		packet.val["remain_time"]=m_gameBetting.m_betRemainTime;
	}
	else if (m_status==AK_EndGame)
	{
		formatGameResult(&packet);
	}

	int i=0;
	for(std::deque<GameResultInfo>::iterator iter=m_gameResultInfo.begin();iter!=m_gameResultInfo.end();++iter)
	{
		for(int j=0;j<AK_SEAT_ID_NU;j++)
		{
			//packet.val["game_history"][i][j]["seat_id"]=AK_SEAT_ID_START+j;
			packet.val["game_history"][i][j]=(*iter).m_seatid[j];
		}
		i++;
	}


	packet.end();

	m_server->unicast(player,packet.tostring());
}


void AllKillGame::broadcastGameReady(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"]=AK_GAME_READY_SB;

	packet.val["rottle_money"]=m_rottleTotalMoney;

	formatRole(&packet);
	formatAskRoleList(&packet);
	
	packet.end();
	m_server->broadcast(player,packet.tostring());
}


void AllKillGame::broadcastDeskPlayerLeave(AllKillPlayer* player, int reason)
{
	Jpacket packet;

	packet.val["cmd"]=AK_DESK_LEAVE_SB;

	packet.val["uid"] = player->getUid();
	packet.val["desk_seatid"] = player->getDeskSeatid();
	packet.val["reason"] = reason;
	
	packet.end();
	m_server->broadcast(NULL,packet.tostring());
}

void AllKillGame::broadcastGameStart(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"]=AK_GAME_START_SB;
	packet.val["remain_time"]=m_gameBetting.m_betRemainTime;
	packet.val["rottle_money"]=m_rottleTotalMoney;
	packet.end();
	m_server->broadcast(player,packet.tostring());
}


void AllKillGame::broadcastGameEnd(AllKillPlayer* player)
{
	Jpacket packet;

	packet.val["cmd"]=AK_GAME_END_SB;

	formatRottleFirstReward(&packet);
	formatGameResult(&packet);

	packet.val["rottle_money"]=m_rottleTotalMoney;
	packet.end();

	m_server->broadcast(player,packet.tostring());
}

void AllKillGame::broadcastAskRoleChange(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"]=AK_ASK_ROLE_LIST_CHANGE_SB;
	formatAskRoleList(&packet);
	formatRole(&packet);
	packet.end();
	m_server->broadcast(player,packet.tostring());

}

void AllKillGame::sendGameInfoToSpeaker()
{
	if(m_rottleResult.m_hasOpenRottle)
	{
		char buf[2048];
		int money_w=m_rottleFirstReward.m_rewardMoney/10000;
		if(money_w >10)
		{
			sprintf(buf,"万人场奖池开奖，恭喜%s获得头奖，%d万奖金",m_rottleFirstReward.m_name.c_str(),money_w);
			m_server->sendSpeaker(4006,0,"系统",buf);
		}
	}


	if(m_mostRewardPlayer)
	{
		char buf[2048];
		int result_money=m_mostRewardPlayer->getBetMoneyResult();
		int money_w=result_money/10000;
		if(money_w>500)
		{
			sprintf(buf,"%s在万人场中胆识超群，天量下注赢得%d万金币车载斗量满载而归",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"系统",buf);
		}
		else if(money_w>100)
		{
			sprintf(buf,"%s在万人场中面不改色，疯狂下注赢得%d万金币终将一树百获",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"系统",buf);
		
		}
		else if(money_w>50)
		{
			sprintf(buf,"%s在万人场中胆大心细，赢得%d万金币稳吃三注",m_mostRewardPlayer->getName(),money_w);
			m_server->sendSpeaker(4006,0,"系统",buf);
		}
	}
}

void AllKillGame::sendBetPlayerResult()
{
	for(std::map<int,AllKillPlayer*>::iterator iter=m_betPlayers.begin();
			iter!=m_betPlayers.end();
			++iter)
	{

		Jpacket p_packet;
		p_packet.val["cmd"]=AK_PLAYER_BET_REWARD_SU;
		p_packet.val["reward"]=iter->second->getBetMoneyResult();
		p_packet.val["money"]=iter->second->getMoney();
		p_packet.end();
		m_server->unicast(iter->second,p_packet.tostring());
	}

	if(m_role)
	{
		Jpacket p_packet;
		p_packet.val["cmd"]=AK_PLAYER_BET_REWARD_SU;

		p_packet.val["reward"]=m_roleResult.m_roleResultBetMoney;
		p_packet.val["money"]=m_role->getMoney();

		p_packet.end();

		m_server->unicast(m_role,p_packet.tostring());
	}

}


void AllKillGame::broadcastGameUpdate(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"]=AK_GAME_UPDATE_SB;

	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		packet.val["seat_bet_info"][i]["seat_id"]=i+AK_SEAT_ID_START;
		packet.val["seat_bet_info"][i]["bet_nu"]=m_gameBetting.m_seatBet[i];
	}

	packet.val["remain_time"]=m_gameBetting.m_betRemainTime;
	packet.end();
	m_server->broadcast(NULL,packet.tostring());

}


void AllKillGame::sendBetError(AllKillPlayer* player,int code,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]=AK_PLAYER_BET_RESULT_SU;
	packet.val["ret"]=code;
	packet.val["desc"]=desc;
	packet.end();

	m_server->unicast(player,packet.tostring());
}


void AllKillGame::sendBetSuccess(AllKillPlayer* player,int seat_id)
{
	Jpacket packet;

	packet.val["cmd"]=AK_PLAYER_BET_RESULT_SU;
	packet.val["ret"]=0;

	packet.val["bet_info"]["seat_id"]=seat_id;
	packet.val["bet_info"]["seat_bet_nu"]=m_gameBetting.m_seatBet[seat_id-AK_SEAT_ID_START];
	packet.val["bet_info"]["your_bet_nu"]=player->getSeatBet(seat_id);
	packet.val["money"]=player->getMoney();

	packet.end();

	m_server->unicast(player,packet.tostring());
}

void AllKillGame::CheckBroadcastDeskBetInfo(AllKillPlayer* player, int seat_id)
{
	if (player->getDeskSeatid() == INVALID_SEATID)
	{
		return;
	}
	
	xt_log.debug("player(%d) broadcast deskbetinfo seatid:%d\n",player->getUid(), player->getDeskSeatid());

	Jpacket packet;

	packet.val["cmd"] = AK_DESK_BET_SB;
	packet.val["uid"] = player->getUid();
	packet.val["desk_seat_id"] = player->getDeskSeatid();
	
	packet.val["bet_info"]["seat_id"] = seat_id;
	packet.val["bet_info"]["seat_bet_nu"]=m_gameBetting.m_seatBet[seat_id-AK_SEAT_ID_START];
	packet.val["bet_info"]["your_bet_nu"]=player->getSeatBet(seat_id);
	
	packet.val["money"]=player->getMoney();

	packet.end();

	m_server->broadcast(NULL,packet.tostring());
}

void AllKillGame::sendAskRoleSuccess(AllKillPlayer* player)
{

	Jpacket packet;
	packet.val["cmd"]=AK_ASK_ROLE_RESULT_SU;
	packet.val["ret"]=0;

	formatAskRoleList(&packet);
	formatRole(&packet);

	packet.end();

	/* unicast */
	m_server->unicast(player,packet.tostring());


	/* broadcast  */
	packet.val["cmd"]=AK_ASK_ROLE_LIST_CHANGE_SB;
	packet.end();

	m_server->broadcast(player,packet.tostring());
}



void AllKillGame::sendAskRoleError(AllKillPlayer* player,int code ,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]=AK_ASK_ROLE_RESULT_SU;
	packet.val["ret"]=code;
	packet.val["desc"]=desc;

	packet.end();
	m_server->unicast(player,packet.tostring());
}


void AllKillGame::sendUnRoleSuccess(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"]= AK_ASK_UN_ROLE_RESULT_SU;
	packet.val["ret"]=0;

	formatAskRoleList(&packet);
	formatRole(&packet);

	packet.end();

	/* unicast */
	m_server->unicast(player,packet.tostring());

	/* broadcast  */
	packet.val["cmd"]=AK_ASK_ROLE_LIST_CHANGE_SB;
	packet.end();

	m_server->broadcast(player,packet.tostring());
}




void AllKillGame::sendUnRoleErr(AllKillPlayer* player,int code,const std::string& desc)
{
	Jpacket packet;
	packet.val["cmd"]=AK_ASK_UN_ROLE_RESULT_SU;
	packet.val["ret"]=code;
	packet.val["desc"]=desc;
	packet.end();

	m_server->unicast(player,packet.tostring());
}

void AllKillGame::sendPlayerLogoutSuccess(AllKillPlayer* player)
{
	Jpacket packet;
	packet.val["cmd"]=AK_LOGOUT_SUCC_SU;
	packet.val["ret"]=0;
	packet.end();
	m_server->unicast(player,packet.tostring());
}


void AllKillGame::sendPlayerChat(AllKillPlayer* player,const std::string& chat) 
{
	Jpacket packet;
	packet.val["cmd"]=AK_CHAT_SB;
	packet.val["uid"]=m_gameConfig.m_chatRoleUid;
	packet.val["name"]=m_gameConfig.m_chatRoleName;
	packet.val["avatar"]=m_gameConfig.m_chatRoleAvatar;
	packet.val["sex"]=m_gameConfig.m_chatRoleSex;
	packet.val["content"]=chat;
	packet.end();

	m_server->unicast(player,packet.tostring());
}

void AllKillGame::formatRole(Jpacket* packet)
{
	if(m_role)
	{
		packet->val["role"]["name"]=m_role->getName();
		packet->val["role"]["money"]=m_role->getMoney();
		packet->val["role"]["avatar"]=m_role->getAvatar();
		packet->val["role"]["uid"]=m_role->getUid();
		packet->val["role"]["sex"]=m_role->getSex();
		packet->val["role"]["next_unrole"]=m_role->getIsUnRole()? 1:0;
	}
	else 
	{
		packet->val["role"]["name"]=m_gameConfig.m_sysRoleName;
		packet->val["role"]["money"]=m_gameConfig.m_sysRoleMoney;
		packet->val["role"]["avatar"]=m_gameConfig.m_sysRoleAvatar;
		packet->val["role"]["uid"]=m_gameConfig.m_sysRoleUid;
		packet->val["role"]["sex"]=m_gameConfig.m_sysRoleSex;
		packet->val["role"]["next_unrole"]=0;
	}

}

void AllKillGame::formatAskRoleList(Jpacket* packet)
{
	int ask_role_nu=m_askRoleList.size();
	for(int i=0;i<ask_role_nu;i++)
	{
		AllKillPlayer* player=m_askRoleList[i];
		packet->val["ask_roles"][i]["name"]=player->getName();
		packet->val["ask_roles"][i]["uid"]=player->getUid();
		packet->val["ask_roles"][i]["avatar"]=player->getAvatar();
		packet->val["ask_roles"][i]["money"]=player->getMoney();
		packet->val["ask_roles"][i]["sex"]=player->getSex();
	}
}

void AllKillGame::formatDeskPlayer(Jpacket& packet)
{	
	packet.val["deskplayer"].resize(0);
	
    AllKillPlayer* player = NULL;
    for(int i = 0, j = 0; i < AK_DECKPLAYER_NU; ++i)
    {
		player = getPlayerNoAdd(m_descPlayers[i]);
        if(NULL == player)
        {
            continue;
        }
		
		packet.val["deskplayer"][j]["name"]=player->getName();
		packet.val["deskplayer"][j]["uid"]=player->getUid();
		packet.val["deskplayer"][j]["avatar"]=player->getAvatar();
		packet.val["deskplayer"][j]["money"]=player->getMoney();
		packet.val["deskplayer"][j]["sex"]=player->getSex();
		packet.val["deskplayer"][j]["seatid"]=i;
		
		++j;
    }
}

void AllKillGame::formatRottleFirstReward(Jpacket* packet)
{
	packet->val["rottle_first_reward"]["name"]=m_rottleFirstReward.m_name;
	packet->val["rottle_first_reward"]["avatar"]=m_rottleFirstReward.m_avatar;
	packet->val["rottle_first_reward"]["sex"]=m_rottleFirstReward.m_sex;
	packet->val["rottle_first_reward"]["uid"]=m_rottleFirstReward.m_uid;
	packet->val["rottle_first_reward"]["reward_money"]=m_rottleFirstReward.m_rewardMoney;

}

void AllKillGame::formatGameResult(Jpacket* packet)
{
	int remain_time=0;
	if(m_rottleResult.m_hasOpenRottle)
	{
		remain_time=AK_END_GAME_TIME-(time(NULL)-m_gameBetting.m_endGameTime)+5;
	}
	else 
	{
		remain_time=AK_END_GAME_TIME-(time(NULL)-m_gameBetting.m_endGameTime);
	}

	if(remain_time<0)
	{
		remain_time=0;
	}

	packet->val["remain_time"]=remain_time;
	packet->val["has_open_rottle"]=m_rottleResult.m_hasOpenRottle?1:0;

	int rottle_total_money=m_rottleResult.m_roleRottleMoney;

	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		rottle_total_money+=m_rottleResult.m_seatRottleMoney[i];
	}

	packet->val["rottle_total_money"]=rottle_total_money;



	/* format role info */
	for(unsigned int i=0;i<m_gameBetting.m_roleCard.m_cards.size();i++)
	{
		packet->val["role_info"]["card"].append(m_gameBetting.m_roleCard.m_cards[i].m_value);
	}


	packet->val["role_info"]["rottle_money"]=m_rottleResult.m_roleRottleMoney;
	packet->val["role_info"]["card_type"]=m_roleResult.m_cardType;
	packet->val["role_info"]["reward_money"]=m_roleResult.m_roleResultBetMoney;
	packet->val["role_info"]["bet_times"]=m_roleResult.m_betTime;

	if(m_role)
	{
		packet->val["role_info"]["money"]=m_role->getMoney();
	}
	else 
	{
		packet->val["role_info"]["money"]=m_gameConfig.m_sysRoleMoney;
	}



	formatRole(packet);




	/* seat info */
	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		packet->val["seat_info"][i]["seat_id"]=i+AK_SEAT_ID_START;
		packet->val["seat_info"][i]["rottle_money"]=m_rottleResult.m_seatRottleMoney[i];
		packet->val["seat_info"][i]["is_win"]=m_seatResult[i].m_isWin;
		packet->val["seat_info"][i]["card_type"]=m_seatResult[i].m_cardType;
		packet->val["seat_info"][i]["total_bet_nu"]=m_seatResult[i].m_totalBetNu;


		for(unsigned int j=0;j<m_gameBetting.m_seatCard[i].m_cards.size();j++)
		{
			packet->val["seat_info"][i]["card"].append(m_gameBetting.m_seatCard[i].m_cards[j].m_value);
		}


		if(m_seatResult[i].m_isWin)
		{
			packet->val["seat_info"][i]["bet_times"]=m_seatResult[i].m_betTime;
		}
		else 
		{
			packet->val["seat_info"][i]["bet_times"]=m_roleResult.m_betTime;
		}


		int size=m_seatResult[i].m_betPlayerList.size();
		if(m_rottleResult.m_seatRottleMoney[i]>0)
		{

			if(size>4)
			{
				size=4;
			}
		}
		else 
		{
			if(size>1)
			{
				size=1;
			}
		}


		for(int j=0;j<size;j++)
		{
			AllKillPlayer* player=m_seatResult[i].m_betPlayerList[j];
			packet->val["seat_info"][i]["player_list"][j]["name"]=player->getName();
			packet->val["seat_info"][i]["player_list"][j]["uid"]=player->getUid();
			packet->val["seat_info"][i]["player_list"][j]["avatar"]=player->getAvatar();
			packet->val["seat_info"][i]["player_list"][j]["sex"]=player->getSex();
			packet->val["seat_info"][i]["player_list"][j]["bet_nu"]=player->getSeatBet(i+AK_SEAT_ID_START);
		}
	}
}

void AllKillGame::saveGameResultToSql()
{
	if(m_status!=AK_EndGame)
	{
		xt_log.error("status error for saveGameResultToSql\n");
		return;
	}

	int online_people=m_loginPlayers.size();

	time_t time_now=time(NULL);

	std::string table_info="(  create_time, vid, online_people, bet_people,rottle_money,rottle_open_money,role_id,role_tcard, role_rcard, role_money, id1_people, id1_betnu, id1_tcard, id1_rcard, id1_iswin,id1_money, id2_people, id2_betnu, id2_tcard, id2_rcard, id2_iswin,id2_money, id3_people, id3_betnu, id3_tcard, id3_rcard,id3_iswin, id3_money, id4_people, id4_betnu, id4_tcard, id4_rcard,id4_iswin, id4_money)" ;

	std::string value="";

	char buf[1024];

	int rottle_open=0;

	if(m_rottleResult.m_hasOpenRottle)
	{
		rottle_open+=m_rottleResult.m_roleRottleMoney;

		for(int i=0;i<AK_SEAT_ID_NU;i++)
		{
			rottle_open+=m_rottleResult.m_seatRottleMoney[i];
		}
	}


	int bet_people=m_betPlayers.size();
	sprintf(buf,"%ld,%d,%d,%d,%d,%d",time_now,
			m_gameConfig.m_vid,
			online_people,
			bet_people,
			m_rottleTotalMoney,
			rottle_open
		   );
	value+=buf;

	int role_id=m_gameConfig.m_sysRoleUid;
	if(m_role)
	{
		role_id=m_role->getUid();
	}

	sprintf(buf,"%d,%d,%d,%d",
			role_id,
			m_roleResult.m_cardType,
			m_roleResult.m_betTime,
			m_roleResult.m_roleResultBetMoney);

	value+=std::string(",")+buf;

    //xt_log.info("role money: %d\n", m_roleResult.m_humanResult);

	for(int i=0;i<AK_SEAT_ID_NU;i++)
	{
		int bet_money=m_seatResult[i].getHumanBet(i)*m_gameConfig.m_baseMoney;
		if(m_seatResult[i].m_isWin)
		{
			bet_money=bet_money*m_seatResult[i].m_betTime;
		}
		else 
		{
			bet_money=-bet_money*m_roleResult.m_betTime;
		}

        //xt_log.info("humanBet: %d, humanMoney: %d\n", m_seatResult[i].getHumanBet(i), bet_money);
		sprintf(buf,",%d,%d,%d,%d,%d,%d",
				m_seatResult[i].getHumanSize(),
				m_seatResult[i].getHumanBet(i),
				m_seatResult[i].m_cardType,
				m_seatResult[i].m_betTime,
				m_seatResult[i].m_isWin,
				bet_money);

		value+=buf;
	}
	value=" value("+value+")";

	std::string sql=std::string("insert allkill_log")+table_info+value;

	//printf("sql:%s\n",sql.c_str());

	int ret=m_server->getSqlClient()->query(sql.c_str());

	if(ret!=0)
	{
		xt_log.error("sql query insert GameResultInfo error.\n");
	}
}
        
