#include <thread>

#include "Reactor.h"
#include "AsyncOperProcessor.h"

namespace antifreeze
{

Reactor::Reactor() : m_exit(false)
{
	m_events.setMaxSize(maxQueueSize);
}

Reactor::~Reactor(){}

void Reactor::handleEvents()
{
	ReactorEvent re;
	
	while (!m_exit) {
		
		re.message = nullptr;
		m_events.waitAndPop(re);
		
		if (!m_exit) { 
			handleEvent(re.handler,re.message);
		}
	}
}

std::shared_ptr<MessageData> Reactor::waitInLoop(EventHandler *handler,
												 const Handle &handle)
{
	if ( ! AsyncOperProcessor::instance().
		 isHandlerRegistered(handler, handle) ) {
		
		return nullptr;
	}
	
	ReactorEvent re;
	
	while (!m_exit) {

		re.message = nullptr;
		m_events.waitAndPop(re);
		
		if (!m_exit) {
			const auto &searchingHandle = re.message->getData()->handle;
			const auto &deregistrationHandle = handler->getDeregisterHandleNonConst();

			if (re.handler == handler) {

				if ( searchingHandle == handle ) {
					return re.message; // ok, нашли то что искали
				}

				if (!re.handler->isDeregistering()) {
					m_events.push(re);// вернем безобидный event в очередь
				}
				else {// сюда попадет дерегистрируемый handler
					if ( searchingHandle == deregistrationHandle ) {// маркер или нет?
						m_events.push(re);
						return nullptr;
					}
					// не маркеры - игнорим выкидывая из очереди
				}
			}
			else {
				if (!re.handler->isDeregistering()) {
					m_events.push(re);
				}
				else {// сюда попадет дерегистрируемый handler
					if ( searchingHandle == deregistrationHandle ) {// маркер или нет?
						m_events.push(re);// маркеры пушаем
					}
					// не маркеры - игнорим выкидывая из очереди
				}
			}

			std::this_thread::yield();
		}
	}
	
	return nullptr;
}

bool Reactor::addEvent(EventHandler *handler, const std::shared_ptr<MessageData> &message)
{
	ReactorEvent re = {handler, message};
	return m_events.push(re);
}

void Reactor::exit()
{
	m_exit = true;
}

} // namespace antifreeze
