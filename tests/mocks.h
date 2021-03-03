#ifndef CSM_TEST_MOCKS_H
#define CSM_TEST_MOCKS_H

#include "transition_table.h"
#include "detail/transition_checker.h"

namespace csm::test{

struct MockHandlerAcceptAll : HandlerMock<MockHandlerAcceptAll, State, Event>
{
    MOCK_ON_TRANSITION(From, To)
    MOCK_ON_ENTER_STATE(From)
    MOCK_ON_LEAVE_STATE(To)
    MOCK_ON_EVENT(Trigger);

    RequiredCalls GetRequiredCallsImpl(
            Event /*e*/,
            std::optional<Tr<State>> transition) const noexcept
    {
        RequiredCalls calls{ EventCall };
        if (transition.has_value())
        {
            calls |= TransitionCall;
            if (transition->IsStateChanged())
            {
                calls |= LeaveCall | EnterCall;
            }
        }

        return calls;
    }

    bool IsAllowedEventImpl(Event /*e*/) const noexcept
    {
        return true;
    }

    bool IsAllowedTransitionImpl(Tr<State> /*transition*/) const noexcept
    {
        return true;
    }
};

struct MockHandlerSelective : HandlerMock<MockHandlerSelective, State, Event>
{
    static constexpr State HandledFrom{ State::_0 };
    static constexpr State HandledTo{ State::_2 };
    static constexpr Event HandledEvent{ Event::_0 };

    MOCK_ON_TRANSITION_COND(From, To, From == HandledFrom && To == HandledTo)
    MOCK_ON_ENTER_STATE_COND(To, To == HandledTo)
    MOCK_ON_LEAVE_STATE_COND(From, From == HandledFrom)
    MOCK_ON_EVENT_COND(Trigger, Trigger == HandledEvent);

    RequiredCalls GetRequiredCallsImpl(
            Event e,
            std::optional<Tr<State>> transition) const noexcept
    {
        RequiredCalls calls{ e == HandledEvent? EventCall : NoCalls };
        if (transition.has_value())
        {
            if (transition->from == HandledFrom && transition->IsStateChanged())
            {
                calls |= LeaveCall;
            }

            if (transition->to == HandledTo && transition->IsStateChanged())
            {
                calls |= EnterCall;
            }

            if (transition->from == HandledFrom && transition->to == HandledTo)
            {
                calls |= TransitionCall;
            }
        }

        return calls;
    }

    bool IsAllowedEventImpl(Event /*e*/) const noexcept
    {
        return true;
    }

    bool IsAllowedTransitionImpl(Tr<State> /*transition*/) const noexcept
    {
        return true;
    }
};

struct MockHandlerAllowedConditions : HandlerMock<MockHandlerAllowedConditions, State, Event>
{
    static constexpr State AllowedFrom{ State::_0 };
    static constexpr State AllowedTo{ State::_2 };
    static constexpr Event AllowedEvent{ Event::_0 };

    MOCK_ON_TRANSITION(From, To)
    MOCK_ON_ENTER_STATE(From)
    MOCK_ON_LEAVE_STATE(To)
    MOCK_ON_EVENT(Trigger);
    MOCK_ALLOW_EVENT(Trigger);
    MOCK_ALLOW_TRANSITION(From, To);

    RequiredCalls GetRequiredCallsImpl(
        Event e,
        std::optional<Tr<State>> transition) const noexcept
    {
        RequiredCalls calls{ EventAllowedCall };
        if (IsAllowedEventImpl(e))
        {
            calls |= EventCall | TransitionAllowedCall;
            if (transition.has_value() && IsAllowedTransitionImpl(*transition))
            {
                calls |= TransitionCall;
                if (transition->IsStateChanged())
                {
                    calls |= LeaveCall | EnterCall;
                }
            }
        }

        return calls;
    }

    bool IsAllowedEventImpl(Event e) const noexcept
    {
        return e == AllowedEvent;
    }

    bool IsAllowedTransitionImpl(Tr<State> transition) const noexcept
    {
        return transition.from == AllowedFrom && transition.to == AllowedTo;
    }
};

}// csm::test

#endif //CSM_TEST_MOCKS_H
