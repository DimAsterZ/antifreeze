#ifndef ASYNCOPERPROCESSOR_H
#define ASYNCOPERPROCESSOR_H

#include <map>
#include <memory>
#include <vector>
#include <set>
#include <sstream>

#include "MessageData.h"
#include "Handle.hpp"

#include "EventHandler.h"
#include "Reactor.h"

#include "threadsafemap.hpp"
#include "atomiccounter.hpp"

#include "antifreeze_global.h"

namespace antifreeze
{

// Синглтон.
// Основной класс для межРеакторного взаимодействия. 
// Процессор асинхронных операций.
// Не создает собственного потока, но предоставляет другим потокам свой
// функционал в потокобезопасном режиме. 
class ANTIFREEZESHARED_EXPORT AsyncOperProcessor
{
public:
	
	// Класс запускает Реакторы и ведёт учет запущенных
	class ANTIFREEZESHARED_EXPORT StartReactorDispatcher
	{
		StartReactorDispatcher(const StartReactorDispatcher &) = delete;
		StartReactorDispatcher &operator=(const StartReactorDispatcher &) = delete;
		
	public:
		StartReactorDispatcher()
		{
			AsyncOperProcessor::instance().m_startedReactorNumbers ++;
			AsyncOperProcessor::instance().startReactorDispatcher();
		}
		
		~StartReactorDispatcher()
		{
	// Вызывать тут shutdownReactorDispatcher() не нужно, так как эта функция к данному
	// моменту уже будет вызвана из EventHandler'а  этого потока.
			AsyncOperProcessor::instance().m_startedReactorNumbers --;
		}
	};

	friend class StartReactorDispatcher;

	static AsyncOperProcessor &instance()
	{
		static AsyncOperProcessor theSingleInstance;
		return theSingleInstance;
	}
	
	// останавливает и удаляет ReactorDispatcher из межРеакторного взаимодействия
	void shutdownReactorDispatcher();
	
	// регистрирует класс(EventHandler), реагирующий на определенные сообщения
	template<typename ReactorType>
	bool registerHandler(EventHandler *handler)
	{
		static_assert( std::is_base_of<Reactor, ReactorType>::value,
					   "Need class derived from 'Reactor'");

		// Проверка 'if(handler->m_deregistering)' продублирована для того чтобы
		// отдать приоритет дерегистрации. См. функцию 'deregisterHandler()'.
		if (handler->m_deregistering)
		{
			return false;
		}
		multithread::AtomicCounter ac(handler->m_registeringThreadsCounter);
		if (handler->m_deregistering)
		{
			return false;
		}
		
		size_t reactId = registerReactor<ReactorType>();
		const std::vector<Handle> &handles = handler->getHandles();
		handler->onRegister();
		
		for ( const Handle &handle : handles ) {
			std::lock_guard<std::shared_mutex> lk(m_mainMapMutex);
			m_mainMap[handle][reactId].insert(handler);
		}
		
		return true;
	}

	// Регистрирует класс(EventHandler), реагирующий на определенные сообщения.
	// Для случая, когда нам известен идентификатор потока Reactor'a,
	// которому должен принадлежать регистрируемый EventHandler.
	bool registerHandler(EventHandler *handler, size_t threadID)
	{

		// Проверка 'if(handler->m_deregistering)' продублирована для того чтобы
		// отдать приоритет дерегистрации. См. функцию 'deregisterHandler()'.
		if (handler->m_deregistering)
		{
			return false;
		}
		multithread::AtomicCounter ac(handler->m_registeringThreadsCounter);
		if (handler->m_deregistering)
		{
			return false;
		}

		size_t reactId;
		if ( ! m_threadToReactor.read(threadID, reactId) ) {
			return false;
		}

		const std::vector<Handle> &handles = handler->getHandles();
		handler->onRegister();

		for ( const Handle &handle : handles ) {
			std::lock_guard<std::shared_mutex> lk(m_mainMapMutex);
			m_mainMap[handle][reactId].insert(handler);
		}

		return true;
	}

	// Убирает (дерегистрирует, отписывает) EventHandler из системы.
	// ВНИМАНИЕ! ПОДВОДНЫЕ КАМНИ!
	//   Данная функция отпишет EventHandler, только от новых(!) сообщений, но в
	// очереди Reactor'а (или даже в нескольких очередях) могут находиться
	// сообщения, предназначенные для данного EventHandler'а, которые уже были 
	// отправлены, но ещё не успели обработаться.
	//
	//   Чем это чревато? 
	// Тремя проблемами:
	//  1) После дерегистрации EventHandler'а, он продолжит обрабатывать оставшиеся 
	// сообщения в то время, как Ваш код будет думать, что всё уже решено.
	//  2) Вы не можете удалить объект класса EventHandler до того, как из 
	// очереди исчезнут все обрабатываемые им сообщения. "Crash! Boom! Bang!"
	//  3) Третья проблема заключается в том, что решение первых двух проблем 
	// усложнеятся или при регистрации EventHandler'а в нескольких реакторах,
	// или в случае, когда несколько потоков могут слать сообщения одному и 
	// тому же EventHandler'у, или и то и другое ("... и можно без хлеба"((с)Пух)).
	// В общем, когда EventHandler работает в многопоточном режиме.
	// 
	//   Есть ли решение?
	// Да. Вот примерные варианты по номерам проблем:
	//  1) Заведите в EventHandler'е флаг 'bool m_isRegistered = true' и 
	// публичный сеттер. Перед дерегистрацией сбрасывайте EventHandler'у флаг в
	// 'false'. В функции 'handleEvent()' проверяйте флаг 
	// 'if ( ! m_isRegistered ) return;'
	//  2) Решение второй проблемы 'немало доставляет' - готовьтесь... 
	// Ok, понятно, удалять хэндлер нельзя, но как узнать когда можно удалить? Никак!
	// Но есть вариант заставить EventHandler'а самого сказать когда его
	// удалить можно. Мы никак не узнаем остались ли в очереди Reactor'а
	// сообщения. Но если перед дерегистрацией мы пошлем для нашего 
	// EventHandler'а сообщение-маркер о его ликвидации - оно встанет в очередь 
	// последним. Улавливаете мысль? Она заключается в том, что когда 
	// EventHandler получит наше последнее сообщение-маркер - это и будет тот
	// момент, когда его (EventHandler) можно удалять. Об этом моменте 
	// EventHandler может оповестить нас любым удобным способом - по желанию.
	// Например, послать в ответ сообщение или взвести разделяемый флажок.
	// Казалось бы достаточно просто, НО...
	//  3) Но предложенные выше варианты, работают только для одного случая:
	// если поток, который дерегистрирует, является единственным потоком, 
	// который шлёт сообщения дерегистрируемому EventHandler'у, при этом 
	// EventHandler зарегистрирован в единственном Reactor'е.
	// Да-да, никто не говорил, что многопоточность - это просто.
	//   В многопоточном варианте, по первому случаю, Вы не можете быть уверены 
	// в том, что при сбросе флага 'm_isRegistered', код EventHandler'а не будет
	// исполняться, т.к. параллельный поток может уже проскочить мимо строчки
	// 'if ( ! m_isRegistered ) return;', поэтому критические участки кода,
	// (включая строчку выше) нужно заворачивать в блокировки. Решение в лоб -
	// - завернуть в мьютекс всё тело функции 'handleEvent()', но, дамы и 
	// господа - это медленно. Придется колдовать над более тонкими блокировками.
	// Если функция 'handleEvent()' короткая, то не грех подумать про один общий мьютекс.
	//   По второму случаю, Вы не можете быть уверены в том, что параллельный
	// поток не вставит свои пять копеек между вашими вызовами строчек
	// 'postMessage(marker-message)' и 'deregisterHandler(...)'. И может 
	// получится, что Ваше маркерное сообщение будет в очереди не последним, и
	// Вы снова получите "Crash! Boom! Bang!", удалив EventHandler раньше времени.
	// ПОЭТОМУ маркерное сообщение для EventHandler'а отправит за вас сама
	// функция 'deregisterHandler()', дав гарантию того, что оно будет последним
	// сообщением для дерегистрируемого EventHandler'а.
	// Главное условие - сообщение-маркер должно предназначаться только 
	// (исключительно) для дерегистрируемого EventHandler'а.
	// Можно использовать, например, следующий 'handle':
	// handle.commandID <- "marker_deregister"
	// handle.messageParam <- (указатель на EventHandler)
	//
	// Чтоб сильно не ломать мозг, юзаем вместо EventHandler 
	// - DeregisterableHandler.
	// функция 'deregisterHandler()' встроена в 'DeregisterableHandler'.
	// Поэтому Вы можете осуществить дерегистрацию просто вызвав 
	// функцию 'deregister()':
	//	{
	//		DeregisterableHandler derHandler;
	//		/*** что-то делаем ***/
	//		derHandler.deregister();
	//	}
	
	// overflows - см. postMessage(...)
	bool deregisterHandler(EventHandler *handler,
			bool isBlocking = false,
			const std::shared_ptr<MessageData> &marker = nullptr,
			std::map<unsigned long long, std::set<EventHandler *> > *overflows = nullptr);
	
	// Посылает сообщение всем классам(хэндлерам), которые на него подписались.
	// Если на данное сообщение никто не подписан - возвращает 'false'.
	// Параметр overflows будет не пустым при переполнении очереди реактора(class Reactor)
	//  и будет содержать его(реактора) ID(или ID нескольких реакторов) и набор указателей
	//	на хэндлеры(EventHandler*), которые не смогли получить сообщение.
	//		При переполнении опасно слать повторные сообщения используя ID реактора, т.к.
	//		реактор имеет локальное время жизни. Если планируется возможность переполнения
	//		очереди, то лучше решать вопрос посылкой сообщений, добавляя в них указатели
	//		на EventHandler*. И фильтровать такие сообщения внутри ваших хэндлеров.
	//	Если переполнение не планируется, то параметр overflows можно игнорировать.
	bool postMessage(const std::shared_ptr<MessageData> &msg,
					 std::map< unsigned long long,
					 std::set<EventHandler *> > *overflows = nullptr);
	
	bool isHandlerRegistered(EventHandler *handler, const Handle &handle);
	
	bool isAllReactorsStopped() {
		return 0 == m_startedReactorNumbers;
	}

	std::shared_ptr<MessageData> waitInLoop(EventHandler *handler,
											const Handle &handle) 
	{
		size_t reactorID;
		size_t threadID;
		
		if ( ! getReactorID(reactorID, threadID) ) {
			return nullptr;
		}

		std::shared_ptr<Reactor> reactor = getReactor(reactorID);
		
		return reactor->waitInLoop(handler, handle);
	}
	
	std::string getDebugInfo() {
		std::stringstream sstream;
		sstream << "m_mainMap: " << m_mainMap.size() << std::endl;
		
		std::shared_lock<std::shared_mutex> lk(m_reactorsMutex);
		
		sstream << "m_reactors: " << m_reactors.size() << std::endl;
		
		for ( auto &elem: m_reactors ) {
			sstream << std::boolalpha << (elem == nullptr) << std::endl;
		}
		sstream << "m_threadToReactor: " 
				<< m_threadToReactor.size() << std::endl;
		return sstream.str();
	}
	
private:
	mutable std::shared_mutex m_mainMapMutex;
	mutable std::shared_mutex m_reactorsMutex;
	
	// Количество запущенных реакторов
	std::atomic<int> m_startedReactorNumbers; 

	// запускает ReactorDispatcher принадлежащий запускающему потоку
	void startReactorDispatcher();
	
	// возвращает идентификатор класса-Реактора(reactorID),
	// который принадлежит вызывающему потоку(threadID)
	// bool - отвечает существует ли такой реактор
	bool getReactorID(size_t &reactorID, size_t &threadID);
	
	// Помещает сообщение в очередь Реактора.
	// Возвращает false если очередь переполнена или передан несуществующий reactId.
	bool eventToReactor(size_t reactId, EventHandler * handler,
						const std::shared_ptr<MessageData> &message);

	// вспомогательная, потоко-не-безопасная функция
	inline bool unlockedPostMessage(const std::shared_ptr<MessageData> &msg,
					std::map< unsigned long long,
							  std::set<EventHandler *>
							> *overflows = nullptr);

	// удаляем Handle из главной map'ы
	void unlockedRemoveHandle(const Handle &handle);
	
	// возвращает ID реактора (попутно инстанцировав такой(реактор) если небыло)
	// ID реактора - привязано(т.е. Реактор принадлежит) к вызывающему потоку
	template<typename ReactorType>
	size_t registerReactor()
	{
		size_t reactorID;
		
		size_t threadID;
		ReactorType *freshReactor = nullptr;
		
		if ( ! getReactorID(reactorID, threadID) ) {
			std::lock_guard<std::shared_mutex> lk(m_reactorsMutex);
			
			reactorID = m_reactors.size();
			std::shared_ptr<ReactorType> reactor =
					std::make_shared<ReactorType>();
			freshReactor = reactor.get();

			for ( size_t id = 0; id < m_reactors.size(); id++ ) {
				
				if ( nullptr == m_reactors[id] ) {
					reactorID = id;
					m_reactors[reactorID] = reactor;
					break;			
				}
			}
			
			if ( reactorID == m_reactors.size() ) {
				m_reactors.push_back(reactor);
			}
			
			m_threadToReactor.write(threadID, reactorID);
		}
		
		if ( nullptr != freshReactor ) {
			freshReactor->auxInit();
		}
		
		return reactorID;
	}	

	multithread::SimpleMap< size_t/*threadID(hash)*/,
								size_t/*reactorID*/ > m_threadToReactor;
	std::map<  Handle, 
				std::map< size_t /*reactorID*/,
							std::set<EventHandler* /*handler*/> >  > m_mainMap;
	std::vector< std::shared_ptr<Reactor> > m_reactors;
	inline std::shared_ptr<Reactor> getReactor(size_t reactorID)
	{
		std::shared_ptr<Reactor> reactorPtr;
				
		{
			std::shared_lock<std::shared_mutex> lk(m_reactorsMutex);
			reactorPtr = m_reactors[reactorID];
		}
		
		return reactorPtr;
	}
	
	AsyncOperProcessor(): m_startedReactorNumbers(0)
	{
		
	}
	AsyncOperProcessor(const AsyncOperProcessor &root) = delete;
	AsyncOperProcessor &operator=(const AsyncOperProcessor &) = delete;
};

} // namespace antifreeze

#endif // ASYNCOPERPROCESSOR_H
