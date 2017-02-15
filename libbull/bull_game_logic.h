#ifndef BullLogic_H
#define BullLogic_H

#include "global_define.h"
#include <algorithm>
using namespace std;

#define MAX_COUNT         5
#define CARDCOUNT         52

//数值掩码
#define	LOGIC_MASK_COLOR	0xF0	  //花色掩码
#define	LOGIC_MASK_VALUE	0x0F	  //数值掩码
#define INVALID_CARD        0x0       //无效扑克
#define MAX_GOOD_COUNT		200		  //最大好牌数目
#define MAX_GOOD_ROUND		5		  //好牌局数

///52张牌(前4字节为花色，后4字节为数值)
const _uint8  m_cbCardData[52] =
{
	//方块 A - K                                                 J      Q     K
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
	//梅花 A - K
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,	
	//红桃 A - K
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
	//黑桃 A - K
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D	
};

//五花牛   ///---没0x3D （5张牌全为JQK的特殊牛牛牌型）
const _uint8  m_cbHCardData[11] =
{
	0x0B, 0x0C, 0x0D, 
	0x1B, 0x1C, 0x1D,
	0x2B, 0x2C, 0x2D,
	0x3B, 0x3C,
};

//五小牛   ///没0x33（五张牌都小于5，且牌点总数≤10）
const _uint8  m_cbSCardData[11] =
{
	0x01, 0x02, 0x03, 
	0x11, 0x12, 0x13,
	0x21, 0x22, 0x23, 
	0x31, 0x32,		
};

//炸弹
const _uint8  m_cbZCardData[11] =
{
	0x05, 0x06,
	0x15, 0x16,
	0x25, 0x26,
	0x35, 0x36,		
};

///{1, 1, 1, 2, 3}?,   ///---也是五小牛吧
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

//扑克类型（基本牌型）
enum CT_BULL_TYPE
{
    CT_NOBULL = 0,      // 没牛
    CT_BULL_ONE,        // 牛一
    CT_BULL_TWO,        // 牛二
    CT_BULL_THREE,      // 牛三
    CT_BULL_FOUR,       // 牛四
    CT_BULL_FIVE,       // 牛五
    CT_BULL_SIX,        // 牛六
    CT_BULL_SEVEN,      // 牛七
    CT_BULL_EIGHT,      // 牛八
    CT_BULL_NINE,       // 牛九
    CT_BULL_BULL,       // 牛牛
    CT_BOMB,            // 炸弹
    CT_FIVE_H_BULL,     // 五花牛
    CT_FIVE_S_BULL,     // 五小牛
};

///---牌型概率
enum BullProbabilityType
{
    enBullBull = 1,       //牛牛概率类型
    enBomb,               //炸弹概率类型
    enFiveHuaBull,        //五花牛概率类型
    enFiveSmallBull       //五小牛概率类型
};

///---概率配置
struct ProbabilityConfig
{
    float r_ProFiveSmallBull; //五小牛概率
    float r_ProFiveHuaBull;   //五花牛概率
    float r_ProBomp;          //炸弹概率
    float r_ProBullBull;      //牛牛概率
};
///---赢方牌型倍数
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

class CBullGameLogic    ///---斗牛逻辑
{
public:
    CBullGameLogic(void);
    ~CBullGameLogic(void);

	//获得对象实例
	static CBullGameLogic * GetInstance();

private:
	_uint8   CARD_TYPE_COUNT;                       ///牌型总数
	_uint8   CARD_TYPE_DATA[MAX_GOOD_COUNT][5];     ///牌型数据（200手组合的牌）
	
public:
	///我的组合（根据大小，牌数，牌型来组合)
	 void My_zuhe(int n,int r, int big_type);    
	//注册好牌
	void regis_goods_card_type();
    //获取类型
    _uint8 GetCardType(_uint8 cbCardData[], _uint8 cbCardCount);
	//获取最大牌型数据
	_uint8 GetCardTypeAndData(_uint8 cbCardData[], _uint8 cbCardCount, _uint8 cbOutCardData[]);	
	//获得选牌牌型
	_uint8 GetChoiceType(_uint8 cbCardData[], _uint8 cbCardCount, _uint8 IsGive);
    //概率牌型类型
    _uint8 GetProbabilityCareType(ProbabilityConfig proConfig);
    //获取数值
    _uint8 GetCardValue(_uint8 cbCardData)
    {
        return (cbCardData & LOGIC_MASK_VALUE);
    }
    //获取花色
    _uint8 GetCardColor(_uint8 cbCardData)
    {
        return (cbCardData & LOGIC_MASK_COLOR);
    }
    //获取牌型倍数
    int GetTimes(_uint8 cbCardData[], _uint8 cbCardCount);
    //获取牛牛  ///XXX
    bool GetOxCard(_uint8 cbCardData[], _uint8 cbCardCount);
    //获取整数
    bool IsIntValue(_uint8 cbCardData[], _uint8 cbCardCount);
    //获取五小牛牌型
    bool GetFiveSmallBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //获取五花牛牌型
    bool GetFiveHuaBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //获取炸弹牌型
    bool GetBombCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //获取牛牛牌型
    bool GetBullBullCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //获取普通牌型
    bool GetGeneralCard(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount);
    //组合数据
    void GroupCard(const _uint8 cbCardData[], _uint8 cbOutCardData[], _uint8 cbCardCount);
    //获取牌型///XX
    void GetCardTypeData(_uint8 cbCardBuffer[], _uint8 cbOutCard[], _uint8 cbBufferCount, _uint8 type);
    //是否有此牌值
    bool IsHaveCard(_uint8 cbCardBuffer[], _uint8 cbCard, _uint8 cbBufferCount);
	//是否有一张这样的牌
	bool IsHaveCardList(_uint8 cbCardBuffer[], _uint8 cbBufferCount, _uint8 cbCardList[],_uint8 cbListCount);

    bool HaveSmallBullType(_uint8 cbCardBuffer[], _uint8 cbCard[], _uint8 cbOut[], _uint8 cbBufferCount);

public:
    //排列扑克(按牌值大小)
    void SortCardList(_uint8 cbCardData[], _uint8 cbCardCount);
    //混乱扑克
    void RandCardList(_uint8 cbCardBuffer[], _uint8 cbBufferCount);
	//好牌算法
	int GoodRandCardList(_tint32 count[], bool is_robot[], _uint8 cbCardBuffer[], _uint8 cbBufferCount, 
		_uint8 cbPlayerCount, _uint8 GoodCardBuff[GAME_PLAYER_COUNT][MAX_COUNT], _uint8 GoodChairId[]);

    //排列扑克(按数据大小)///XXX
    void SortCardDataList(_uint8 cbCardData[], _uint8 cbCardCount);
    //排列炸弹顺序
    void SortBom(_uint8 cbCardBuffer[], _uint8 cbCardOut[], _uint8 cbBufferCount);
	//删除扑克
	bool RemoveCard(const _uint8 cbRemoveCard[], _uint8 cbRemoveCount, _uint8 cbCardData[], _uint8 cbCardCount);
    //功能函数
public:
    //逻辑数值
    _uint8 GetCardLogicValue(_uint8 cbCardData);
    //对比扑克
    bool CompareCard(_uint8 cbFirstData[], _uint8 cbNextData[], _uint8 cbCardCount, _uint8 cbFiretIsGive, _uint8 cbNextIsGive);
	//对比扑克
	bool CompateCard(_uint8 cbFirstData[], _uint8 cbNextData[], _uint8 cbCardCount);
};
#endif