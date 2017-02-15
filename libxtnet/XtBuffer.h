#ifndef _XT_BUFFER_H_
#define _XT_BUFFER_H_ 

#include <stdint.h>
///×Ö½ÚBuffer
class XtBuffer 
{

	public:
		XtBuffer();
		XtBuffer(const uint8_t* data,int len);
		virtual ~XtBuffer();

	public:

		void forward(int size);

		int read(void* data,int len);
		void append(const uint8_t* data,int len);
		const uint8_t* dpos();
		int nbytes();

		XtBuffer* copy();


	protected:
		void enlarge(int len);


	private:
		uint8_t* m_buf;

		int m_bufCap;
		int m_bufSize;
		int m_beginPos;
};


#endif /*_XT_BUFFER_H_*/


