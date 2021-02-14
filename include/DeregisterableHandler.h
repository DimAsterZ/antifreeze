#ifndef DEREGISTERABLEHANDLER_H
#define DEREGISTERABLEHANDLER_H

#include "MessageData.h"
#include "Handle.hpp"
#include "EventHandler.h"
#include <atomic>

#include "antifreeze_global.h"
namespace antifreeze
{

// EventHandler с поддержкой функционала по дерегистрации
class ANTIFREEZESHARED_EXPORT DeregisterableHandler: public EventHandler
{

protected:
	inline std::string getMarkerCommand() const
    {
        return "marker_deregister";
    }

	unsigned long long getMarkerParam() const;
	
	// Используется для переопределения вместо handleEvent.
	// Не забудьте флаги и блокировки. 
	// См. комментарии к AsyncOperProcessor::deregisterHandler().
	virtual void onHandleEvent(const std::shared_ptr<MessageData> &) {}
	
	// Вызывается, когда DeregisterableHandler готов к уничтожению.
	virtual void onDestroyable(const std::shared_ptr<MessageData> &) {}

	// Вызывается при регистрации EventHandler'а
	virtual void onRegisterContinuation() {}

public:
	DeregisterableHandler();
	DeregisterableHandler(const std::string &endmarker);
	~DeregisterableHandler() override;
	
	// Handle, на который подписываемся, для ожидания дерегистрации
	const Handle &getDeregistrationHandle();
	
	// Дерегистрируем сами себя.
	// Если возвращает 'false', handler не зарегистрирован.
	bool deregister();

	// То же, что и deregister(), но блокируется до тех пор, пока Handler
	// не закончит обрабатывать сообщения, которые успел начать к этому моменту.
	bool deregisterBlocking();

private:
	std::atomic<int> m_registrationCounter;

	inline bool deregister(bool isBlocking);

	// Вместо неё - onHandleEvent().
	virtual void handleEvent(const std::shared_ptr<MessageData> &msg) override final;

	// Ведётся подсчет количества регистраций
	virtual void onRegister() override final;

	Handle &getDeregisterHandleNonConst() override final
	{
		return EventHandler::getDeregisterHandleNonConst();
	}

};

} // namespace antifreeze

#endif // DEREGISTERABLEHANDLER_H
