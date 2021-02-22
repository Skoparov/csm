#include "state_machine.h"
 
enum class MyState{ Off, On, Updated };
enum class MyEvent{ Start, Restart, Update, UpdateRequeired, Stop };

class MyHandler
{
public:
    //  Other OnTransition/OnEnterState/OnLeaveState callbacks can be declared in a similar fashion if needed

    template<MyState From, MyState To, std::enable_if_t<From == MyState::Updated && To == MyState::Off>...>
    void OnTransition()
    {
        int i = 0; ++i;
        // Handle switch off from updated state
    }

    template<MyState S, std::enable_if_t<S == MyState::Updated>...>
    void OnEnterState()
    {
        int i = 0; ++i;
        // Handle first update
    }

    template<MyState S, std::enable_if_t<S == MyState::Off>...>
    void OnLeaveState()
    {
        int i = 0; ++i;
        // Handle switch on
    }
};

using MyStateMachine = cpsm::StateMachine<
    cpsm::TransitionTable
    <
        cpsm::To<MyState::On,
            cpsm::If<cpsm::From<MyState::Off>, cpsm::On<MyEvent::Start, MyEvent::Restart>>,
            cpsm::If<cpsm::From<MyState::Updated>, cpsm::On<MyEvent::UpdateRequeired>>>,
        cpsm::To<MyState::Updated,
            cpsm::If<cpsm::From<MyState::On, MyState::Updated>, cpsm::On<MyEvent::Update>>>,
        cpsm::To<MyState::Off,
            cpsm::If<cpsm::From<MyState::On, MyState::Updated>, cpsm::On<MyEvent::Stop>>>
    >,
    MyHandler>;

int main()
{
    MyHandler h;
    MyStateMachine machine{ h, MyState::Off };

    // does nothing as no appropriate transitions are found
    machine.ProcessEvent<MyEvent::Update>();

    // switches from Off to On, calls Handler::OnLeaveState<...> as an appropriate handler function is found
    machine.ProcessEvent<MyEvent::Start>();

    // switches from On to Updated, calls Handler::OnEnterState<...> as an appropriate handler function is found
    machine.ProcessEvent<MyEvent::Update>();

    // switched from Updated to Off, calls Handler::OnTransition<...> as an appropriate handler function is found
    machine.ProcessEvent<MyEvent::Stop>();

    return 0;
}
