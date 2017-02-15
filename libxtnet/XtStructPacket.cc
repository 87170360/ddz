#include "XtStructPacket.h"


XtStructPacket::XtStructPacket
{
	m_buffer=new XtBuffer;
}

XtStructPacket::XtStructPacket(XtBuffer* buffer,bool clone=true)
{
	if(clone)
	{
		m_buffer=buffer->copy();
	}
	else 
	{
		m_buffer=buffer;
	}
}


/* read signed integer */

int XtStructPacket::readInt8(int8_t* value)
{
	int ret=m_buffer->read(value,sizeof(int8_t));
	return ret;
}
int XtStructPacket::readInt16(int16_t* value)
{
	int ret=m_buffer->read(value,sizeof(int16_t));
	return ret;
}

int XtStructPacket::readInt32(int32_t* value)
{
	int ret=m_buffer->read(value,sizeof(int32_t));
	return ret;
}

int XtStructPacket::readInt64(int64_t* value)
{
	int ret=m_buffer->read(value,sizeof(int64_t));
	return ret;
}


/* read unsigned integer */
int XtStructPacket::readUint8(uint8_t* value)
{
	int ret=m_buffer->read(value,sizeof(uint8_t));
	return ret;
}
int XtStructPacket::readUint16(uint16_t* value)
{
	int ret=m_buffer->read(value,sizeof(uint16_t));
	return ret;
}

int XtStructPacket::readUint32(uint32_t* value)
{
	int ret=m_buffer->read(value,sizeof(uint32_t));
	return ret;
}

int XtStructPacket::readUint64(uint64_t* value)
{
	int ret=m_buffer->read(value,sizeof(uint64_t));
	return ret;
}


/* read float */

int XtStructPacket::readFloat(float* value)
{
	int ret=m_buffer->read(value,sizeof(float));
	return ret;
}

/* read double */
int XtStructPacket::readDouble(double* value)
{
	int ret=m_buffer->read(value,sizeof(double));
	return ret;
}

/* read string*/
int XtStructPacket::readData(XtData* value)
{
	char v[2];

	int ret=m_buffer->read(&v,sizeof(v));
	if(ret<0||v[0]!='D'||v[1]!='T')
	{
		return -1;
	}


	uint16_t str_len;
	ret=m_buffer->read(&str_len,sizeof(uint16_t));
	if(ret<0)
	{
		return -1;
	}

	if(m_buffer->nbytes()<str_len)
	{
		return -1;
	}

	value->set(m_buffer->dpos(),str_len);
	m_buffer->forward(str_len);

	return 0;
}




/* Read Array */
int XtStructPacket::beginReadArray()
{
	char v[2];
	int ret=m_buffer->read(&v,sizeof(v));
	if(ret<0||v[0]!='A'||v[1]!='Y')
	{
		return -1;
	}

	uint16_t array_nu=0;
	ret=m_buffer->read(&array_nu,sizeof(uint16_t));
	if(ret<0)
	{
		return -1;
	}

	uint32_t array_size=0;;
	ret=m_buffer->read(&array_size,sizeof(uint32_t));
	if(ret<0)
	{
		return -1;
	}

	ret=beginReadMark(array_size);
	return array_nu;

}

int XtStructPacket::endReadArray()
{
	int ret=endReadMark();
	return ret;
}

}



/* Read Struct */
int XtStructPacket::beginReadStruct()
{
	char v[2];
	int ret=m_buffer->read(&v,sizeof(v));
	if(ret<0||v[0]!='S'||v[1]!='T')
	{
		return -1;
	}

	uint16_t struct_size=0;
	ret=m_buffer->read(&struct_size,sizeof(uint16_t));
	if(ret<0)
	{
		return -1;
	}

	if(m_buffer->nbytes()<struct_size)
	{
		return -1;
	}

	m_readMark.push_back(m_buffer->nbytes()-struct_size);
	return 0;
}


int XtStructPacket::endReadStruct()
{
	endReadMark();
}




/* Write signed */
int XtStructPacket::writeInt8(int8_t value)
{
	m_buffer->append(&value,sizeof(int8_t));
	return 0;
}
int XtStructPacket::writeInt16(int16_t value)
{
	m_buffer->append(&value,sizeof(int16_t));
	return 0;
}

int XtStructPacket::writeInt32(int32_t value)
{
	m_buffer->append(&value,sizeof(int32_t));
	return 0;
}

int XtStructPacket::writeInt64(int64_t value)
{
	m_buffer->append(&value,sizeof(int64_t));
	return 0;
}


/* Write unsigned */
int XtStructPacket::writeUint8(uint8_t value)
{
	m_buffer->append(&value,sizeof(uint8_t));
	return 0;
}
int XtStructPacket::writeUint16(uint16_t value)
{
	m_buffer->append(&value,sizeof(uint16_t));
	return 0;
}

int XtStructPacket::writeUint32(uint32_t value)
{
	m_buffer->append(&value,sizeof(uint32_t));
	return 0;
}

int XtStructPacket::writeUint64(uint64_t value)
{
	m_buffer->append(&value,sizeof(uint64_t));
	return 0;
}


/* write Float */

int XtStructPacket::writeFloat(float value)
{
	m_buffer->append(&value,sizeof(float));
}


/* write double */
int XtStructPacket::writeDouble(double value)
{
	m_buffer->append(&value,sizeof(double));
}

/* write string*/
int XtStructPacket::writeString(const char* str)
{
	uint16_t length=(uint16_t)strlen(str);
	writeString(str,length);
}


int XtStructPacket::writeString(const char* str,uint16_t length)
{
	char v[2]={'D','T'};
	m_buffer->append(v,sizeof(v));
	m_buffer->append(&length,sizeof(uint16_t));
	m_buffer->append(str,length);
}



int XtStructPacket::writeData(const XtData* data,int length)
{

}

int XtStructPacket::beginWriteArray(uint16_t data_num)
{
	char v[2]={'A','Y'};
	m_buffer->append(v,sizeof(v));
	m_buffer->append(&data_num,sizeof(uint16_t));
	beginWriteMark();

	uint32_t array_size=0;
	m_buffer->append(&array_size,sizeof(uint32_t));
	return 0;

}

int XtStructPacket::endWriteArray()
{
	return endWrite32Mark();
}


int XtStructPacket::beginWriteStruct()
{
	char v[2]={'S','T'};
	m_buffer->append(v,sizeof(v));
	uint16_t struct_len=0;

	m_beginWriteMark();
	m_buffer->append(&struct_len,sizeof(uint16_t));
	return 0;



}


int XtStructPacket::endWriteStruct()
{
	return endWrite16Mark();
}








int XtStructPacket::endReadMark()
{
	if(m_readMark.size()==0)
	{
		return -1;
	}

	int bytes=m_buffer->nbytes();
	int ret_bytes=m_readMark.back();
	m_readMark.pop_back();

	if(bytes<ret_bytes)
	{
		return -1;
	}

	m_buffer->forward(bytes-ret_bytes);
	return 0;
}

int XtStructPacket::beginWriteMark()
{
	int nbytes=m_buffer->nbytes();
	m_writeMark.push_back(nbytes);
	return 0;
}

int XtStructPacket::endWrite16Mark()
{
	if(m_writeMark.size()==0)
	{
		return -1;
	}

	assert(m_buffer->nbytes>=m_writeMark.back());

	int nbytes=m_writeMark.back();
	m_writeMark.pop_back();


	*((uint16_t*)(m_buffer->dpos()+nbytes))=m_buffer->nbytes-nbytes;

	return 0;
}

int XtStructPacket::endWrite32Mark()
{
	if(m_writeMark.size()==0)
	{
		return -1;
	}

	assert(m_buffer->nbytes>=m_writeMark.back());

	int nbytes=m_writeMark.back();
	m_writeMark.pop_back();


	*((uint32_t*)(m_buffer->dpos()+nbytes))=m_buffer->nbytes-nbytes;
	return 0;
}

























