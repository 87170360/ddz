#ifndef _XT_BEAUTY_DECK_H_
#define _XT_BEAUTY_DECK_H_



#define XT_CARD_SUIT_NU  4          ///��ɫ��Ŀ
#define XT_CARD_SINGLE_SUIT_NU 13   ///---������ɫ���Ƶ���Ŀ
#define XT_MAX_CARD_TYPE 6          ///---����������Ŀ


#include <stdint.h>

#include "XtHoleCards.h"



///---����ƽ̨
class XtTypeDeck 
{
	public:
		XtTypeDeck();
		~XtTypeDeck();

	public:
		virtual int shuffle(int seed);    ///ϴ��
		virtual void fill();
		virtual void empty();
		virtual int count() const ;
		virtual int getHoleCards(XtHoleCards* card);

		virtual int getHoleCards(XtHoleCards* card,int type);
		virtual int changeHoleCards(int pos,XtHoleCards* hole_cards);

	public:
		void setTypeWeight(int* weight);

	protected:
		int randomNum(int start,int end);

		int randomCard(int start,int end,int suit);
		int randomCardWithMark(int start,int end,int suit,unsigned int mark);

		int randomType();


		int getBaoZi(XtHoleCards* card);
		int getShunJin(XtHoleCards* card);
		int getJinHua(XtHoleCards* card);
		int getShunZi(XtHoleCards* card);
		int getDuiZhi(XtHoleCards* card);
		int getGaoPai(XtHoleCards* card);

		int cardIsShunZi(int a,int b, int c);


		int pop(XtCard* card);


	protected:
		int getBaoZiByCard(XtHoleCards* card,int suit,int face);
		int getShunJinByCard(XtHoleCards* card,int suit,int face);
		int getShunZiByCard(XtHoleCards* card,int face);

		int getDuiZhiByCard(XtHoleCards* card,int face);

	private:
		uint8_t m_cardMask[XT_CARD_SUIT_NU][XT_CARD_SINGLE_SUIT_NU];   ///---�Ƶ�����

		int m_typeWeight[XT_MAX_CARD_TYPE];
		int m_typeTotal;
};


#endif /*_XT_BEAUTY_DECK_H_*/



