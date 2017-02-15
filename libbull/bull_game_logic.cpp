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

//�߼���  ///---�ҵ����
/*
My_zuhe(11, 5, CT_BULL_EIGHT);
My_zuhe(11, 5, CT_FIVE_H_BULL);
My_zuhe(11, 5, CT_FIVE_S_BULL);
My_zuhe(8,  5, CT_BOMB);
*/
/*
    �ҵ���ϣ���һЩ�ƣ�
	ȡ������Ϊ��big_type, ����r, ��Χ[1, n]
*/
void CBullGameLogic::My_zuhe(int n,int r, int big_type) 
{  
	///map (int->vector)
	multimap<int,vector<int> > mm,mm2;   
	multimap<int,vector<int> >::iterator pos;   ///map������

	vector<int> nn,temp,bb;  

	///��ά����
	vector<vector<int> > target;         
	vector<int>::iterator ita,ita_bb;  
	typedef pair<int,vector<int> > pr;      ///pair����
	for(int i=1;i<=n;i++)   
		bb.push_back(i);     ///bb����[1, n]
	if(r>n)   
		return;

	int k;      
	ita_bb=bb.begin();  
	temp.push_back(*ita_bb);  ///�����׸��ַ�
	mm.insert(pr(temp.size(),temp));  ///���루1,1��
	temp.clear();             ///---���

	++ita_bb;  
	while(ita_bb != bb.end())   ///---[2, n]
	{   
		k=*ita_bb;   
		for(pos=mm.begin();pos!=mm.end();++pos)   
		{    
			temp=pos->second;    ///---ȡ��map�е�vector
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
			target.push_back(pos->second);     ///---�����ϵ���
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

	//����׼��
	int count = 0;
	_uint8 cbCardData[CARDCOUNT] = {0};   ///---�����ͼ��ض�Ӧ����
	switch(big_type)
	{
	case CT_BULL_EIGHT:   ///---ţ8
		memcpy(cbCardData, m_cbCardData, CARDCOUNT);   ///��ʼΪ����52����
		RandCardList(cbCardData, CARDCOUNT);
		count = 150;
		break;
	case CT_BOMB:         ///---ը��
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

	_uint8 card_data[MAX_COUNT] = { 0 };   ///---����
	int card_count = 0;
	for(tt_pos=target.begin(), k=0; tt_pos != target.end(); ++tt_pos, k++)  ///---������ά����
	{     
		index = 0;
		for(ita=tt_pos->begin();ita != tt_pos->end();++ita)    ///---ȡ�����һά����
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

//----��������-----
void CBullGameLogic::regis_goods_card_type()
{
	//��ͨ����
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

//��ȡ���ͣ������Ƶ����ͣ�
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
		return CT_FIVE_S_BULL; //��Сţ

    if(bKingCount == MAX_COUNT) 
		return CT_FIVE_H_BULL; //�廨ţ

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
            return CT_BOMB;//ը��
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

//��ȡ���ͱ���
int CBullGameLogic::GetTimes(_uint8 cbCardData[], _uint8 cbCardCount)
{
    if(cbCardCount != MAX_COUNT)   ///����5����
		return 0;

	///---����
    _uint8 bTimes = GetCardType(cbCardData, MAX_COUNT);

	if(bTimes <= CT_BULL_SIX)        ///ţ����ţһ,��ţ
		return 1;
	else if(bTimes <= CT_BULL_NINE)  ///ţ�ŵ�ţ��
		return 2;	
	else if(bTimes == CT_BULL_BULL)  ///ţţ
		return  3;
	else if(bTimes == CT_BOMB)       ///��ը
		return 4;
	else if(bTimes == CT_FIVE_H_BULL) ///�廨ţ
		return  5;
	else if(bTimes == CT_FIVE_S_BULL) ///��Сţ
		return  8;

    return 0;
}

//��ȡ�����������(ϵͳ����)
/*
  param: cbOutCardData
  return: ����ֵ
*/
_uint8 CBullGameLogic::GetCardTypeAndData(_uint8 cbCardData[], _uint8 cbCardCount, _uint8 cbOutCardData[])
{
	_uint8 CardData[MAX_COUNT];
	memset(CardData, 0, sizeof(CardData));
	memcpy(CardData, cbCardData, sizeof(CardData));

	///����
	_uint8 UserOxType = GetCardType(CardData, MAX_COUNT);

	///����
	_uint8 GropCard[MAX_COUNT];
	memset(GropCard, 0, sizeof(GropCard));

	if (UserOxType < CT_BOMB && UserOxType > CT_NOBULL)
	{
		//���ţ����
		GroupCard(CardData, GropCard, MAX_COUNT);
	}
	else
	{
		memcpy(GropCard, CardData, sizeof(CardData));
	}
	
	//�������Ϊը��,����
	if(UserOxType == CT_BOMB)
		SortBom(CardData, GropCard, MAX_COUNT);
	
	memcpy(cbOutCardData, GropCard, sizeof(GropCard));
	return UserOxType;
}

//���ѡ�����ͣ����ش������ͣ�
_uint8 CBullGameLogic::GetChoiceType(_uint8 cbCardData[], _uint8 cbCardCount, _uint8 IsGive)
{
	if (IsGive)   ///����
	{
		return CT_NOBULL;
	}
	
	///�������ݴ�����
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

	///����
	_uint8 UserOxType = GetCardType(CardData, MAX_COUNT);
	if (UserOxType >= CT_BOMB)
	{
		return UserOxType;
	}

	///ǰ��������ֵ֮��
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

	///��ţ
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

//��ȡţţ///---û�õ�
bool CBullGameLogic::GetOxCard(_uint8 cbCardData[], _uint8 cbCardCount)
{
  //  ASSERT(cbCardCount == MAX_COUNT);

    //���ñ���
    _uint8 bTemp[MAX_COUNT], bTempData[MAX_COUNT];
    memcpy(bTempData, cbCardData, sizeof(bTempData));

    _uint8 bSum = 0;
    for (_uint8 i = 0; i < cbCardCount; i++)
    {
        bTemp[i] = GetCardLogicValue(cbCardData[i]);   ///��ȡ�Ƶ��߼�ֵ������10�Ķ�Ϊ10
        bSum += bTemp[i];
    }

    //����ţţ
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

//��ȡ����(�����Ƿ�Ϊ10�ı���)
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

//�����˿�
void CBullGameLogic::SortCardList(_uint8 cbCardData[], _uint8 cbCardCount)
{
	//ת����ֵ
	_uint8 cbLogicValue[MAX_COUNT];
	for (_uint8 i=0;i<cbCardCount;i++) 
	{
		cbLogicValue[i]=GetCardValue(cbCardData[i]);
		//if(cbLogicValue[i] == 1)
		//	cbLogicValue[i] = 14;    ///Ϊʲô����1��ת����14
	}

	//�������  ///ð������
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
				//����λ��
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


///---û�õ�
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

//����ը��
void CBullGameLogic::SortBom(_uint8 cbCardBuffer[], _uint8 cbCardOut[], _uint8 cbBufferCount)
{
    _uint8 card[MAX_COUNT];
    memset(card, 0, sizeof(card));
    memcpy(card, cbCardBuffer, MAX_COUNT);

    _uint8 CardNalve = 0;  ///ը��ֵ
    _uint8 bBombCount;
    for (_uint8 i = 0; i < cbBufferCount - 1; i++)
    {
        bBombCount = 1;
        for (_uint8 j = i + 1; j < cbBufferCount; j++)
        {
            if (GetCardValue(card[i]) == GetCardValue(card[j]))
            {
                //��ȡը��ֵ
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
            cbCardOut[index] = card[n];  ///��ը�����ƣ�����ǰ4��λ��
            index++;
        }
        else
        {
            LastCard = card[n];
        }
    }
    cbCardOut[MAX_COUNT - 1] = LastCard;  ///����һ������� 

}

//�����˿�  ///---��cbCardBuffer[]����
void CBullGameLogic::RandCardList(_uint8 cbCardBuffer[], _uint8 cbBufferCount)
{
    //����׼��
    _uint8 cbCardData[CARDCOUNT] = {0};
    memcpy(cbCardData, cbCardBuffer, cbBufferCount);

    //�����˿�
	srand(time(0));
    _uint8 bRandCount = 0, bPosition = 0;
    do
    {
        bPosition = rand() % (cbBufferCount - bRandCount);
        cbCardBuffer[bRandCount++] = cbCardData[bPosition];
        cbCardData[bPosition] = cbCardData[cbBufferCount - bRandCount];  ///�������
    }
    while (bRandCount < cbBufferCount);
    return;
}

//�����㷨
int CBullGameLogic::GoodRandCardList(
		_tint32 count[],        ///---��ҵľ���
		bool is_robot[],        ///---�ǲ��ǻ�����
		_uint8 cbCardBuffer[],  ///---�գ�Ӧ����ʣ����ƣ�
		_uint8 cbBufferCount,   ///---�Ƶ�����
		_uint8 cbPlayerCount,   ///---�����
		_uint8 GoodCardBuff[GAME_PLAYER_COUNT][MAX_COUNT], ///����buffer
		_uint8 GoodChairId[]                               ///��������id
  )
{
	//todo no use good card
/*	tagGoodCardCfg GoodGfg = conf_mgr::GetInstance()->GetGoodCardCfg();

	//����׼��
	_uint8 cbCardData[CARDCOUNT] = {0};
	memcpy(cbCardData, m_cbCardData, CARDCOUNT);   ///---��ʼ����

	_uint8  TYPE_DATA[MAX_GOOD_COUNT][5];          ///---ȫ��������Ŀ
	memset(TYPE_DATA, 0, sizeof(TYPE_DATA));
	memcpy(TYPE_DATA, CARD_TYPE_DATA, MAX_GOOD_COUNT * MAX_COUNT);    ///CARD_TYPE_DATA �ҵ���ϸ�ֵ
	int Good_Type_Count = CARD_TYPE_COUNT;   ///������

	//�������������
	int chair_count = 0;
	int left_count = CARDCOUNT;   ///---ʣ�µ�����
	srand(time(0));

	for(int i = 0; i < cbPlayerCount; i++)
	{
		if (count[i] == -1)   ///---��ҵ��ܾ���Ϊ-1��˵�����Ϊ��,����
			continue;

		int is_good = rand() % 100;   ///�������ֵ
		int good_value = 0;
		
		if (is_robot[i])   ///---Ϊ������
		{
			good_value = GoodGfg.wRobotRate;
		}
		else
		{
			if (count[i] == 0)  ///---һ�ֶ�û��
				good_value = GoodGfg.wNewUserFirst;		//��ע���û�
			else
				if (count[i] >= 1 && count[i] <= GoodGfg.wNewUserDefine)
					good_value = GoodGfg.wNewUserNext;	//���һ��ʱ������û�
				else 
					good_value = GoodGfg.wOldUser;		//���û�
		}
			
		if (is_good <= good_value)   ///---С�����õĺ���ֵ
		{
			int id = rand() % Good_Type_Count;   ///---���ѡ��һ����

			//�Ѻ��ƴ�����
			memcpy(GoodCardBuff[i], TYPE_DATA[id], MAX_COUNT);  ///--��λ�õĺ���
			GoodChairId[i] = i;   ///---λ��ID
			chair_count++;        ///---λ�ø����ۼ�

			RemoveCard(
				TYPE_DATA[id], 
				MAX_COUNT, 
				cbCardData, 
				left_count);

			left_count -= MAX_COUNT;

			//����ѡ���ƴ��ƶ��Ƴ� + �Ѻ����Ƴ�
			//���õ���5���Ƶ������Ƴ�����ֹ���ظ���
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

	//ϴ�ƺ������ҷ���
	RandCardList(cbCardData, left_count);
	memcpy(cbCardBuffer, cbCardData, left_count);
	return chair_count;*/
	return -1;
}

//����˿�
void CBullGameLogic::GroupCard(const _uint8 cbCardData[], _uint8 cbOutCardData[], _uint8 cbCardCount)
{
    _uint8 cbIndex_i = 0;
    _uint8 cbIndex_j = 0;
    _uint8 cbIndex_k = 0;
    _uint8 cbPostion = 3;
    bool bFind = false;
    _uint8 cbGroupCardData[MAX_COUNT] = {0};
    memcpy(cbGroupCardData, cbCardData, cbCardCount);
    //�ȶ��ƽ���������������ʹ���ţ����Ϊ��С���
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
                    //�����±�
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

//�Ա��˿�
bool CBullGameLogic::CompareCard(_uint8 cbFirstData[], 
								 _uint8 cbNextData[], 
								 _uint8 cbCardCount, 
								 _uint8 cbFiretIsGive, 
								 _uint8 cbNextIsGive)
{
    //��ȡ����
    _uint8 cbNextType = GetChoiceType(cbNextData, cbCardCount, cbNextIsGive);
    _uint8 cbFirstType = GetChoiceType(cbFirstData, cbCardCount, cbFiretIsGive);

    //��������
    if (cbFirstType != cbNextType)  ///�ȱȽ�����
		return (cbFirstType > cbNextType);  

	///��������ͬ���Ƚ��ƴ�С
    //�����С
    _uint8 bFirstTemp[MAX_COUNT], bNextTemp[MAX_COUNT];
    memcpy(bFirstTemp, cbFirstData, cbCardCount);
    memcpy(bNextTemp, cbNextData, cbCardCount);
    SortCardList(bFirstTemp, cbCardCount);
    SortCardList(bNextTemp, cbCardCount);

    //�Ƚ���ֵ
    _uint8 cbNextMaxValue = GetCardValue(bNextTemp[0]);
	//if (cbNextMaxValue == 1)
	//	cbNextMaxValue = 14;
	
    _uint8 cbFirstMaxValue = GetCardValue(bFirstTemp[0]);
	if (cbFirstMaxValue == 1)
		cbFirstMaxValue = 14;
	
    if(cbNextMaxValue != cbFirstMaxValue)
		return cbFirstMaxValue > cbNextMaxValue;

    //�Ƚ���ɫ
    return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
}

//�Ա��˿�
bool CBullGameLogic::CompateCard(_uint8 cbFirstData[], 
								 _uint8 cbNextData[], 
								 _uint8 cbCardCount)
{
	//��ȡ����
	_uint8 cbNextType = GetCardType(cbNextData, cbCardCount);
	_uint8 cbFirstType = GetCardType(cbFirstData, cbCardCount);

	//��������
	if (cbFirstType != cbNextType) 
		return (cbFirstType > cbNextType);

	//�����С
	_uint8 bFirstTemp[MAX_COUNT], bNextTemp[MAX_COUNT];
	memcpy(bFirstTemp, cbFirstData, cbCardCount);
	memcpy(bNextTemp, cbNextData, cbCardCount);
	SortCardList(bFirstTemp, cbCardCount);
	SortCardList(bNextTemp, cbCardCount);

	//�Ƚ���ֵ
	_uint8 cbNextMaxValue = GetCardValue(bNextTemp[0]);
	if (cbNextMaxValue == 1)
		cbNextMaxValue = 14;

	_uint8 cbFirstMaxValue = GetCardValue(bFirstTemp[0]);
	if (cbFirstMaxValue == 1)
		cbFirstMaxValue = 14;

	if(cbNextMaxValue != cbFirstMaxValue)
		return cbFirstMaxValue > cbNextMaxValue;

	//�Ƚ���ɫ
	return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
}


//�߼��˿���ֵ
_uint8 CBullGameLogic::GetCardLogicValue(_uint8 cbCardData)
{
    //�˿�����
    //_uint8 bCardColor = GetCardColor(cbCardData);
    _uint8 bCardValue = GetCardValue(cbCardData);

    //ת����ֵ
    return (bCardValue > 10) ? (10) : bCardValue;
}


//��ȡ��������
_uint8 CBullGameLogic::GetProbabilityCareType(ProbabilityConfig proConfig)
{

    int Rand_Type[4];
    Rand_Type[0] = (int)(proConfig.r_ProBullBull * 100);
    Rand_Type[1] = (int)(proConfig.r_ProBomp * 100 + Rand_Type[0]);
    Rand_Type[2] = (int)(proConfig.r_ProFiveHuaBull * 100 + Rand_Type[1]);
    Rand_Type[3] = (int)(proConfig.r_ProFiveSmallBull * 100 + Rand_Type[2]);

    //�����ȡ����
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

//��ȡ��Сţ����
bool CBullGameLogic::GetFiveSmallBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{    
    _uint8 CardData[CARDCOUNT] = {0};
    _uint8 TempCard[MAX_COUNT] = {0};

    int CardType = CT_NOBULL;

	///10����Сţ��
    _uint8 TempSmallCardType[10][5];
    memset(TempSmallCardType, 0, sizeof(TempSmallCardType));
    memcpy(TempSmallCardType, m_cbSmallType, sizeof(m_cbSmallType));

	///�ݴ浽CardData
    memcpy(CardData, cbCardBuffer, cbBufferCount);

    //1��4С������
    int nSmallCount = 0;
    _uint8 TempSmallCard[52] = {0};
    for(int i = 0; i < cbBufferCount; i++)
    {
        if (GetCardValue(CardData[i]) < 5 && GetCardValue(CardData[i]) > 0)
        {
            //����С��
            TempSmallCard[nSmallCount] = CardData[i];
            nSmallCount++;
        }
    }
    //��С����������ʱ
    if (nSmallCount < 5)
    {
        return false;
    }
    //���ƴ���
    //int CardIndex = 0;
    bool isHave = false;
    //����������Сţ����
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

//��ȡ�廨ţ����
bool CBullGameLogic::GetFiveHuaBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{

	///�ݴ浽CardData
    _uint8 CardData[CARDCOUNT] = {0};
    memcpy(CardData, cbCardBuffer, cbBufferCount);

    //��������
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
        //��������������ֱ�ӷ���
        return false;
    }

	///---���һ���
    _uint8 TemcopyHuaCard[52] = {0};
    memcpy(TemcopyHuaCard, TempHuaCard, 52);
    RandCardList(TemcopyHuaCard, nHuaCount);

    //������������
    memcpy(cbOutCard, TemcopyHuaCard, MAX_COUNT);
    return true;
}


//��ȡը������
bool CBullGameLogic::GetBombCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{
    _uint8 nCount = 0;
    _uint8 CardData[CARDCOUNT] = {0};

    _uint8 TempCard[MAX_COUNT] = {0};
    int CardType = CT_NOBULL;

	///�ݴ洫�����
    memcpy(CardData, cbCardBuffer, cbBufferCount);

    //int nTimes = 0;
    for (int i = 0; i < cbBufferCount; i++)
    {
        nCount = 0;
		///����Ʒ�Χ[1, k]
        if (GetCardValue(CardData[i]) > 0 && GetCardValue(CardData[i]) < 14)
        {
            int nValue = GetCardValue(CardData[i]);

            for (_uint8 j = 0; j < cbBufferCount; j++)  ///Ҳ��0��ʼ
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
                int bLastPosition = 0;   ///���һ����

                do
                {
                    bLastPosition = rand() % cbBufferCount;
                    //��ֹ�����廨ţ
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


//��ȡţţ����
bool CBullGameLogic::GetBullBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{
    _uint8 CardData[CARDCOUNT] = {0};
    _uint8 TempCard[MAX_COUNT] = {0};
    int CardType = CT_NOBULL;

	///�ݴ������������
    memcpy(CardData, cbCardBuffer, cbBufferCount);
    int index = cbBufferCount;   ///������

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


//��ȡ��ͨ����  ///--(����С��ţţ����)
bool CBullGameLogic::GetGeneralCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount)
{
	///�ݴ洫�����
    _uint8 CardData[CARDCOUNT] = {0};
    memset(CardData, 0, sizeof(CardData));
    memcpy(CardData, cbCardBuffer, cbBufferCount);

    _uint8 TempCard[MAX_COUNT] = {0};
    int index = cbBufferCount;
    int cardIndex = 0;
    int nTimes = 0;
    int CardType = CT_NOBULL;
    //��������
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
        //��ȡ����
        CardType = GetCardType(TempCard, MAX_COUNT);
    } while ((CardType >= CT_BULL_BULL 
		    && CardType <= CT_FIVE_S_BULL) 
			&& cardIndex > (index - MAX_COUNT));

    //ȡǰ��������
    memcpy(cbOutCard, TempCard, MAX_COUNT);
    return true;
}

//�Ƿ������Сţ����
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

//�Ƿ���������Щ��
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

//�ж��Ƿ��и��˿�
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

//��ȡ�����˿ˣ���û�У��򷵻�ţţ���µ����ͣ�///---û�õ�
void CBullGameLogic::GetCardTypeData(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount, _uint8 type)
{

    switch(type)
    {
    case enFiveSmallBull:  //��Сţ
    {
        if(!GetFiveSmallBullCard(cbCardBuffer, cbOutCard, cbBufferCount))
        {
            GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        }
        return;
    }
    case enFiveHuaBull:   //�廨ţ
    {
        if(!GetFiveHuaBullCard(cbCardBuffer, cbOutCard, cbBufferCount))
        {
            GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        }
        return;
    }
    case enBomb:         //ը��
    {
        if(!GetBombCard(cbCardBuffer, cbOutCard, cbBufferCount))
        {
            GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        }
        return;
    }
    case enBullBull:    //ţţ
    {
        if(!GetBullBullCard(cbCardBuffer, cbOutCard, cbBufferCount))
        {
            GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        }
        return;
    }
    default:           //��ͨ����
    {
        GetGeneralCard(cbCardBuffer, cbOutCard, cbBufferCount);
        return;
    }
    }
}

//ɾ���˿ˣ���cbCardData[]�У��Ƴ�����cbRemoveCard[]���ƣ�
bool CBullGameLogic::RemoveCard(
		const _uint8 cbRemoveCard[],    ///---��Ҫ�Ƴ�����
		_uint8 cbRemoveCount, 
		_uint8 cbCardData[],            
		_uint8 cbCardCount)
{
	//��������
	//ASSERT(cbRemoveCount<=cbCardCount);

	if(cbRemoveCount>cbCardCount)
		return false ;

	//�������
	_uint8 cbDeleteCount=0,cbTempCardData[54];
	memcpy(cbTempCardData,cbCardData,cbCardCount*sizeof(cbCardData[0]));

	//�����˿�
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

	//�����˿�
	memset(cbCardData, 0, sizeof(_uint8) * cbCardCount);

	_uint8 cbCardPos=0;
	for (_uint8 i=0;i<cbCardCount;i++)
	{
		if (cbTempCardData[i]!=0) 
			cbCardData[cbCardPos++]=cbTempCardData[i];
	}
	return true;
}