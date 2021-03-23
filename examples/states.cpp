#include <csm.h>

enum class MyState{ _1, _2, _3 };

// Events
struct Event1{};
struct Event2{};
struct Event3{};

struct Impl : csm::StatefulObject<Impl>
{
    // States
    struct State1 : State<MyState::_1>
    {
        template<MyState From, class Event>
        void OnEnter(Impl& /* impl */, const Event& /* e */) const noexcept{}

        template<MyState To, class Event>
        void OnLeave(Impl& /* impl */, const Event& /* e */) const noexcept{}
    };

    struct State2 : State<MyState::_2>
    {
        template<MyState To, class Event>
        void OnLeave(Impl& /* impl */, const Event& /* e */) const noexcept{ }
    };

    struct State3 : State<MyState::_3>{};

    // Traits
    struct AlwaysTrue
    {
        bool operator()(Impl& /* impl */) const noexcept
        {
            return true;
        }
    };

    struct IfValue
    {
        bool operator()(Impl& impl) const noexcept
        {
            return impl.value;
        }
    };

    // Transition table
    static constexpr auto Table{ csm::MakeTransitions(
             From<State1> && On<Event1, Event2>
                    = To<State2>,
             (From<State2, State3> && On<Event1>) ||
             (From<State2> && On<Event2> && IfAll<IfValue, Not<AlwaysTrue>>)
                    = To<State1>
        ) };

    const bool value{ true };
};

int main()
{
    Impl impl;
    csm::StateMachine<Impl> sm{ MyState::_1, impl };
	
    sm.ProcessEvent(Event1{}); // State 1 -> State 2
    sm.ProcessEvent(Event2{}); // State is not changed as Not<AlwaysType> is not satisfied

    return 0;
}
