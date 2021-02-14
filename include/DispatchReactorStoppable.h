#ifndef DISPATCHREACTORSTOPPABLE_H
#define DISPATCHREACTORSTOPPABLE_H

#include "Reactor.h"
#include "antifreeze_global.h"


namespace antifreeze
{

class StoppingHandler;

//Данный Диспатчер отличается тем, что "из коробки" включает в себя  
// 'EventHandler'(StoppingHandler), зарегистрированный на команду остановки такого Диспaтчера. 
// Таким образом, создавая Диспатчер(aka Reactor), мы автоматически создаем обработчик ожидающий команду 
//  на завершение работы Диспатчера. Остается, при необходимости, лишь послать соответсвующее сообщение. 
// * Прелесть данного Диспатчера заключается в том, что имея несколько !таких! Диспатчеров  
//     в нескольких потоках, мы можем "за_shutdown_ить" их всех одним единственным сообщением(PostMessage()). 
class ANTIFREEZESHARED_EXPORT DispatchReactorStoppable : public Reactor
{
public:
	DispatchReactorStoppable();
	~DispatchReactorStoppable() override;

	virtual void auxInit() override;
	
private:
	std::shared_ptr<StoppingHandler> m_stoppingHandlerPtr;
	
};

} // namespace antifreeze

#endif // DISPATCHREACTORSTOPPABLE_H
