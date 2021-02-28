#ifndef CSM_TEST_TRANSITION_CHECKER_H
#define CSM_TEST_TRANSITION_CHECKER_H

#include "required_calls.h"
#include <csm.h>

namespace csm::test{

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
    void CheckTransitionToOnEvent(State to)
    {
        State from{ m_stateMachine.GetState() };
        RequiredCalls calls{ m_handler.GetRequiredCalls(from, to) };

        ExpectTransitionCalls(from, to, calls);
        REQUIRE_NOTHROW(m_stateMachine.template ProcessEvent<E>());
        REQUIRE(m_stateMachine.GetState() == to);

        m_handler.CheckAllExpectationsSatisfied();
    }

    template<Event E>
    void CheckNoTransitionOccured()
    {
        State state{ m_stateMachine.GetState() };
        REQUIRE_NOTHROW(m_stateMachine.template ProcessEvent<E>());
        REQUIRE(m_stateMachine.GetState() == state);

        m_handler.CheckAllExpectationsSatisfied();
    }

    void ResetStateMachine(State state) noexcept
    {
        m_stateMachine = StateMachine{ state, &m_handler };
    }

private:
    void ExpectTransitionCalls(State from, State to, RequiredCalls flags)
    {
        if (flags & RequiredCalls::Leave)
        {
            m_handler.ExpectLeaveCalls(from);
        }

        if (flags & RequiredCalls::Transition)
        {
            m_handler.ExpectTransitionCalls(from, to);
        }

        if (flags & RequiredCalls::Enter)
        {
            m_handler.ExpectEnterCalls(to);
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
        RemainingTransitions...> /* transitions */,
    Checker& checker)
{
    checker.ResetStateMachine(From);
    checker. template CheckTransitionToOnEvent<CurrEvent>(To);

    if constexpr(sizeof...(Events) != 0)
    {
        using IncTransition = csm::detail::Transition<From, To, csm::detail::ValPack<Events...>>;
        using Next = csm::detail::Pack<IncTransition, RemainingTransitions...>;
        CheckTransitions(Next{}, checker);
    }
    else if constexpr(sizeof...(RemainingTransitions) != 0)
    {
        using Next = csm::detail::Pack<RemainingTransitions...>;
        CheckTransitions(Next{}, checker);
    }
}

}// namespace csm::test

#endif // CSM_TEST_TRANSITION_CHECKER_H
