#ifndef REACTOR_H
#define REACTOR_H

#include "EventHandler.h"
#include "threadsafequeue.hpp"

#include "antifreeze_global.h"

namespace antifreeze
{

// это же ивент для хэндлера - всё просто:)
struct ANTIFREEZESHARED_EXPORT ReactorEvent
{
	EventHandler *handler;
	std::shared_ptr<MessageData> message;
};

// Реактор, он же диспетчер(диспатчер), содержит ивент луп
class ANTIFREEZESHARED_EXPORT Reactor
{
public:
	Reactor();	
	virtual ~Reactor();
	
	// Дополнительная инициализация. Вызывается после конструирования Реактора.
	// Например, можно зарегистрировать набор EventHandler'ов 'по умолчанию'.
	virtual void auxInit(){}
	
	// Цикл очереди сообщений aka massage loop (or event loop).
	// Переопределяем, и наслаждаемся его обработкой по своему вкусу.
	// Не забываем проверять m_exit - остальное по желанию.
	virtual void handleEvents();
	
	// Цикл очереди сообщений aka massage loop (or event loop).
	// ! вызывается не 'системой', а EventHandler'ом,
	//   когда он хочет подождать на месте ответ на свой запрос(PostMessage()),
	//   'не отходя от кассы'.
	// *handler - указатель на себя ждущего.
	// &handle - на что подписываемся, ожидая ответ.
	// Проверяем обязательно возвращаемое значение на 'nullptr',
	//   ибо если так то дело идет к shutdown'у,
	//   ну, и просто, чтобы не "лохануться"
	//   (например: если вы передали внутрь незареганный EventHandler ) -
	//   - тоже проверяем.
	virtual std::shared_ptr<MessageData> waitInLoop(EventHandler *handler,
													const Handle &handle);	
	
	// Добавляем в очередь сообщений - сообщение :)
	bool addEvent(EventHandler *handler,
				  const std::shared_ptr<MessageData> &message);
	
	void exit();
	
protected:
	// Ваш while в handleEvents() должен проверять m_exit.
	std::atomic<bool> m_exit; 
	
	// Очередь сообщений (ивентов, мессаг - кому как).
	multithread::SimpleQueue<ReactorEvent> m_events;
	
	// Максимальное количество event'ов-сообщений
	// , одновременно ждущих в очереди на обработку.
	// Возник вопрос откуда такое магическое число?
	//   Это повод отнаследоваться и завести своё.
	const int maxQueueSize = 100000;
	
	// Позволяет получить доступ к вызову функции 'EventHandler::handleEvent()'
	// из классов-наследников Reactor'а
	inline void handleEvent(EventHandler *handler,
							const std::shared_ptr<MessageData> &msg)
	{
		handler->handleEvent(msg);
	}
	
};

} // namespace antifreeze

#endif // REACTOR_H
