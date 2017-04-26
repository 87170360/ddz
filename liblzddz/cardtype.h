#ifndef _CARD_TYPE_H_
#define _CARD_TYPE_H_

enum CardType
{
	CT_ERROR                = 0,			// 错误类型
	CT_ROCKET               = 1,			// 火箭
	CT_BOMB                 = 2,			// 炸弹
	CT_SHUTTLE_0            = 3,			// 航天飞机0翼
	CT_SHUTTLE_2            = 4,			// 航天飞机2翼(同或异)
	CT_AIRCRAFT_0           = 5,			// 飞机0翼
	CT_AIRCRAFT_1           = 6,			// 飞机1翼
	CT_AIRCRAFT_2S          = 7,			// 飞机2同翼
	CT_4AND2_2S             = 8,			// 四带二2同翼
	CT_4AND2_2D             = 9,			// 四带二2异翼
	CT_4AND2_4              = 10,			// 四带二4翼(2对)
	CT_DOUBLE_STRAIGHT      = 11,			// 双顺 
	CT_STRAIGHT             = 12,			// 单顺 
	CT_THREE_0              = 13,			// 三带0翼
	CT_THREE_1              = 14,			// 三带1翼
	CT_THREE_2S             = 15,			// 三带2同翼
	CT_PAIR                 = 16,			// 对子
	CT_SINGLE               = 17,			// 单牌
};

enum DivideType
{
    DT_4                    = 0,            
    DT_3                    = 1,            
    DT_2                    = 2,            
    DT_1                    = 3,            
    DT_ROCKET               = 4,            
    DT_STRAITHT             = 5,            //单顺
    DT_DS                   = 6,            //双顺 
    DT_AIRCRAFT             = 7,            //三顺
    DT_LZ                   = 8,            //癞子
};

#endif /* _CARD_TYPE_H_ */


