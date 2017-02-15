#ifndef _XT_SHULLE_DECK_H_
#define _XT_SHULLE_DECK_H_

#include "XtDeck.h"

#include "XtCard.h"


///---Ï´ÅÆÆ½Ì¨
class XtShuffleDeck:public XtDeck 
{
	public:
		XtShuffleDeck();
		virtual ~XtShuffleDeck();

	public:
		virtual void shuffle(int seed);
		virtual void fill();
		virtual void empty();
		virtual int count() const;
		virtual int getHoleCards(XtHoleCards* hole_card);
		virtual int changeHoleCards(int pos,XtHoleCards* hole_card);

	protected:
		bool push(const XtCard& card);
		bool pop(XtCard* card);

	private:
		vector<XtCard> m_cards;
};


#endif /*_XT_SHULLE_DECK_H_*/



