#ifndef THREADSAFEMAP_H
#define THREADSAFEMAP_H

#include <shared_mutex>
#include <map>
#include <algorithm>

namespace multithread
{

template<typename Key, typename Value>
class SimpleMap
{
// Потокобезопасная std::map
public:
        SimpleMap() {}
        ~SimpleMap() {}

	SimpleMap (const SimpleMap &other)
	{
                std::shared_lock<std::shared_mutex> lkOther(other.m_write);
		std::lock_guard<std::shared_mutex> lkThis(m_write);
		m_map = other.m_map;
	}

	SimpleMap& operator=(const SimpleMap &other)
	{
		if (this != &other) {
			std::shared_lock<std::shared_mutex> lkOther(other.m_write);
			std::lock_guard<std::shared_mutex> lkThis(m_write);
			m_map = other.m_map;
		}

		return *this;
	}

	bool read(const Key &key, /*out*/ Value &value) const
	{
		std::shared_lock<std::shared_mutex> lock(m_write);
		const auto iFind = m_map.find(key);

		if (m_map.end() == iFind) {
			return false;
		}

		value = iFind->second;

		return true;
	}

	void write(const Key &key, const Value &value)
	{
		std::lock_guard<std::shared_mutex> lock(m_write);
		m_map[key] = value;
	}

	bool writeIfNotExist(const Key &key, const Value &value)
	{
		std::lock_guard<std::shared_mutex> lock(m_write);
		const auto iFind = m_map.find(key);

		if (m_map.end() != iFind) {
			return false;
		}
		
		m_map[key] = value;
		
		return true;
	}

	void erase(const Key &key)
	{
		std::lock_guard<std::shared_mutex> lock(m_write);
		m_map.erase(key);
	}

	std::size_t size() const
	{
		std::shared_lock<std::shared_mutex> lock(m_write);
		return m_map.size();
	}

	void clear()
	{
		std::lock_guard<std::shared_mutex> lock(m_write);
		m_map.clear();
	}

	template<typename Function>
	Function find(Function pred)
	{
	    std::shared_lock<std::shared_mutex> lock(m_write);
	    std::find_if(m_map.begin(), m_map.end(), pred);
	    return pred;
	}

	bool find(std::function<bool (const std::pair<Key, Value>&) > pred, /*out*/ Key &key) const
	{
		std::shared_lock<std::shared_mutex> lock(m_write);
		const auto iFind = std::find_if(m_map.begin(), m_map.end(), pred);

		if (m_map.end() == iFind) {
			return false;
		}
		
		key = iFind->first;

		return true;
	}

private:
	mutable std::shared_mutex m_write;
	std::map<Key, Value> m_map;
};

}//namespace multithread
#endif // THREADSAFEMAP_H
