#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "threadsafequeue.hpp"


#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>

namespace multithread
{
// ждет завершения всех потоков
class ThreadsJoiner
{
	std::vector<std::thread>& m_threads;
		
public:
	explicit ThreadsJoiner(std::vector<std::thread>& threads) : m_threads(threads)
	{}
	
	~ThreadsJoiner()
	{
		for (unsigned long i = 0; i < m_threads.size(); ++i) {
			
			if (m_threads[i].joinable()) {
				m_threads[i].join();
			}
		}
	}
};

class SimpleThreadPool
{
	
public:
	// Если intensity == true(не рекомендуется),
	//      то потоки будут ожидать таски постоянно опрашивая очередь и не засыпая,
	//	    таким образом реакция на появление новой задачи станет более быстрой, 
	//	    но в паузах, когда нечего делать, будет лишняя нагрузка на процессор.
	ThreadPool(unsigned int threadCount = 0, bool intensity = false) 
		: m_intensity(intensity), m_threadCount(0), m_done(false),
		  m_joiner(m_threads)
	{
		if ( 0 == threadCount ) {
			threadCount = std::thread::hardware_concurrency(); // сколько ядер - столько потоков
		}
		increaseThreads(threadCount);
	}
	
	~ThreadPool()
	{
		m_done = true; // говорим потокам - пора закругляться
		
		{// разбудим уснувшие внутри очерди потоки если такие есть
			auto blankFunc = [](){};
			for (unsigned int i = 0; i < m_threadCount; ++i) {
				m_workQueue.push(blankFunc);
			}
		}
	}
	
	unsigned int getThreadsCount()
	{
		return m_threadCount;
	}
	
	//Увеличиваем количество потоков.
	//Изначально, количество потоков равно числу физических потоков CPU.
	void increaseThreads(unsigned int threadsCount)
	{
		m_threadCount += threadsCount;
		
		try {
			
			if (false == m_intensity) {
				
				for (unsigned i = 0; i < threadsCount; ++i) {
					m_threads.push_back(std::thread(&ThreadPool::workerThread, this));
				}
			}
			else {
				
				for (unsigned i = 0; i < threadsCount; ++i) {
					m_threads.push_back(std::thread(&ThreadPool::workerThread_intense, this));
				}
			}
		}
		catch (...) {
			m_done = true;
			throw;
		}
	}
	
	// помещает функцию/функтор в очередь работ, для исполнения содержащегося там кода нашими потоками.
	template<typename FunctionType>
	bool submit(FunctionType f)
	{
		return m_workQueue.push( std::function<void()>(f) );
	}
	
	size_t queueSize()
	{
	    return m_workQueue.size();
	}
	
private:
	int m_intensity;
	unsigned int m_threadCount;
	SimpleQueue< std::function<void()> > m_workQueue; // потокобезопасная очередь работ, наши потоки тянут оттуда себе задачи 
	std::atomic< bool > m_done; // говорит потокам - хватит таскать таски, пора заканчивать работу
	std::vector< std::thread > m_threads; // наши потоки, создаем их в конструкторе
	ThreadsJoiner m_joiner; // это джойнер, он всем нашим потокам сделает ".join" при уничтожении пула
	
	// тут потоки ищут над чем бы поработать, пока им не скажут: "m_done = true", т.е. хватит.
	void workerThread_intense()
	{
		while (!m_done) {
			std::function<void()> task;
			
			if ( m_workQueue.tryAndPop(task) ) {
				
				try {
					task();
				}
				catch (const std::bad_function_call& e) {
					std::cerr << e.what() << std::endl;
				}
			}
			else {
				std::this_thread::yield();
			}
		}
	}
	
	// тут потоки ищут над чем бы поработать, пока им не скажут: "m_done = true", т.е. хватит.
	void workerThread()
	{
		while (!m_done) {
			std::shared_ptr< std::function<void()> > func;
			func = m_workQueue.waitAndPop( );
			
			if ( nullptr != func ){
				
				try {
					(*func)();
				}
				catch (const std::bad_function_call& e) {
					std::cerr << e.what() << std::endl;
				}
			}
			else {
				std::this_thread::yield();
			}
		}
	}
};

}

#endif // THREADPOOL_H

