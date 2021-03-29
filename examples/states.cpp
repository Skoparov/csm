#include <csm.h>

namespace state_example{

enum class MyState{ _1, _2, _3 };

// Events
struct Event1{};
struct Event2{};
struct Event3{};

struct StateExample : csm::StateMachine<StateExample, MyState>
{
    // States
    struct State1 : State<MyState::_1>
    {
        template<MyState From, class Event>
        void OnEnter(StateExample&, const Event&) const noexcept{}

        template<MyState To, class Event>
        void OnLeave(StateExample&, const Event&) const noexcept{}
    };

    struct State2 : State<MyState::_2>
    {
        template<MyState To, class Event>
        void OnLeave(StateExample&, const Event&) const noexcept{ }
    };

    struct State3 : State<MyState::_3>{};

    // Traits
    struct AlwaysTrue
    {
        bool operator()(StateExample&) const noexcept
        {
            return true;
        }
    };

    struct IfValue
    {
        bool operator()(StateExample& impl) const noexcept
        {
            return impl.value;
        }
    };

    // Transition table
    static constexpr auto TransitionRules{ csm::MakeTransitionRules(
             From<State1> && On<Event1, Event2>
                    = To<State2>,
             (From<State2, State3> && On<Event1>) ||
             (From<State2> && On<Event2> && If<IfValue, Not<AlwaysTrue>>)
                    = To<State1>
        ) };

    const bool value{ true };
};

void Use()
{
    StateExample sm{ MyState::_1 };
    sm.ProcessEvent(Event1{}); // State 1 -> State 2
    sm.ProcessEvent(Event2{}); // State is not changed as Not<AlwaysType> is not satisfied
}

}// state_example
