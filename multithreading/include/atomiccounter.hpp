#ifndef ATOMICCOUNTER_HPP
#define ATOMICCOUNTER_HPP

#include <atomic>

namespace multithread
{
// Класс показывает, сколько потоков находится, в текущий момент, внутри скопа.
// Пример:
// std::atomic<int> m_threads = 0;
//
// void foo1()
// {
//    {
//       AtomicCounter counter(m_threads);
//       /*** Исполняемый код ***/
//       std::cout << m_threads; // узнаём сколько потоков внутри текущего скопа
//       /*** Исполняемый код ***/
//    }
// }
//
// void foo2()
// {
//    std::cout << m_threads; // узнаём сколько потоков внутри скопа из 'foo1()'
// }

class AtomicCounter
{
public:
	AtomicCounter(std::atomic<int> &count)
		:m_count(count)
	{
		m_count++;
	}
	
	~AtomicCounter()
	{
		m_count--;
	}

	AtomicCounter(const AtomicCounter&) = delete;
	AtomicCounter& operator=(const AtomicCounter&) = delete;
	
private:
	std::atomic<int> &m_count;
};

} //namespace multithread
#endif // ATOMICCOUNTER_HPP
