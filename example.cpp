#include <csm.h>

enum class MyState{ Off, On, Updated };
enum class MyEvent{ Start, Restart, Update, UpdateRequeired, Stop };

//  Other OnTransition/OnEnterState/OnLeaveState callbacks can be declared in a similar fashion if needed
struct MyHandler
{
    template<MyState From, MyState To, std::enable_if_t<From == MyState::Updated && To == MyState::Off>...>
    void OnTransition()
    {
        // Handle switch off from updated state
    }

    template<MyState S, std::enable_if_t<S == MyState::Updated>...>
    void OnEnterState()
    {
        // Handle first update
    }

    template<MyState S, std::enable_if_t<S == MyState::Off>...>
    void OnLeaveState()
    {
        // Handle switch on
    }
};

using MyStateMachine = csm::StateMachine<
    CSM_TRANSITION_TABLE(
        (CSM_FROM(MyState::Off) + CSM_ON(MyEvent::Start, MyEvent::Restart)) ||
        (CSM_FROM(MyState::Updated) + CSM_ON(MyEvent::UpdateRequeired)) = CSM_TO(MyState::On),
        CSM_FROM(MyState::On, MyState::Updated) + CSM_ON(MyEvent::Update) = CSM_TO(MyState::Updated),
        CSM_FROM(MyState::On, MyState::Updated) + CSM_ON(MyEvent::Stop) = CSM_TO(MyState::Off)),
    MyHandler>;

int main()
{
    MyHandler h;
    MyStateMachine machine{ h, MyState::Off };

    // Does nothing as no appropriate transitions are found
    machine.ProcessEvent<MyEvent::Update>();

    // Switches from Off to On, calls Handler::OnLeaveState<...> as an appropriate handler function is found
    machine.ProcessEvent<MyEvent::Start>();

    // Switches from On to Updated, calls Handler::OnEnterState<...> as an appropriate handler function is found
    machine.ProcessEvent<MyEvent::Update>();

    // Switched from Updated to Off, calls Handler::OnTransition<...> as an appropriate handler function is found
    machine.ProcessEvent<MyEvent::Stop>();

    return 0;
}