#ifndef _XT_PROTOCOL_H_
#define _XT_PROTOCOL_H_ 


class XtProtocol 
{
	public:
		void appendData(uint8_t* data,int length);

	public:
		XtBuffer* popData();

	public:
		XtProtocol();
		~XtProtocol();

	private:
		int m_state;
		int m_headerLength;
		std::vector<uint8_t> m_reciveData;
		std::queue<XtBuffer*> m_datas;
};




#endif /*_XT_PROTOCOL_H_*/





