#include "bull_game_logic.h"
#include <time.h>
#include <iterator>
#include <map>
#include <vector>
#include<string.h>
using namespace std;

CBullGameLogic* CBullGameLogic::GetInstance()
{
	static CBullGameLogic instance;
	return &instance;
}

//逻辑类  ///---我的组合
/*
My_zuhe(11, 5, CT_BULL_EIGHT);
My_zuhe(11, 5, CT_FIVE_H_BULL);
My_zuhe(11, 5, CT_FIVE_S_BULL);
My_zuhe(8,  5, CT_BOMB);
*/
/*
    我的组合（做一些牌）
	取出牌型为：big_type, 张数r, 范围[1, n]
*/
void CBullGameLogic::My_zuhe(int n,int r, int big_type) 
{  
	///map (int->vector)
	multimap<int,vector<int> > mm,mm2;   
	multimap<int,vector<int> >::iterator pos;   ///map迭代器

	vector<int> nn,temp,bb;  

	///二维数组
	vector<vector<int> > target;         
	vector<int>::iterator ita,ita_bb;  
	typedef pair<int,vector<int> > pr;      ///pair类型
	for(int i=1;i<=n;i++)   
		bb.push_back(i);     ///bb放入[1, n]
	if(r>n)   
		return;

	int k;      
	ita_bb=bb.begin();  
	temp.push_back(*ita_bb);  ///放入首个字符
	mm.insert(pr(temp.size(),temp));  ///放入（1,1）
	temp.clear();             ///---清空

	++ita_bb;  
	while(ita_bb != bb.end())   ///---[2, n]
	{   
		k=*ita_bb;   
		for(pos=mm.begin();pos!=mm.end();++pos)   
		{    
			temp=pos->second;    ///---取得map中的vector
			temp.push_back(k);    
			mm2.insert(pr(temp.size(),temp));    
			temp.clear();   
		} 

		for(pos=mm2.begin();pos!=mm2.end();++pos)    
			mm.insert(*pos);  
		mm2.clear();    
		temp.push_back(k);   
		mm.insert(pr(temp.size(),temp));   
		temp.clear();   
		++ita_bb;
	}
	for(pos=mm.begin();pos != mm.end();++pos)  
	{   
		if(pos->first == r)   
		{    
			target.push_back(pos->second);     ///---随机组合的牌
		}  
	}
	//sort(target.begin(),target.end());  
	vector<vector<int> >::iterator tt_pos;  
	//cout<<"In all we have "<<target.size()<<" kinds, and they are:"<<endl;  
	/*
		[0]	[5](1,2,3,4,5)	std::vector<int,std::allocator<int> >
		[1]	[5](1,2,3,4,6)	std::vector<int,std::allocator<int> >
		[2]	[5](1,2,3,5,6)	std::vector<int,std::allocator<int> >
		[3]	[5](1,2,4,5,6)	std::vector<int,std::allocator<int> >
		[4]	[5](1,3,4,5,6)	std::vector<int,std::allocator<int> >
		[5]	[5](2,3,4,5,6)	std::vector<int,std::allocator<int> >
	*/

	int index = 0;

	//混乱准备
	int count = 0;
	_uint8 cbCardData[CARDCOUNT] = {0};   ///---按牌型加载对应的牌
	switch(big_type)
	{
	case CT_BULL_EIGHT:   ///---牛8
		memcpy(cbCardData, m_cbCardData, CARDCOUNT);   ///初始为所有52张牌
		RandCardList(cbCardData, CARDCOUNT);
		count = 150;
		break;
	case CT_BOMB:         ///---炸弹
		memcpy(cbCardData, m_cbZCardData, 8);
		count = 10;
		break;
	case CT_FIVE_H_BULL:
		memcpy(cbCardData, m_cbHCardData, 11);
		count = 6;
		break;
	case CT_FIVE_S_BULL:
		memcpy(cbCardData, m_cbSCardData, 11);
		count = 3;
		break;
	default:
		break;
	}

	_uint8 card_data[MAX_COUNT] = { 0 };   ///---手牌
	int card_count = 0;
	for(tt_pos=target.begin(), k=0; tt_pos != target.end(); ++tt_pos, k++)  ///---遍历二维数组
	{     
		index = 0;
		for(ita=tt_pos->begin();ita != tt_pos->end();++ita)    ///---取里面的一维数组
		{
			if (*ita - 1 < 0)
			{
				break;
			}

			card_data[index] = cbCardData[*ita - 1];
			index++;
		}
		if (index != MAX_COUNT)
			continue;

		if(CBullGameLogic::GetInstance()->GetCardType(card_data, MAX_COUNT) >= big_type)
		{
			if (card_count >= count)
				return;

			memcpy(CARD_TYPE_DATA[CARD_TYPE_COUNT], card_data, MAX_COUNT);
			CARD_TYPE_COUNT++;
			card_count++;
		}
	}
}

//----好牌牌型-----
void CBullGameLogic::regis_goods_card_type()
{
	//普通好牌
	CARD_TYPE_COUNT = 0;
	memset(CARD_TYPE_DATA, 0, sizeof(CARD_TYPE_DATA));
	My_zuhe(11, 5, CT_BULL_EIGHT);
	My_zuhe(11, 5, CT_FIVE_H_BULL);
	My_zuhe(11, 5, CT_FIVE_S_BULL);
	My_zuhe(8,  5, CT_BOMB);
}

CBullGameLogic::CBullGameLogic(void)
{
	
}

CBullGameLogic::~CBullGameLogic(void)
{
}

//获取类型（返回牌的类型）
_uint8 CBullGameLogic::GetCardType(_uint8 cbCardData[], _uint8 cbCardCount)
{
    //ASSERT(cbCardCount == MAX_COUNT);

    _uint8 bKingCount = 0, bLessFiveCount = 0, bBombCount = 1;
    _uint8 bTemp[MAX_COUNT];
    _uint8 bSum = 0;
    for(_uint8 i = 0; i < cbCardCount; i++)
    {
        bTemp[i] = GetCardLogicValue(cbCardData[i]);
        bSum += bTemp[i];
        if(GetCardValue(cbCardData[i]) > 10)
        {
            bKingCount++;
        }
        if (GetCardValue(cbCardData[i]) < 5)
        {
            bLessFiveCount++;
        }
    }

    if (bSum <= 10 && bLessFiveCount == MAX_COUNT)
		return CT_FIVE_S_BULL; //五小牛

    if(bKingCount == MAX_COUNT) 
		return CT_FIVE_H_BULL; //五花牛

    for (_uint8 i = 0; i < cbCardCount - 1; i++)
    {
        bBombCount = 1;
        for (_uint8 j = i + 1; j < cbCardCount; j++)
        {
            if (GetCardValue(cbCardData[i]) == GetCardValue(cbCardData[j]))
            {
                bBombCount++;
            }
        }
        if (bBombCount == MAX_COUNT - 1)
        {
            return CT_BOMB;//炸弹
        }
    }

    for (_uint8 i = 0; i < cbCardCount - 1; i++)
    {
        for (_uint8 j = i + 1; j < cbCardCount; j++)
        {

            if((bSum - bTemp[i] - bTemp[j]) % 10 == 0)
            {
                return ((bTemp[i] + bTemp[j]) > 10) ? (bTemp[i] + bTemp[j] - 10) : (bTemp[i] + bTemp[j]);
            }
        }
    }
    return CT_NOBULL;
}

//获取牌型倍数
int CBullGameLogic::GetTimes(_uint8 cbCardData[], _uint8 cbCardCount)
{
    if(cbCardCount != MAX_COUNT)   ///不是5张牌
		return 0;

	///---牌型
    _uint8 bTimes = GetCardType(cbCardData, MAX_COUNT);

	if(bTimes <= CT_BULL_SIX)        ///牛六到牛一,无牛
		return 1;
	else if(bTimes <= CT_BULL_NINE)  ///牛九到牛七
		return 2;	
	else if(bTimes == CT_BULL_BULL)  ///牛牛
		return  3;
	else if(bTimes == CT_BOMB)       ///四炸
		return 4;
	else if(bTimes == CT_FIVE_H_BULL) ///五花牛
		return  5;
	else if(bTimes == CT_FIVE_S_BULL) ///五小牛
		return  8;

    return 0;
}

//获取最大牌型数据(系统组牌)
/*
  param: cbOutCardData
  return: 牌型值
*/
_uint8 CBullGameLogic::GetCardTypeAndData(_uint8 cbCardData[], _uint8 cbCardCount, _uint8 cbOutCardData[])
{
	_uint8 CardData[MAX_COUNT];
	memset(CardData, 0, sizeof(CardData));
	memcpy(CardData, cbCardData, sizeof(CardData));

	///牌型
	_uint8 UserOxType = GetCardType(CardData, MAX_COUNT);

	///组牌
	_uint8 GropCard[MAX_COUNT];
	memset(GropCard, 0, sizeof(GropCard));

	if (UserOxType < CT_BOMB && UserOxType > CT_NOBULL)
	{
		//组合牛数据
		GroupCard(CardData, GropCard, MAX_COUNT);
	}
	else
	{
		memcpy(GropCard, CardData, sizeof(CardData));
	}
	
	//如果牌型为炸弹,排序
	if(UserOxType == CT_BOMB)
		SortBom(CardData, GropCard, MAX_COUNT);
	
	memcpy(cbOutCardData, GropCard, sizeof(GropCard));
	return UserOxType;
}

//获得选牌牌型（返回此牌牌型）
_uint8 CBullGameLogic::GetChoiceType(_uint8 cbCardData[], _uint8 cbCardCount, _uint8 IsGive)
{
	if (IsGive)   ///放弃
	{
		return CT_NOBULL;
	}
	
	///把数据暂存起来
	_uint8 CardData[MAX_COUNT];
	memset(CardData, 0, sizeof(CardData));
	memcpy(CardData, cbCardData, sizeof(CardData));

	if (cbCardData[0] == 0 &&
		cbCardData[1] == 0 &&
		cbCardData[2] == 0 &&
		cbCardData[3] == 0 &&
		cbCardData[4] == 0)
	{
		return CT_NOBULL;
	}

	///牌型
	_uint8 UserOxType = GetCardType(CardData, MAX_COUNT);
	if (UserOxType >= CT_BOMB)
	{
		return UserOxType;
	}

	///前三张牌数值之和
	int value = 0;
	int card_value = 0;
	for (int i = 0; i < 3; i++)
	{
		card_value = GetCardValue(cbCardData[i]);
		card_value = (card_value > 10 ? 10 : card_value);
		value += card_value;
	}
	
	if (value % 10 != 0)
		return CT_NOBULL;

	///有牛
	value = 0;
	for (int i = 3; i < 5; i++)
	{
		card_value = GetCardValue(cbCardData[i]);
		card_value = (card_value > 10 ? 10 : card_value);
		value += card_value;
	}

	if(value % 10 == 0)
	{
		return CT_BULL_BULL;
	}
	return (value % 10);
}

//获取牛牛///---没用到
bool CBullGameLogic::GetOxCard(_uint8 cbCardData[], _uint8 cbCardCount)
{
  //  ASSERT(cbCardCount == MAX_COUNT);

    //设置变量
    _uint8 bTemp[MAX_COUNT], bTempData[MAX_COUNT];
    memcpy(bTempData, cbCardData, sizeof(bTempData));

    _uint8 bSum = 0;
    for (_uint8 i = 0; i < cbCardCount; i++)
    {
        bTemp[i] = GetCardLogicValue(cbCardData[i]);   ///获取牌的逻辑值，大于10的都为10
        bSum += bTemp[i];
    }

    //查找牛牛
    for (_uint8 i = 0; i < cbCardCount - 1; i++)
    {
        for (_uint8 j = i + 1; j < cbCardCount; j++)
        {
            if((bSum - bTemp[i] - bTemp[j]) % 10 == 0)
            {
                _uint8 bCount = 0;
                for (_uint8 k = 0; k < cbCardCount; k++)
                {
                    if(k != i && k != j)
                    {
                        cbCardData[bCount++] = bTempData[k];
                    }
                }
               // ASSERT(bCount == 3);

                cbCardData[bCount++] = bTempData[i];
                cbCardData[bCount++] = bTempData[j];
                return true;
            }
        }
    }
    return false;
}

//获取整数(即：是否为10的倍数)
bool CBullGameLogic::IsIntValue(_uint8 cbCardData[], _uint8 cbCardCount)
{
    _uint8 sum = 0;
    for(_uint8 i = 0; i < cbCardCount; i++)
    {
        sum += GetCardLogicValue(cbCardData[i]);
    }
   // ASSERT(sum > 0);
    return (sum % 10 == 0);
}

//排列扑克
void CBullGameLogic::SortCardList(_uint8 cbCardData[], _uint8 cbCardCount)
{
	//转换数值
	_uint8 cbLogicValue[MAX_COUNT];
	for (_uint8 i=0;i<cbCardCount;i++) 
	{
		cbLogicValue[i]=GetCardValue(cbCardData[i]);
		//if(cbLogicValue[i] == 1)
		//	cbLogicValue[i] = 14;    ///为什么等于1，转换成14
	}

	//排序操作  ///冒泡排序
	bool bSorted=true;
	_uint8 cbTempData,bLast=cbCardCount-1;
	do
	{
		bSorted=true;
		for (_uint8 i=0;i<bLast;i++)
		{
			if ((cbLogicValue[i]<cbLogicValue[i+1])||
				((cbLogicValue[i]==cbLogicValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
			{
				//交换位置
				cbTempData=cbCardData[i];
				cbCardData[i]=cbCardData[i+1];
				cbCardData[i+1]=cbTempData;
				cbTempData=cbLogicValue[i];
				cbLogicValue[i]=cbLogicValue[i+1];
				cbLogicValue[i+1]=cbTempData;
				bSorted=false;
			}	
		}
		bLast--;
	} while(bSorted==false);

	return;
}


///---没用到
void CBullGameLogic::SortCardDataList(_uint8 cbCardData[], _uint8 cbCardCount)
{
    for (int j = 0; j < cbCardCount - 1; j++)
    {
        for (int i = j + 1; i < cbCardCount; i++)
        {
            if (cbCardData[j] < cbCardData[i])
            {
                cbCardData[j] ^= cbCardData[i] ^= cbCardData[j] ^= cbCardData[i];
            }
        }
    }
}

//排列炸弹
void CBullGameLogic::SortBom(_uint8 cbCardBuffer[], _uint8 cbCardOut[], _uint8 cbBufferCount)
{
    _uint8 card[MAX_COUNT];
    memset(card, 0, sizeof(card));
    memcpy(card, cbCardBuffer, MAX_COUNT);

    _uint8 CardNalve = 0;  ///炸弹值
    _uint8 bBombCount;
    for (_uint8 i = 0; i < cbBufferCount - 1; i++)
    {
        bBombCount = 1;
        for (_uint8 j = i + 1; j < cbBufferCount; j++)
        {
            if (GetCardValue(card[i]) == GetCardValue(card[j]))
            {
                //获取炸弹值
                CardNalve = GetCardValue(card[i]);
                bBombCount++;
                break;
            }
        }
        if (bBombCount > 1)
        {
            break;
        }
    }

    _uint8 index = 0;
    _uint8 LastCard = 0;
    for (_uint8 n = 0; n < cbBufferCount; n++)
    {
        if (GetCardValue(card[n]) == CardNalve)
        {
            cbCardOut[index] = card[n];  ///把炸弹的牌，排在前4个位置
            index++;
        }
        else
        {
            LastCard = card[n];
        }
    }
    cbCardOut[MAX_COUNT - 1] = LastCard;  ///另外一张排最后 

}

//混乱扑克  ///---把cbCardBuffer[]打乱
void CBullGameLogic::RandCardList(_uint8 cbCardBuffer[], _uint8 cbBufferCount)
{
    //混乱准备
    _uint8 cbCardData[CARDCOUNT] = {0};
    memcpy(cbCardData, cbCardBuffer, cbBufferCount);

    //混乱扑克
	srand(time(0));
    _uint8 bRandCount = 0, bPosition = 0;
    do
    {
        bPosition = rand() % (cbBufferCount - bRandCount);
        cbCardBuffer[bRandCount++] = cbCardData[bPosition];
        cbCardData[bPosition] = cbCardData[cbBufferCount - bRandCount];  ///随机交换
    }
    while (bRandCount < cbBufferCount);
    return;
}

//好牌算法
int CBullGameLogic::GoodRandCardList(
		_tint32 count[],        ///---玩家的局数
		bool is_robot[],        ///---是不是机器人
		_uint8 cbCardBuffer[],  ///---空（应该是剩余的牌）
		_uint8 cbBufferCount,   ///---牌的张数
		_uint8 cbPlayerCount,   ///---玩家数
		_uint8 GoodCardBuff[GAME_PLAYER_COUNT][MAX_COUNT], ///好牌buffer
		_uint8 GoodChairId[]                               ///好牌椅子id
  )
{
	//todo no use good card
/*	tagGoodCardCfg GoodGfg = conf_mgr::GetInstance()->GetGoodCardCfg();

	//混乱准备
	_uint8 cbCardData[CARDCOUNT] = {0};
	memcpy(cbCardData, m_cbCardData, CARDCOUNT);   ///---初始化牌

	_uint8  TYPE_DATA[MAX_GOOD_COUNT][5];          ///---全部手牌数目
	memset(TYPE_DATA, 0, sizeof(TYPE_DATA));
	memcpy(TYPE_DATA, CARD_TYPE_DATA, MAX_GOOD_COUNT * MAX_COUNT);    ///CARD_TYPE_DATA 我的组合赋值
	int Good_Type_Count = CARD_TYPE_COUNT;   ///好牌数

	//给新玩家挑好牌
	int chair_count = 0;
	int left_count = CARDCOUNT;   ///---剩下的牌数
	srand(time(0));

	for(int i = 0; i < cbPlayerCount; i++)
	{
		if (count[i] == -1)   ///---玩家的总局数为-1，说明玩家为空,跳过
			continue;

		int is_good = rand() % 100;   ///好牌随机值
		int good_value = 0;
		
		if (is_robot[i])   ///---为机器人
		{
			good_value = GoodGfg.wRobotRate;
		}
		else
		{
			if (count[i] == 0)  ///---一局都没玩
				good_value = GoodGfg.wNewUserFirst;		//新注册用户
			else
				if (count[i] >= 1 && count[i] <= GoodGfg.wNewUserDefine)
					good_value = GoodGfg.wNewUserNext;	//玩过一段时间的新用户
				else 
					good_value = GoodGfg.wOldUser;		//老用户
		}
			
		if (is_good <= good_value)   ///---小于配置的好牌值
		{
			int id = rand() % Good_Type_Count;   ///---随机选择一好牌

			//把好牌存起来
			memcpy(GoodCardBuff[i], TYPE_DATA[id], MAX_COUNT);  ///--该位置的好牌
			GoodChairId[i] = i;   ///---位置ID
			chair_count++;        ///---位置个数累加

			RemoveCard(
				TYPE_DATA[id], 
				MAX_COUNT, 
				cbCardData, 
				left_count);

			left_count -= MAX_COUNT;

			//把挑选的牌从牌堆移除 + 把好牌移除
			//把用到这5个牌的牌型移除，防止发重复牌
			for (int m = 0; m < Good_Type_Count; )
			{
				if ( IsHaveCardList(TYPE_DATA[m], MAX_COUNT, TYPE_DATA[id], MAX_COUNT) )
				{
					memcpy(TYPE_DATA[m], TYPE_DATA[Good_Type_Count - 1], MAX_COUNT);
					Good_Type_Count--;
				}
				else
				{
					m++;
				}
			}
			
			if (Good_Type_Count == 0)
				break;
		}
	}

	//洗牌后给老玩家发牌
	RandCardList(cbCardData, left_count);
	memcpy(cbCardBuffer, cbCardData, left_count);
	return chair_count;*/
	return -1;
}

//组合扑克
void CBullGameLogic::GroupCard(const _uint8 cbCardData[], _uint8 cbOutCardData[], _uint8 cbCardCount)
{
    _uint8 cbIndex_i = 0;
    _uint8 cbIndex_j = 0;
    _uint8 cbIndex_k = 0;
    _uint8 cbPostion = 3;
    bool bFind = false;
    _uint8 cbGroupCardData[MAX_COUNT] = {0};
    memcpy(cbGroupCardData, cbCardData, cbCardCount);
    //先对牌进行排序，这样可以使组成牛的数为最小组合
    SortCardList(cbGroupCardData, cbCardCount);
    for (_uint8 i = 0; i < cbCardCount - 2; i++)
    {
        for (_uint8 j = i + 1; j < cbCardCount - 1; j++)
        {
            for(_uint8 k = j + 1; k < cbCardCount; k++)
            {
                if ((GetCardLogicValue(cbGroupCardData[i]) +
                        GetCardLogicValue(cbGroupCardData[j]) +
                        GetCardLogicValue(cbGroupCardData[k])) % 10 == 0)
                {
                    //保存下标
                    cbIndex_i = i;
                    cbIndex_j = j;
                    cbIndex_k = k;
                    cbOutCardData[0] =  cbGroupCardData[i];
                    cbOutCardData[1] =  cbGroupCardData[j];
                    cbOutCardData[2] =  cbGroupCardData[k];
                    bFind = true;
                }
            }
        }
    }
    if (bFind)
    {
        for (_uint8 index = 0; index < cbCardCount; index++)
        {
            if (cbIndex_i != index &&
                    cbIndex_j != index &&
                    cbIndex_k != index)
            {
                cbOutCardData[cbPostion] =  cbGroupCardData[index];
                if(cbPostion < 4) cbPostion++;
            }
        }
    }
    else
    {
        memcpy(cbOutCardData, cbGroupCardData, cbCardCount);
    }
}

//对比扑克
bool CBullGameLogic::CompareCard(_uint8 cbFirstData[], 
								 _uint8 cbNextData[], 
								 _uint8 cbCardCount, 
								 _uint8 cbFiretIsGive, 
								 _uint8 cbNextIsGive)
{
    //获取类型
    _uint8 cbNextType = GetChoiceType(cbNextData, cbCardCount, cbNextIsGive);
    _uint8 cbFirstType = GetChoiceType(cbFirstData, cbCardCount, cbFiretIsGive);

    //点数类型
    if (cbFirstType != cbNextType)  ///先比较牌型
		return (cbFirstType > cbNextType);  

	///若牌型相同，比较牌大小
    //排序大小
    _uint8 bFirstTemp[MAX_COUNT], bNextTemp[MAX_COUNT];
    memcpy(bFirstTemp, cbFirstData, cbCardCount);
    memcpy(bNextTemp, cbNextData, cbCardCount);
    SortCardList(bFirstTemp, cbCardCount);
    SortCardList(bNextTemp, cbCardCount);

    //比较数值
    _uint8 cbNextMaxValue = GetCardValue(bNextTemp[0]);
	//if (cbNextMaxValue == 1)
	//	cbNextMaxValue = 14;
	
    _uint8 cbFirstMaxValue = GetCardValue(bFirstTemp[0]);
	if (cbFirstMaxValue == 1)
		cbFirstMaxValue = 14;
	
    if(cbNextMaxValue != cbFirstMaxValue)
		return cbFirstMaxValue > cbNextMaxValue;

    //比较颜色
    return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
}

//对比扑克
bool CBullGameLogic::CompateCard(_uint8 cbFirstData[], 
								 _uint8 cbNextData[], 
								 _uint8 cbCardCount)
{
	//获取类型
	_uint8 cbNextType = GetCardType(cbNextData, cbCardCount);
	_uint8 cbFirstType = GetCardType(cbFirstData, cbCardCount);

	//点数类型
	if (cbFirstType != cbNextType) 
		return (cbFirstType > cbNextType);

	//排序大小
	_uint8 bFirstTemp[MAX_COUNT], bNextTemp[MAX_COUNT];
	memcpy(bFirstTemp, cbFirstData, cbCardCount);
	memcpy(bNextTemp, cbNextData, cbCardCount);
	SortCardList(bFirstTemp, cbCardCount);
	SortCardList(bNextTemp, cbCardCount);

	//比较数值
	_uint8 cbNextMaxValue = GetCardValue(bNextTemp[0]);
	if (cbNextMaxValue == 1)
		cbNextMaxValue = 14;

	_uint8 cbFirstMaxValue = GetCardValue(bFirstTemp[0]);
	if (cbFirstMaxValue == 1)
		cbFirstMaxValue = 14;

	if(cbNextMaxValue != cbFirstMaxValue)
		return cbFirstMaxValue > cbNextMaxValue;

	//比较颜色
	return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
}


//逻辑扑克数值
_uint8 CBullGameLogic::GetCardLogicValue(_uint8 cbCardData)
{
    //扑克属性
    //_uint8 bCardColor = GetCardColor(cbCardData);
    _uint8 bCardValue = GetCardValue(cbCardData);

    //转换数值
    return (bCardValue > 10) ? (10) : bCardValue;
}


//抽取概率牌型
_uint8 CBullGameLogic::GetProbabilityCareType(ProbabilityConfig proConfig)
{

    int Rand_Type[4];
    Rand_Type[0] = (int)(proConfig.r_ProBullBull * 100);
    Rand_Type[1] = (int)(proConfig.r_ProBomp * 100 + Rand_Type[0]);
    Rand_Type[2] = (int)(proConfig.r_ProFiveHuaBull * 100 + Rand_Type[1]);
    Rand_Type[3] = (int)(proConfig.r_ProFiveSmallBull * 100 + Rand_Type[2]);

    //随机抽取类型
    int NumType = rand() % 10000;
    for(_uint8 num = 1; num < 5; num++)
    {
        if (NumType <= Rand_Type[num - 1])
        {
            return num;
        }
    }
    return 0;
}

//获取五小牛牌型
bool CBullGameLogic::GetFiveSmallBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{    
    _uint8 CardData[CARDCOUNT] = {0};
    _uint8 TempCard[MAX_COUNT] = {0};

    int CardType = CT_NOBULL;

	///10手五小牛牌
    _uint8 TempSmallCardType[10][5];
    memset(TempSmallCardType, 0, sizeof(TempSmallCardType));
    memcpy(TempSmallCardType, m_cbSmallType, sizeof(m_cbSmallType));

	///暂存到CardData
    memcpy(CardData, cbCardBuffer, cbBufferCount);

    //1—4小牌数量
    int nSmallCount = 0;
    _uint8 TempSmallCard[52] = {0};
    for(int i = 0; i < cbBufferCount; i++)
    {
        if (GetCardValue(CardData[i]) < 5 && GetCardValue(CardData[i]) > 0)
        {
            //保存小牌
            TempSmallCard[nSmallCount] = CardData[i];
            nSmallCount++;
        }
    }
    //当小牌数量不够时
    if (nSmallCount < 5)
    {
        return false;
    }
    //组牌次数
    //int CardIndex = 0;
    bool isHave = false;
    //混乱排序五小牛数组
    for (int i = 0; i < 10; i++)
    {
        int pos = rand() % 10;
        if (i != pos)
        {
            for (int j = 0; j < MAX_COUNT; j++)
            {
                TempSmallCardType[i][j] ^= TempSmallCardType[pos][j] ^= TempSmallCardType[i][j] ^= TempSmallCardType[pos][j];
            }
        }
    }

    for (int i = 0; i < 10; i++)
    {
        if(HaveSmallBullType(TempSmallCard, TempSmallCardType[i], TempCard, nSmallCount))
        {
            isHave = true;
            break;
        }
    }
    if (isHave)
    {
        CardType = GetCardType(TempCard, MAX_COUNT);
    }

    if (CardType != CT_FIVE_S_BULL)
    {
        return false;
    }

    memcpy(cbOutCard, TempCard, sizeof(TempCard));
    return true;
}

//获取五花牛牌型
bool CBullGameLogic::GetFiveHuaBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{

	///暂存到CardData
    _uint8 CardData[CARDCOUNT] = {0};
    memcpy(CardData, cbCardBuffer, cbBufferCount);

    //花牌数量
    int nHuaCount = 0;
    _uint8 TempHuaCard[52] = {0};

    for(int i = 0; i < cbBufferCount; i++)
    {
        if (GetCardValue(CardData[i]) < 14 && GetCardValue(CardData[i]) > 10)  ///J K Q
        {
            TempHuaCard[nHuaCount] = CardData[i];
            nHuaCount++;
        }
    }

    if (nHuaCount < 5)
    {
        //花牌数量不够，直接返回
        return false;
    }

	///---混乱花牌
    _uint8 TemcopyHuaCard[52] = {0};
    memcpy(TemcopyHuaCard, TempHuaCard, 52);
    RandCardList(TemcopyHuaCard, nHuaCount);

    //拷贝牌型数据
    memcpy(cbOutCard, TemcopyHuaCard, MAX_COUNT);
    return true;
}


//获取炸弹牌型
bool CBullGameLogic::GetBombCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{
    _uint8 nCount = 0;
    _uint8 CardData[CARDCOUNT] = {0};

    _uint8 TempCard[MAX_COUNT] = {0};
    int CardType = CT_NOBULL;

	///暂存传入的牌
    memcpy(CardData, cbCardBuffer, cbBufferCount);

    //int nTimes = 0;
    for (int i = 0; i < cbBufferCount; i++)
    {
        nCount = 0;
		///检查牌范围[1, k]
        if (GetCardValue(CardData[i]) > 0 && GetCardValue(CardData[i]) < 14)
        {
            int nValue = GetCardValue(CardData[i]);

            for (_uint8 j = 0; j < cbBufferCount; j++)  ///也从0开始
            {
                if (GetCardValue(CardData[j]) == nValue)
                {
                    TempCard[nCount] = CardData[j];
                    CardData[i] = INVALID_CARD;
                    nCount++;
                }
            }

            if (nCount == 4)
            {
                int bLastPosition = 0;   ///最后一张牌

                do
                {
                    bLastPosition = rand() % cbBufferCount;
                    //防止出现五花牛
                    if (GetCardValue(TempCard[0]) > 10 && GetCardValue(TempCard[0]) < 14)
                    {
                        if (GetCardValue(CardData[bLastPosition]) < 11 
							&& GetCardValue(CardData[bLastPosition]) > 0
                            && GetCardValue(CardData[bLastPosition]) != GetCardValue(TempCard[0]))
                        {
                            TempCard[nCount] = CardData[bLastPosition];
                            nCount++;
                        }
                    }
                    else
                    {
                        if (GetCardValue(CardData[bLastPosition]) != GetCardValue(TempCard[0])
							&& CardData[bLastPosition] != INVALID_CARD)
                        {
                            TempCard[nCount] = CardData[bLastPosition];
                            nCount++;
                        }
                    }


                }
                while (nCount != MAX_COUNT);
            }
            else
            {
                continue;
            }

            if (nCount == MAX_COUNT)
            {
                CardType = GetCardType(TempCard, MAX_COUNT);
                if (CardType == CT_BOMB)
                {
                    memcpy(cbOutCard, TempCard, sizeof(TempCard));
                    return true;
                }
            }
        }
    }
    return false;
}


//获取牛牛牌型
bool CBullGameLogic::GetBullBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{
    _uint8 CardData[CARDCOUNT] = {0};
    _uint8 TempCard[MAX_COUNT] = {0};
    int CardType = CT_NOBULL;

	///暂存起来传入的牌
    memcpy(CardData, cbCardBuffer, cbBufferCount);
    int index = cbBufferCount;   ///牌数量

    int cardIndex = 0;
    int nTimes = 0;
    do
    {
        if (cardIndex > (index - MAX_COUNT))
        {
            cardIndex = 0;
            RandCardList(CardData, index);
            nTimes++;
        }
        memcpy(TempCard, &CardData[index - cardIndex - MAX_COUNT], MAX_COUNT);
        cardIndex++;
        CardType = GetCardType(TempCard, MAX_COUNT);
    } while (CardType != CT_BULL_BULL && cardIndex < index - MAX_COUNT);

    memcpy(cbOutCard, TempCard, sizeof(TempCard));
    if (CardType == CT_BULL_BULL)
    {
        return true;
    }

    return false;
}


//获取普通牌型  ///--(牌型小于牛牛的牌)
bool CBullGameLogic::GetGeneralCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{
	///暂存传入参数
    _uint8 CardData[CARDCOUNT] = {0};
    memset(CardData, 0, sizeof(CardData));
    memcpy(CardData, cbCardBuffer, cbBufferCount);

    _uint8 TempCard[MAX_COUNT] = {0};
    int index = cbBufferCount;
    int cardIndex = 0;
    int nTimes = 0;
    int CardType = CT_NOBULL;
    //混乱排序
    //RandCardList(CardData,index);
    do
    {
        if (cardIndex > (index - MAX_COUNT))
        {
            cardIndex = 0;
            RandCardList(CardData, index);
            nTimes++;
        }
        memcpy(TempCard, &CardData[index - cardIndex - MAX_COUNT], MAX_COUNT);
        cardIndex++;
        //获取类型
        CardType = GetCardType(TempCard, MAX_COUNT);
    } while ((CardType >= CT_BULL_BULL 
		    && CardType <= CT_FIVE_S_BULL) 
			&& cardIndex > (index - MAX_COUNT));

    //取前五张牌型
    memcpy(cbOutCard, TempCard, MAX_COUNT);
    return true;
}

//是否存在五小牛牌型
bool CBullGameLogic::HaveSmallBullType(_uint8 cbCardBuffer[], _uint8 cbCard[], _uint8 cbOut[], _uint8 cbBufferCount)
{
    int count = 0;
    _uint8 TempCardBuf[CARDCOUNT] = {0};
    memset(TempCardBuf, 0, sizeof(TempCardBuf));
    memcpy(TempCardBuf, cbCardBuffer, cbBufferCount);

    for (int i = 0; i < MAX_COUNT; i++)
    {
        for (int j = 0; j < cbBufferCount; j++)
        {
            if (GetCardValue(TempCardBuf[j]) == cbCard[i])
            {
                cbOut[count] = TempCardBuf[j];
                TempCardBuf[j] = INVALID_CARD;
                count++;
                if (count == MAX_COUNT)
                {
                    return true;
                }
                break;
            }
        }
    }
    return false;
}

//是否手上有这些牌
bool CBullGameLogic::IsHaveCardList(_uint8 cbCardBuffer[], _uint8 cbBufferCount, _uint8 cbCardList[],_uint8 cbListCount)
{
	for (int i = 0; i < cbListCount; i++)
	{
		if (IsHaveCard(cbCardBuffer, cbCardList[i], cbBufferCount))
		{
			return true;
		}
	}
	return false;
}

//判断是否有该扑克
bool CBullGameLogic::IsHaveCard(_uint8 cbCardBuffer[], _uint8 cbCard, _uint8 cbBufferCount)
{
    for (int i = 0; i < cbBufferCount; i++)
    {
        if (cbCard == cbCardBuffer[i])
        {
            return true;
        }
    }
    return false;
}

//抽取牌型扑克（若没有，则返回牛牛以下的牌型）///---没用到
void CBullGameLogic::GetCardTypeData(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount, _uint8 type)
{

    switch(type)
    {
    case enFiveSmallBull:  //五小牛
    {
        if(!GetFiveSmallBullCard(cbCardBuffer, cbOutCard, cbBufferCount))
        {
            GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        }
        return;
    }
    case enFiveHuaBull:   //五花牛
    {
        if(!GetFiveHuaBullCard(cbCardBuffer, cbOutCard, cbBufferCount))
        {
            GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        }
        return;
    }
    case enBomb:         //炸弹
    {
        if(!GetBombCard(cbCardBuffer, cbOutCard, cbBufferCount))
        {
            GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        }
        return;
    }
    case enBullBull:    //牛牛
    {
        if(!GetBullBullCard(cbCardBuffer, cbOutCard, cbBufferCount))
        {
            GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        }
        return;
    }
    default:           //普通牌型
    {
        GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        return;
    }
    }
}

//删除扑克（从cbCardData[]中，移除存在cbRemoveCard[]的牌）
bool CBullGameLogic::RemoveCard(
		const _uint8 cbRemoveCard[],    ///---需要移除的牌
		_uint8 cbRemoveCount, 
		_uint8 cbCardData[],            
		_uint8 cbCardCount)
{
	//检验数据
	//ASSERT(cbRemoveCount<=cbCardCount);

	if(cbRemoveCount>cbCardCount)
		return false ;

	//定义变量
	_uint8 cbDeleteCount=0,cbTempCardData[54];
	memcpy(cbTempCardData,cbCardData,cbCardCount*sizeof(cbCardData[0]));

	//置零扑克
	for (_uint8 i=0;i<cbRemoveCount;i++)
	{
		for (_uint8 j=0;j<cbCardCount;j++)
		{
			if (cbRemoveCard[i]==cbTempCardData[j])
			{
				cbDeleteCount++;
				cbTempCardData[j]=0;
				break;
			}
		}
	}

	if (cbDeleteCount!=cbRemoveCount) 
		return false;

	//清理扑克
	memset(cbCardData, 0, sizeof(_uint8) * cbCardCount);

	_uint8 cbCardPos=0;
	for (_uint8 i=0;i<cbCardCount;i++)
	{
		if (cbTempCardData[i]!=0) 
			cbCardData[cbCardPos++]=cbTempCardData[i];
	}
	return true;
}