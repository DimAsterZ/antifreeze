#ifndef SPINLOCKGUARD_HPP
#define SPINLOCKGUARD_HPP

#include <atomic>
#include <thread>

namespace multithread
{

class SpinLockGuard
{
public:
	SpinLockGuard(std::atomic_flag &flag) : m_flag{flag}
	{
		while (m_flag.test_and_set()) {std::this_thread::yield();}
	}

	~SpinLockGuard()
	{
		m_flag.clear();
	}

	SpinLockGuard() = delete;
	SpinLockGuard(const SpinLockGuard &) = delete;
	SpinLockGuard(SpinLockGuard &&) = delete;
	SpinLockGuard &operator=(const SpinLockGuard &) = delete;
	SpinLockGuard &operator=(SpinLockGuard &&) = delete;

private:
	std::atomic_flag &m_flag;
};

} //namespace multithread
#endif // SPINLOCKGUARD_HPP
