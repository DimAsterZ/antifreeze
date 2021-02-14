#include "MessageData.h"

namespace antifreeze
{

ConstData::ConstData()
{
	
}

ConstData::ConstData(const ConstData &data) : handle(data.handle),
	strCommandID(data.strCommandID), strMessageParam(data.strMessageParam)
{
	
}

ConstData::~ConstData()
{
	
}

ConstData &ConstData::operator=(const ConstData &data)
{
	handle = data.handle;
	strCommandID = data.strCommandID;
	strMessageParam = data.strMessageParam;
	return *this;
}

} // namespace antifreeze
