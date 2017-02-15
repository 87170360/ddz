#ifndef _XT_STRUCT_PACKET_H_
#define _XT_STRUCT_PACKET_H_

#include "XtBuffer.h"


/* Type Marker 
 * Array:   AY{array_nu:uint16_t} {array_size:uin32_t}
 * Struct:	ST{struct_size:uint16_t}
 * Data:    DT{string_length:uint16_t}
 */


/* NOTE 
 * While You Read, Not Do Write 
 * While You Write, Not Do Read 
 */

///二进制包读写类
class XtStructPacket 
{
	public:
		XtStructPacket();
		XtStructPacket(XtBuffer* buffer,bool clone=true);

		virtual ~XtStructPacket();

	public:
		int readInt8(int8_t* value);
		int readInt16(int16_t* value);
		int readInt32(int32_t* value);
		int readInt64(int64_t* value);

		int readUint8(uint8_t* value);
		int readUint16(uint16_t* value);
		int readUint32(uint32_t* value);
		int readUint64(uint64_t* value);

		int readFloat(float* value); 
		int readDouble(double* value);

		int readData(XtData* value);

		int beginReadArray();
		int endReadArray();

		int beginReadStruct();
		int endReadStruct();

	public:
		int writeInt8(int8_t value);
		int writeInt16(int16_t value);
		int writeInt32(int32_t value);
		int writeInt64(int64_t value);

		int writeUint8(uint8_t value);
		int writeUint16(uint16_t value);
		int writeUint32(uint32_t value);
		int writeUint64(uint64_t value);

		int writeFloat(float value);
		int writeDouble(double value);
		

		int writeData(const XtData* data,int length)

		int writeString(const char* str,int length);
		int writeString(const char* str);

		int beginWriteArray(int data_num);
		int endWriteArray();

		int beginWriteStruct();
		int endWriteStruct();
	protected:
		int beginReadMark(int size);
		int endReadMark();

		int beginWriteMark();
		int endWrite32Mark();
		int endWrite16Mark();

	private:
		XtBuffer* m_buffer;
		
		std::vector<int> m_readMark;
		std::vector<int> m_writeMark;
};





#endif /*_XT_STRUCT_PACKET_H_*/



