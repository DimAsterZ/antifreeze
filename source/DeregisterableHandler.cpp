#include "AsyncOperProcessor.h"
#include "DeregisterableHandler.h"
#include <chrono>

namespace antifreeze
{

DeregisterableHandler::DeregisterableHandler() : m_registrationCounter(0)
{
	std::hash<std::string> hasher;

	getDeregisterHandleNonConst().commandID = hasher(getMarkerCommand());
	getDeregisterHandleNonConst().messageParam = getMarkerParam();

	addHandle(getDeregistrationHandle());
}

DeregisterableHandler::DeregisterableHandler(const std::string &marker)
	: m_registrationCounter(0)
{
	std::hash<std::string> hasher;

	getDeregisterHandleNonConst().commandID = hasher(marker);
	getDeregisterHandleNonConst().messageParam = getMarkerParam();

	addHandle(getDeregistrationHandle());
}

DeregisterableHandler::~DeregisterableHandler()
{

}

void DeregisterableHandler::onRegister()
{
	m_registrationCounter ++;

	onRegisterContinuation();
}

void DeregisterableHandler::handleEvent(const std::shared_ptr<MessageData> &msg)
{
	if ( msg->getData()->handle == getDeregistrationHandle() ) {
		
		if ( -- m_registrationCounter == 0) {
			onDestroyable(msg);
		}
		return;
	}
	
	if (!isDeregistering()) {
		multithread::AtomicCounter ac(m_threadsCounter);
		onHandleEvent(msg);
	}
}

const Handle &DeregisterableHandler::getDeregistrationHandle()
{
	return getDeregisterHandleNonConst();
}

bool DeregisterableHandler::deregister()
{
	return deregister(false);
}

bool DeregisterableHandler::deregisterBlocking()
{
	return deregister(true);
}

bool DeregisterableHandler::deregister(bool isBlocking)
{
	std::unique_ptr<ConstData> cdata = std::make_unique<ConstData>();
	cdata->handle = getDeregistrationHandle();

	bool result = AsyncOperProcessor::instance().deregisterHandler( this, isBlocking,
		std::make_shared<MessageData>(cdata));

	return result;
}

unsigned long long DeregisterableHandler::getMarkerParam() const
{
	unsigned long long justThis = reinterpret_cast<unsigned long long>(this);

	return justThis;
}

} // namespace antifreeze
