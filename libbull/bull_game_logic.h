#ifndef BullLogic_H
#define BullLogic_H

#include "global_define.h"
#include <algorithm>
using namespace std;

#define MAX_COUNT         5
#define CARDCOUNT         52

//��ֵ����
#define	LOGIC_MASK_COLOR	0xF0	  //��ɫ����
#define	LOGIC_MASK_VALUE	0x0F	  //��ֵ����
#define INVALID_CARD        0x0       //��Ч�˿�
#define MAX_GOOD_COUNT		200		  //��������Ŀ
#define MAX_GOOD_ROUND		5		  //���ƾ���

///52����(ǰ4�ֽ�Ϊ��ɫ����4�ֽ�Ϊ��ֵ)
const _uint8  m_cbCardData[52] =
{
	//���� A - K                                                 J      Q     K
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
	//÷�� A - K
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,	
	//���� A - K
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
	//���� A - K
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D	
};

//�廨ţ   ///---û0x3D ��5����ȫΪJQK������ţţ���ͣ�
const _uint8  m_cbHCardData[11] =
{
	0x0B, 0x0C, 0x0D, 
	0x1B, 0x1C, 0x1D,
	0x2B, 0x2C, 0x2D,
	0x3B, 0x3C,
};

//��Сţ   ///û0x33�������ƶ�С��5�����Ƶ�������10��
const _uint8  m_cbSCardData[11] =
{
	0x01, 0x02, 0x03, 
	0x11, 0x12, 0x13,
	0x21, 0x22, 0x23, 
	0x31, 0x32,		
};

//ը��
const _uint8  m_cbZCardData[11] =
{
	0x05, 0x06,
	0x15, 0x16,
	0x25, 0x26,
	0x35, 0x36,		
};

///{1, 1, 1, 2, 3}?,   ///---Ҳ����Сţ��
const _uint8 m_cbSmallType[10][5] =
{
    {1, 1, 1, 2, 2},
    {1, 1, 1, 2, 3},
    {1, 1, 1, 2, 3},
    {1, 1, 1, 3, 3},
    {1, 1, 1, 3, 4},
    {1, 1, 2, 2, 2},
    {1, 1, 2, 2, 3},
    {1, 1, 2, 2, 4},
    {1, 1, 2, 3, 3},
    {1, 2, 2, 2, 3}
};

//�˿����ͣ��������ͣ�
enum CT_BULL_TYPE
{
    CT_NOBULL = 0,      // ûţ
    CT_BULL_ONE,        // ţһ
    CT_BULL_TWO,        // ţ��
    CT_BULL_THREE,      // ţ��
    CT_BULL_FOUR,       // ţ��
    CT_BULL_FIVE,       // ţ��
    CT_BULL_SIX,        // ţ��
    CT_BULL_SEVEN,      // ţ��
    CT_BULL_EIGHT,      // ţ��
    CT_BULL_NINE,       // ţ��
    CT_BULL_BULL,       // ţţ
    CT_BOMB,            // ը��
    CT_FIVE_H_BULL,     // �廨ţ
    CT_FIVE_S_BULL,     // ��Сţ
};

///---���͸���
enum BullProbabilityType
{
    enBullBull = 1,       //ţţ��������
    enBomb,               //ը����������
    enFiveHuaBull,        //�廨ţ��������
    enFiveSmallBull       //��Сţ��������
};

///---��������
struct ProbabilityConfig
{
    float r_ProFiveSmallBull; //��Сţ����
    float r_ProFiveHuaBull;   //�廨ţ����
    float r_ProBomp;          //ը������
    float r_ProBullBull;      //ţţ����
};
///---Ӯ�����ͱ���
struct CardTypeTimes
{
	int nBull_Seven;
	int nBull_Eight;
	int nBull_Nine;
	int nBull_Bull;
	int nBomb;
	int nFive_H_Bull;
	int nFive_S_Bull;
};

class CBullGameLogic    ///---��ţ�߼�
{
public:
    CBullGameLogic(void);
    ~CBullGameLogic(void);

	//��ö���ʵ��
	static CBullGameLogic * GetInstance();

private:
	_uint8   CARD_TYPE_COUNT;                       ///��������
	_uint8   CARD_TYPE_DATA[MAX_GOOD_COUNT][5];     ///�������ݣ�200����ϵ��ƣ�
	
public:
	///�ҵ���ϣ����ݴ�С�����������������)
	 void My_zuhe(int n,int r, int big_type);    
	//ע�����
	void regis_goods_card_type();
    //��ȡ����
    _uint8 GetCardType(_uint8 cbCardData[], _uint8 cbCardCount);
	//��ȡ�����������
	_uint8 GetCardTypeAndData(_uint8 cbCardData[], _uint8 cbCardCount, _uint8 cbOutCardData[]);	
	//���ѡ������
	_uint8 GetChoiceType(_uint8 cbCardData[], _uint8 cbCardCount, _uint8 IsGive);
    //������������
    _uint8 GetProbabilityCareType(ProbabilityConfig proConfig);
    //��ȡ��ֵ
    _uint8 GetCardValue(_uint8 cbCardData)
    {
        return (cbCardData & LOGIC_MASK_VALUE);
    }
    //��ȡ��ɫ
    _uint8 GetCardColor(_uint8 cbCardData)
    {
        return (cbCardData & LOGIC_MASK_COLOR);
    }
    //��ȡ���ͱ���
    int GetTimes(_uint8 cbCardData[], _uint8 cbCardCount);
    //��ȡţţ  ///XXX
    bool GetOxCard(_uint8 cbCardData[], _uint8 cbCardCount);
    //��ȡ����
    bool IsIntValue(_uint8 cbCardData[], _uint8 cbCardCount);
    //��ȡ��Сţ����
    bool GetFiveSmallBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //��ȡ�廨ţ����
    bool GetFiveHuaBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //��ȡը������
    bool GetBombCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //��ȡţţ����
    bool GetBullBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //��ȡ��ͨ����
    bool GetGeneralCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //�������
    void GroupCard(const _uint8 cbCardData[], _uint8 cbOutCardData[], _uint8 cbCardCount);
    //��ȡ����///XX
    void GetCardTypeData(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount, _uint8 type);
    //�Ƿ��д���ֵ
    bool IsHaveCard(_uint8 cbCardBuffer[], _uint8 cbCard, _uint8 cbBufferCount);
	//�Ƿ���һ����������
	bool IsHaveCardList(_uint8 cbCardBuffer[], _uint8 cbBufferCount, _uint8 cbCardList[],_uint8 cbListCount);

    bool HaveSmallBullType(_uint8 cbCardBuffer[], _uint8 cbCard[], _uint8 cbOut[], _uint8 cbBufferCount);

public:
    //�����˿�(����ֵ��С)
    void SortCardList(_uint8 cbCardData[], _uint8 cbCardCount);
    //�����˿�
    void RandCardList(_uint8 cbCardBuffer[], _uint8 cbBufferCount);
	//�����㷨
	int GoodRandCardList(_tint32 count[], bool is_robot[], _uint8 cbCardBuffer[], _uint8 cbBufferCount, 
		_uint8 cbPlayerCount, _uint8 GoodCardBuff[GAME_PLAYER_COUNT][MAX_COUNT], _uint8 GoodChairId[]);

    //�����˿�(�����ݴ�С)///XXX
    void SortCardDataList(_uint8 cbCardData[], _uint8 cbCardCount);
    //����ը��˳��
    void SortBom(_uint8 cbCardBuffer[], _uint8 cbCardOut[], _uint8 cbBufferCount);
	//ɾ���˿�
	bool RemoveCard(const _uint8 cbRemoveCard[], _uint8 cbRemoveCount, _uint8 cbCardData[], _uint8 cbCardCount);
    //���ܺ���
public:
    //�߼���ֵ
    _uint8 GetCardLogicValue(_uint8 cbCardData);
    //�Ա��˿�
    bool CompareCard(_uint8 cbFirstData[], _uint8 cbNextData[], _uint8 cbCardCount, _uint8 cbFiretIsGive, _uint8 cbNextIsGive);
	//�Ա��˿�
	bool CompateCard(_uint8 cbFirstData[], _uint8 cbNextData[], _uint8 cbCardCount);
};
#endif