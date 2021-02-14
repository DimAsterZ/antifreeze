#include <iostream>
#include <thread>
#include <chrono>

#include "helper.h"

using namespace antifreeze;
using namespace antifreeze::helper;

int main()
{
	struct Counter : ConstData
	{
		int counter = 0;
	};

	auto threadActor1 = [](){
		auto actor = makeActor("actor1",
				[&](const std::shared_ptr<MessageData> &data) {
					std::this_thread::sleep_for(std::chrono::seconds(1));
					auto msg = getMessage<Counter>(data);
					std::cout << "actor1: " << ++ msg.counter << std::endl;
					postMessage(msg, "actor2");
				});
		actor->startDispatcher(); //Event loop
	};

	auto threadActor2 = [](){
		auto actor = makeActor("actor2",
				[&](const std::shared_ptr<MessageData> &data) {
					std::this_thread::sleep_for(std::chrono::seconds(1));
					auto msg = getMessage<Counter>(data);
					std::cout << "actor2: " << ++ msg.counter << std::endl;
					postMessage(msg, "actor1");
				});
		actor->startDispatcher(); //Event loop
	};

	std::thread thread1(threadActor1);
	std::thread thread2(threadActor2);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	std::cout << "Posting start message." << std::endl;
	antifreeze::helper::postMessage(Counter(), "actor1");

	std::this_thread::sleep_for(std::chrono::seconds(10));

	std::cout << "Posting stop message." << std::endl;
	StoppingHandler::postStoppingMessage(); //Stop all event loops

	thread1.join();
	thread2.join();

	return 0;
}
