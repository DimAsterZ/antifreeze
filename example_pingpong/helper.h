#ifndef HELPER_H
#define HELPER_H


#include "AsyncOperProcessor.h"
#include "DeregisterableHandler.h"
#include "DispatchReactorStoppable.h"
#include "StoppingHandler.h"

#include <iostream>

namespace antifreeze::helper
{

class Actor : public DeregisterableHandler
{
public:
	Actor(std::string_view handlestring,
		  std::function<void(const std::shared_ptr<MessageData> &)> hfunc) :
		m_handlerFunc(hfunc)
	{
		Handle handle;
		std::hash<std::string_view> hasher;
		handle = {hasher(handlestring), 0};
		addHandle(handle);
	}
	~Actor()
	{
		deregisterBlocking();
	}

	void startDispatcher()
	{
		AsyncOperProcessor::StartReactorDispatcher();
	}

protected:
	std::function<void(const std::shared_ptr<MessageData> &)> m_handlerFunc;

	void onHandleEvent(const std::shared_ptr<MessageData> &data) override
	{
		m_handlerFunc(data);
	}
};

template <typename T>
T getMessage(const std::shared_ptr<MessageData> &data)
{
	const auto &d = std::static_pointer_cast<const T>(data->getData());
	T msg = *d;
	return msg;
}

void registerHandler(EventHandler *handler)
{
	AsyncOperProcessor::instance().registerHandler<DispatchReactorStoppable>(handler);
}

std::unique_ptr<Actor> makeActor(std::string_view handle,
				std::function<void(const std::shared_ptr<MessageData> &)> hfunc)
{
	auto actor = std::make_unique<Actor>(handle, hfunc);
	registerHandler(actor.get());
	return actor;
}

template <typename T>
void postMessage(T data, std::string_view handle)
{
	static_assert( std::is_base_of<ConstData, T>::value,
				   "Need class derived from 'ConstData'");

	std::unique_ptr<T> custom = std::make_unique<T>();
	*custom = data;
	std::hash<std::string_view> hasher;
	custom->handle = {hasher(handle), 0};
	auto constData = std::unique_ptr<ConstData>(std::move(custom));
	AsyncOperProcessor::instance().postMessage(std::make_shared<MessageData>(constData));
}

} // namespace antifreeze::helper

#endif // HELPER_H
