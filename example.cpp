#include "state_machine.h"
 
enum class State{ Off, On, Updated };
enum class Event{ Start, Update, Stop };
 
struct MyTransitionsInfo : state_machine::TransitionsInfoBase<State, Event>
{
    using Table = state_machine::TransitionTable
    <
        TransitionTo<State::On, FromStates<State::Off>, OnEvents<Event::Start>>,
        TransitionTo<State::Off, FromStates<State::On, State::Updated>, OnEvents<Event::Stop>>,
        TransitionTo<State::Updated, FromStates<State::On>, OnEvents<Event::Update>>
    >;
};
 
class Handler
{
public:
	//  Other OnTransition/OnEnterState/OnLeaveState callbacks can be declared in a similar fashion if needed

    template<State From, State To, std::enable_if_t<From == State::Updated && To == State::Off>...>
    void OnTransition()
    {
        // Handle switch off from updated state
    }
 
    template<State S, std::enable_if_t<S == State::Updated>...>
    void OnEnterState()
    {
        // Handle first update
    }
 
    template<State S, std::enable_if_t<S == State::Off>...>
    void OnLeaveState()
    {
        // Handle switch on
    }
};
 
int main()
{
    Handler h;
    state_machine::StateMachine<MyTransitionsInfo, Handler> machine{ h, State::Off };
	
	// does nothing as no appropriate transitions are found
    machine.ProcessEvent<Event::Update>();
	
	// switches from Off to On, calls Handler::OnLeaveState<...> as an appropriate handler function is found	
    machine.ProcessEvent<Event::Start>(); 
	
	// switches from On to Updated, calls Handler::OnEnterState<...> as an appropriate handler function is found	
    machine.ProcessEvent<Event::Update>(); 
	
	// switched from Updated to Off, calls Handler::OnTransition<...> as an appropriate handler function is found	
    machine.ProcessEvent<Event::Stop>();
    
    return 0;
}