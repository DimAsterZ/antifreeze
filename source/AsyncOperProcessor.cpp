#include "AsyncOperProcessor.h"
#include <thread>

static std::atomic_flag destroyReactorSpinlock = ATOMIC_FLAG_INIT;

namespace antifreeze
{

void AsyncOperProcessor::startReactorDispatcher()
{
	size_t reactorID;
	size_t threadID;

	if ( getReactorID(reactorID, threadID) ) {
		getReactor(reactorID)->handleEvents();
	
		while ( destroyReactorSpinlock.test_and_set() ) {
			;
		}
		{	
			std::shared_lock<std::shared_mutex> lk(m_reactorsMutex);
			m_reactors[reactorID] = nullptr; // потокобезопасное самоуничтожение Реактора
		}
		destroyReactorSpinlock.clear();
	}
}

void AsyncOperProcessor::shutdownReactorDispatcher()
{
	size_t reactorID;
	size_t threadID;
	
	if ( ! getReactorID(reactorID, threadID ) ) {
		return;
	}
	
	getReactor(reactorID)->exit();
	m_threadToReactor.erase(threadID);//Убираем Реактор из map`ы thread-reactor

	//Удаляем всё относящееся к Реактору-Диспатчеру из m_mainMap
	{
		std::lock_guard<std::shared_mutex> lk(m_mainMapMutex);
		
		for ( auto itMain = m_mainMap.begin(); itMain != m_mainMap.end(); ) {
			std::map<size_t, std::set<EventHandler*>> &reactMap = itMain->second;
			reactMap.erase(reactorID);
			
			if ( reactMap.empty() ) {
				m_mainMap.erase(itMain ++);
			}
			else {
				++ itMain;
			}
		}
	}
}

bool AsyncOperProcessor::deregisterHandler(EventHandler *handler, bool isBlocking,
		const std::shared_ptr<MessageData> &marker,
		std::map<unsigned long long, std::set<EventHandler *>> *overflows)
{
	handler->m_deregistering = true;

	while (handler->m_registeringThreadsCounter){
		std::this_thread::yield();
	}
	
	const auto &handles = handler->getHandles();

	{// lock_guard
		std::lock_guard<std::shared_mutex> lk(m_mainMapMutex);
		if ( nullptr != marker ) {
			bool isMsgPosted = unlockedPostMessage(marker, overflows);
			if (!isMsgPosted) {
				return false;
			}
		}

		for ( const auto &handle : handles ) {
			auto itMain = m_mainMap.find(handle);

			if ( m_mainMap.end() == itMain ) {
				continue;
			}
			std::map<size_t, std::set<EventHandler*>> &reactMap = itMain->second;

			for ( auto itReact = reactMap.begin(); itReact != reactMap.end(); ) {
				std::set<EventHandler*> &handlerSet = itReact->second;

				handlerSet.erase(handler);

				if ( handlerSet.empty() ) {
					reactMap.erase(itReact ++);
				}
				else {
					++ itReact;
				}
			}

			if ( reactMap.empty() ) {
				m_mainMap.erase(itMain);
			}
		}

		if ( nullptr != marker ) {
			unlockedRemoveHandle(marker->getData()->handle);
		}
	}

	if (isBlocking) {
		while (handler->m_threadsCounter) {
			std::this_thread::yield();
		}
	}
	
	return true;
}

bool AsyncOperProcessor::postMessage(const std::shared_ptr<MessageData> &msg,
				std::map<unsigned long long, std::set<EventHandler *>> *overflows)
{
	std::shared_lock<std::shared_mutex> lk(m_mainMapMutex);
	
	return unlockedPostMessage(msg, overflows);
}

bool AsyncOperProcessor::unlockedPostMessage(const std::shared_ptr<MessageData> &msg,
				std::map<unsigned long long,  std::set<EventHandler *>> *overflows)
{
	auto &dataPtr = *msg->getData();
	auto it = m_mainMap.find(dataPtr.handle);
	
	if ( it == m_mainMap.end() ) {
		return false;
	}
	
	for ( const auto &reactId_HandlerSet : it->second ) {
		size_t reactID = reactId_HandlerSet.first;
		const std::set<EventHandler *> &handlerSet = reactId_HandlerSet.second;
		
		for ( EventHandler *handler : handlerSet ) {
			if ( !eventToReactor(reactID, handler, msg) && overflows ) {
				(*overflows)[reactID].insert(handler);
			}
		}
	}
	
	return true;
}

void AsyncOperProcessor::unlockedRemoveHandle(const Handle &handle)
{
	auto it = m_mainMap.find(handle);
	
	if ( it != m_mainMap.end() ) {
		m_mainMap.erase(it);
	}
}

bool AsyncOperProcessor::isHandlerRegistered(EventHandler *handler, const Handle &handle)
{
	size_t reactorID;
	size_t threadID;
	
	if ( ! getReactorID(reactorID, threadID) ) {
		return false;
	}

	std::shared_lock<std::shared_mutex> lk(m_mainMapMutex);
	auto it = m_mainMap.find(handle);
	
	if ( it == m_mainMap.end() ) {
		return false;
	}
	
	auto &handlerSet = it->second[reactorID];
	
	if ( handlerSet.find(handler) != handlerSet.end() ) {
		return true; // ура нашли
	}
	
	return false;
}

bool AsyncOperProcessor::eventToReactor(size_t reactId, EventHandler *handler,
										const std::shared_ptr<MessageData> &message)
{
	bool result = false;

	if ( reactId < m_reactors.size() ) {
	
		while ( destroyReactorSpinlock.test_and_set() ) {
            std::this_thread::yield();
		}
		{//spinlocked
			std::shared_ptr<Reactor> reactor = nullptr;
			reactor = getReactor(reactId);
			
			if ( nullptr != reactor ) {
				result = reactor->addEvent(handler, message);
			}
		}
		destroyReactorSpinlock.clear();
	}

	return result;
}

bool AsyncOperProcessor::getReactorID(size_t &reactorID, size_t &threadID)
{
	std::hash<std::thread::id> hasher;
	threadID = hasher(std::this_thread::get_id());
	
	return m_threadToReactor.read(threadID, reactorID);
}

} // namespace antifreeze
