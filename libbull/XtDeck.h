#ifndef _XT_DECK_H_
#define _XT_DECK_H_


#include "XtHoleCards.h"
////---发牌平台“接口类”
class XtDeck
{
	public:
		XtDeck();
		virtual ~XtDeck();

	public:
		virtual void shuffle(int seed)=0;    ///洗牌
		virtual void fill()=0;
		virtual void empty()=0;
		virtual int count() const =0 ;
		virtual int getHoleCards(XtHoleCards* hole_card)=0;
		virtual int changeHoleCards(int pos,XtHoleCards* hole_card)=0;
};

#endif /*_XT_DECK_H_*/



