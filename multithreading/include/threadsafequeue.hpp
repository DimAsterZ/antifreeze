#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include <atomic>


namespace multithread
{
template<typename T>
class SimpleQueue
{
private:
    mutable std::mutex m_mutex;
    std::queue< std::shared_ptr<T> > m_dataQueue;
    std::condition_variable m_dataCondition;
    std::atomic<size_t> m_maxSize;
    
public:
        SimpleQueue(): m_maxSize(2147483647)
        // 2147483647 - максимальное положительное число для 32bit int'а
	{
	}
	
        void setMaxSize(size_t size)
	{
	    m_maxSize = size;
	}
	
        size_t getMaxSize()
	{
	    return m_maxSize;
	}
	
	// выдёргивает (насовсем) элемент из очереди, но если очередь пуста - заставляет поток ждать (пока не появится, что выдернуть)
	void waitAndPop(T& value)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_dataCondition.wait(lock, [this]{ return !m_dataQueue.empty(); });
		value = std::move(*m_dataQueue.front());
		m_dataQueue.pop();
	}
	
	// выдёргивает (насовсем) элемент из очереди, но если очередь пуста - заставляет поток ждать (пока не появится, что выдернуть)
	std::shared_ptr<T> waitAndPop()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_dataCondition.wait(lock, [this]{ return !m_dataQueue.empty(); });
		std::shared_ptr<T> result = m_dataQueue.front();
		m_dataQueue.pop();
		return result;
	}
	
	// выдёргивает (насовсем) элемент из очереди, но если очередь пуста - НЕ будет заставлять поток ждать (пока не появится, что выдернуть)
	// вернёт успешность операции
	bool tryAndPop(T& value)
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		bool result = false;
		
		if (!m_dataQueue.empty()) {
			value = std::move(*m_dataQueue.front());
			m_dataQueue.pop();
			result = true;
		}
		
		return result;
	}
	
	// выдёргивает (насовсем) элемент из очереди, но если очередь пуста - НЕ будет заставлять поток ждать (пока не появится, что выдернуть)
	// возвращает результат, если он есть, либо пустой поинтер.
	std::shared_ptr<T> tryAndPop()
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		
		if (m_dataQueue.empty()) {
			return std::shared_ptr<T>();
		}
		std::shared_ptr<T> result = m_dataQueue.front();
		m_dataQueue.pop();
		return result;
	}
	
	// помещает элемент в очередь и говорит одному из ждущих потоков:	Я пришел к тебе с приветом,
	//									Рассказать, что солнце встало,
	//									Что оно горячим светом
	//									По листам затрепетало... (Афанасий Фет)
	//
	// после чего поток, уснувший (если такой был) в функции waitAndPop, просыпается и забирает этот новый элемент
	bool push(T new_value)
	{
		std::shared_ptr<T> data( std::make_shared<T>(std::move(new_value)) );
		std::lock_guard<std::mutex> guard(m_mutex);
	
                if (m_dataQueue.size() >= m_maxSize) {
		    return false;
		}
		m_dataQueue.push(data);
		m_dataCondition.notify_one();
		return true;
	}
	
	bool empty() const
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		return m_dataQueue.empty();
	}

	size_t size() const
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		return m_dataQueue.size();
	}

	void clear()
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		std::queue<std::shared_ptr<T>> emptyQueue;
		std::swap(m_dataQueue, emptyQueue);
	}
};

}
#endif // THREADSAFEQUEUE_H
