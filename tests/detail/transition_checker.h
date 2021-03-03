#ifndef CSM_TEST_TRANSITION_CHECKER_H
#define CSM_TEST_TRANSITION_CHECKER_H

#include "handler_mock.h"
#include "required_calls.h"
#include <csm.h>

namespace csm::test{

constexpr int EventData{ 42 };

template<class Handler, class TransitionTable>
class TransitionChecker
{
    using StateMachine = csm::StateMachine<TransitionTable, Handler>;
    using State = typename StateMachine::State;
    using Event = typename StateMachine::Event;

public:
    explicit TransitionChecker(State startState) noexcept
        : m_stateMachine(startState, &m_handler) {}

    template<Event E>
    void CheckCallsOnEvent(int data, std::optional<State> to = {})
    {
        State from{ m_stateMachine.GetState() };
        std::optional<Tr<State>> transition;
        if (to.has_value())
        {
            transition = Tr(from, *to);
        }

        bool stateMustChange{
            m_handler.IsAllowedEvent(E) &&
            transition.has_value() &&
            m_handler.IsAllowedTransition(*transition) };

        State expectedState{ stateMustChange? *to : from };
        RequiredCalls calls{ m_handler.GetRequiredCalls(E, transition) };
        ExpectTransitionCalls(E, data, calls, transition);

        CHECK_NOTHROW(m_stateMachine.template ProcessEvent<E>(data));
        REQUIRE(m_stateMachine.GetState() == expectedState);
        m_handler.CheckAllExpectationsSatisfied();
    }

    void ResetStateMachine(State state) noexcept
    {
        m_stateMachine = StateMachine{ state, &m_handler };
    }

private:
    void ExpectTransitionCalls(
                Event event,
                int eventData,
                RequiredCalls flags,
                std::optional<Tr<State>> transition)
    {
        if (flags & LeaveCall)
        {
            m_handler.ExpectLeaveCall(transition->from);
        }

        if (flags & TransitionCall)
        {
            m_handler.ExpectTransitionCall(*transition);
        }

        if (flags & EnterCall)
        {
            m_handler.ExpectEnterCall(transition->to);
        }

        if (flags & EventCall)
        {
            m_handler.ExpectEventCall(event, eventData);
        }

        if (flags & EventAllowedCall)
        {
            m_handler.ExpectEventAllowedCall(event);
        }

        if (flags & TransitionAllowedCall)
        {
            m_handler.ExpectTransitionAllowedCall(*transition);
        }
    }

private:
    Handler m_handler;
    StateMachine m_stateMachine;
};

namespace csmd = csm::detail;

template<class Checker, class State, State From, State To,
         auto CurrEvent, auto... Events, class... RemainingTransitions>
void CheckTransitions(
    csmd::Pack<
        csmd::Transition<From, To, csmd::ValPack<CurrEvent, Events...>>,
        RemainingTransitions...>  /*transitions*/,
    Checker& checker)
{
    checker.ResetStateMachine(From);
    checker.template CheckCallsOnEvent<CurrEvent>(EventData, To);

    if constexpr(sizeof...(Events) != 0)
    {
        using Next = csmd::Pack<
            csmd::Transition<From, To, csmd::ValPack<Events...>>,
            RemainingTransitions...>;

        CheckTransitions(Next{}, checker);
    }
    else if constexpr(sizeof...(RemainingTransitions) != 0)
    {
        CheckTransitions(csmd::Pack<RemainingTransitions...>{}, checker);
    }
}

}// namespace csm::test

#endif // CSM_TEST_TRANSITION_CHECKER_H
