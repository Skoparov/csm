#ifndef CSM_TEST_HANDLER_MOCK_H
#define CSM_TEST_HANDLER_MOCK_H

#include "required_calls.h"
#include <catch.hpp>
#include <set>

namespace csm::test{

template<class Handler, class State, class Event>
class HandlerMock
{
    static_assert(std::is_enum_v<State>);
    static_assert(std::is_enum_v<Event>);

public:
    void ExpectTransitionCalls(State from, State to)
    {
        REQUIRE(m_expectedTransitionCalls.insert(std::make_pair(from, to)).second);
    }

    void ExpectEnterCalls(State state)
    {
        REQUIRE(m_expectedEnterCalls.insert(state).second);
    }

    void ExpectLeaveCalls(State state)
    {
        REQUIRE(m_expectedLeaveCalls.insert(state).second);
    }

    void CheckAllExpectationsSatisfied() const
    {
        REQUIRE(m_expectedEnterCalls.empty());
        REQUIRE(m_expectedLeaveCalls.empty());
        REQUIRE(m_expectedTransitionCalls.empty());
    }

    RequiredCalls GetRequiredCalls(State from, State to) const noexcept
    {
        return static_cast<const Handler&>(*this).GetRequiredCallsImpl(from, to);
    }

protected:
    void _OnTransitionCall(State from, State to)
    {
        REQUIRE(m_expectedTransitionCalls.erase(
                    std::make_pair(from, to)) == 1);
    }

    void _OnEnterCall(State state)
    {
        REQUIRE(m_expectedEnterCalls.erase(state) == 1);
    }

    void _OnLeaveCall(State state)
    {
        REQUIRE(m_expectedLeaveCalls.erase(state) == 1);
    }

private:
    std::set<State> m_expectedEnterCalls;
    std::set<State> m_expectedLeaveCalls;
    std::set<std::pair<State, State>> m_expectedTransitionCalls;
};

namespace detail {

template<bool... C>
struct Eval
{
    static constexpr bool Value{
        sizeof... (C) != 0? ((C == true) && ...) : true };
};

template<bool... C>
constexpr bool EvalV{ Eval<C...>::Value };

}// detail
}//sm::test

#define MOCK_ON_TRANSITION_COND(From, To, Cond) \
    template<auto From, auto To, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    void OnTransition(){ _OnTransitionCall(From, To); } \

#define MOCK_ON_ENTER_STATE_COND(To, Cond) \
    template<auto To, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    void OnEnterState(){ _OnEnterCall(To); } \

#define MOCK_ON_LEAVE_STATE_COND(From, Cond) \
    template<auto From, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    void OnLeaveState(){ _OnLeaveCall(From); } \

#define MOCK_ON_TRANSITION(From, To) MOCK_ON_TRANSITION_COND(From, To, true)
#define MOCK_ON_ENTER_STATE(To) MOCK_ON_ENTER_STATE_COND(To, true)
#define MOCK_ON_LEAVE_STATE(From) MOCK_ON_LEAVE_STATE_COND(From, true)

#endif // CSM_TEST_HANDLER_MOCK_H
