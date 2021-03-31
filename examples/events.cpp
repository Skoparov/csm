#include <csm.h>
#include <iostream>

namespace events_example {

enum class MyState{ _1, _2 };

// Events
struct ValueAdd{ int change; };
struct ValueAdd2{ int change; };
struct ValueReset{};
struct EndOfCalc{};

struct EventsExample : csm::StateMachine<EventsExample, MyState>
{
    // States
    struct State1 : State<MyState::_1>{};
    struct State2 : State<MyState::_2>{};

    // Traits
    template<int Val>
    struct ValueIs
    {
        bool operator()(const EventsExample& impl) const noexcept
        {
            return impl.value == Val;
        }
    };

    // Actions
    struct ChangeValue
    {
        template<class Event>
        void operator()(EventsExample& impl, const Event& e) const noexcept
        {
            impl.value += e.change;
        }
    };

    struct PrintValue
    {
        template<class Event>
        void operator()(EventsExample& impl, const Event&) const
        {
            std::cout << impl.value << '\n';
        }
    };

    struct ResetValue
    {
        template<class Event>
        void operator()(EventsExample& impl, const Event&) const noexcept
        {
            impl.value = 0;
        }
    };

    // Transition table
    static constexpr auto TransitionRules{ csm::MakeTransitionRules(
             From<State1> && On<ResetValue> = To<State2> // just some random stuff
        ) };

    static constexpr auto ActionRules{ csm::MakeActionRules(
        (On<ValueAdd> && IfNone<ValueIs<5>, ValueIs<10>>) ||
        (On<ValueAdd2> && If<ValueIs<5>>)
            = Do<ChangeValue, PrintValue>,
        On<ValueReset, EndOfCalc>
            = Do<PrintValue, ResetValue>
        ) };

    int value{ 0 };
};

void Use()
{
    EventsExample sm{ MyState::_1 };
    sm.ProcessEvent(ValueAdd{ 5 }); // sets value to 5, prints 5
    sm.ProcessEvent(ValueAdd{ 5 }); // event ignored due as !ValueIs<5> condition is not satisfied
    sm.ProcessEvent(ValueAdd2{ 5 }); // sets value to 10, prints 10
    sm.ProcessEvent(ValueReset{}); // prints 10, sets value = 0
}

}// events_example
