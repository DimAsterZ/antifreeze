#ifndef MESSAGEDATA_H
#define MESSAGEDATA_H

#include <string>
#include <memory>
#include "Handle.hpp"

#include "antifreeze_global.h"

namespace antifreeze
{

// Чтобы передать в 'PostMessage' свои данные:
// 1. наследуем свою структуру с данными от 'ConstData' и заполняем "свими делами";
// 2. пихаем её unique_shared'ом в 'MessageData' (и, сопсна, постаем далее "куда хотим")
// Excusez moi за мой французский.
struct ANTIFREEZESHARED_EXPORT ConstData
{
	Handle handle;// может содержать хэши строк, что ниже:
	std::string strCommandID;	// например "onPress"
	std::string strMessageParam;// например "MYBUTTON_ID"
	
	ConstData();
	ConstData(const ConstData &data);
	virtual ~ConstData();
	ConstData &operator=(const ConstData &data);
};

// Смысл этого класса в том, что бы вернуть константные данные,
// которые могут быть обработаны в один момент в разных обработчиках(EventHandler),
//    находящихся в разных потоках и принадлежащих разным Реакторам.
class ANTIFREEZESHARED_EXPORT MessageData final
{
public:
	explicit MessageData(std::unique_ptr<ConstData> &data)
	{
		m_data = std::move(data);
	}

	const std::shared_ptr<const ConstData> &getData()
	{
		return m_data;
	}
	
private:
	std::shared_ptr<const ConstData> m_data;
};

} // namespace antifreeze

#endif // MESSAGEDATA_H
