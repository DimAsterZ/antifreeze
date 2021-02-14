#ifndef HANDLE_HPP
#define HANDLE_HPP
#include <functional>

#include "antifreeze_global.h"

namespace antifreeze
{

// Идентифицирует событие
// EventHandler "любит" на эту идентификацию подписываться
// commandID и messageParam - это может быть что угодно, например, std::hash<std::string>
struct ANTIFREEZESHARED_EXPORT Handle
{
	unsigned long long commandID;		// как вариант - это может быть команда "onPressed"
	unsigned long long messageParam;	// как вариант - это может быть ID'шник контрола на интерфейсе (MYBUTTON_ID)
	
	bool operator<(const Handle &right) const
	{
		if (this->commandID < right.commandID) {
			return true;
		}

		if (this->commandID > right.commandID) {
			return false;
		}
		
		if (this->messageParam < right.messageParam) {
			return true;
		}
		
		return false;
	}
	
	bool operator==(const Handle &right) const
	{
		return ( (this->commandID == right.commandID) &&
				 (this->messageParam == right.messageParam) );
	}

	bool operator!=(const Handle &right) const
	{
		return ( ! operator==(right) );
	}
};

} // namespace antifreeze

#endif // HANDLE_HPP
