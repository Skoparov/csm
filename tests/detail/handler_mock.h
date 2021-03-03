#ifndef CSM_TEST_HANDLER_MOCK_H
#define CSM_TEST_HANDLER_MOCK_H

#include "required_calls.h"
#include <catch.hpp>
#include <optional>

namespace csm::test{
namespace detail {

template<class... T>
void Clear(T&&... ts) noexcept
{
    ((ts.reset()), ...);
}

template<bool... C>
struct Eval
{
    static constexpr bool Value{
        sizeof... (C) != 0? ((C == true) && ...) : true };
};

template<bool... C>
constexpr bool EvalV{ Eval<C...>::Value };

}// detail

template<class State>
struct Tr
{
    Tr(State from, State to) noexcept // for CTAD
        : from(from)
        , to(to) {}

    bool IsStateChanged() const noexcept
    {
        return from != to;
    }

    bool operator==(Tr o) const noexcept
    {
        return from == o.from && to == o.to;
    }

    State from;
    State to;
};

template<class Handler, class State, class Event>
class HandlerMock
{   
    template<class T>
    struct Allowed
    {
        T value;
        bool allowed;
    };

    static_assert(std::is_enum_v<State>);
    static_assert(std::is_enum_v<Event>);

public:
    void ExpectTransitionCall(Tr<State> transition)
    {
        REQUIRE(!m_expectedTransitionCall.has_value());
        m_expectedTransitionCall = transition;
    }

    void ExpectEnterCall(State state)
    {
        REQUIRE(!m_expectedEnterCall.has_value());
        m_expectedEnterCall = state;
    }

    void ExpectLeaveCall(State state)
    {
        REQUIRE(!m_expectedLeaveCall.has_value());
        m_expectedLeaveCall = state;
    }

    void ExpectEventCall(Event event, int eventData)
    {
        REQUIRE(!m_expectedEventCall.has_value());
        m_expectedEventCall = { event, eventData };
    }

    void ExpectEventAllowedCall(Event event)
    {
        REQUIRE(!m_expectedEventAllowedCall.has_value());
        m_expectedEventAllowedCall = { event, IsAllowedEvent(event) };
    }

    void ExpectTransitionAllowedCall(Tr<State> transition)
    {
        REQUIRE(!m_expectedTransitionAllowedCall.has_value());
        m_expectedTransitionAllowedCall =
            { transition, IsAllowedTransition(transition) };
    }

    void CheckAllExpectationsSatisfied()
    {
        REQUIRE(!m_expectedEnterCall.has_value());
        REQUIRE(!m_expectedLeaveCall.has_value());
        REQUIRE(!m_expectedTransitionCall.has_value());
        REQUIRE(!m_expectedEventCall.has_value());
        REQUIRE(!m_expectedEventAllowedCall.has_value());
        REQUIRE(!m_expectedTransitionAllowedCall.has_value());

        detail::Clear(m_expectedEnterCall,
                      m_expectedLeaveCall,
                      m_expectedTransitionCall,
                      m_expectedEventCall,
                      m_expectedEventAllowedCall,
                      m_expectedTransitionAllowedCall);
    }

    RequiredCalls GetRequiredCalls(
            Event e,
            std::optional<Tr<State>> transition) const noexcept
    {
        return GetImpl().GetRequiredCallsImpl(e, transition);
    }

    bool IsAllowedEvent(Event e) const noexcept
    {
        return GetImpl().IsAllowedEventImpl(e);
    }

    bool IsAllowedTransition(Tr<State> transition) const noexcept
    {
        return GetImpl().IsAllowedTransitionImpl(transition);
    }

protected:
    void OnTransitionCall(State from, State to)
    {
        REQUIRE(m_expectedTransitionCall == Tr{ from, to });
        m_expectedTransitionCall.reset();
    }

    void OnEnterCall(State state)
    {
        REQUIRE(m_expectedEnterCall == state);
        m_expectedEnterCall.reset();
    }

    void OnLeaveCall(State state)
    {
        REQUIRE(m_expectedLeaveCall == state);
        m_expectedLeaveCall.reset();
    }

    void OnEventCall(Event event, int data)
    {
        REQUIRE(m_expectedEventCall == std::make_pair(event, data));
        m_expectedEventCall.reset();
    }

    bool OnAllowEventCall(Event event)
    {
        REQUIRE(m_expectedEventAllowedCall.has_value());
        REQUIRE(m_expectedEventAllowedCall->value == event);

        bool allowed{ m_expectedEventAllowedCall->allowed };
        m_expectedEventAllowedCall.reset();
        return allowed;
    }

    bool OnAllowTransitionCall(State from, State to)
    {
        REQUIRE(m_expectedTransitionAllowedCall.has_value());
        REQUIRE(m_expectedTransitionAllowedCall->value == Tr<State>{ from, to });

        bool allowed{ m_expectedTransitionAllowedCall->allowed };
        m_expectedTransitionAllowedCall.reset();
        return allowed;
    }

private:
    const Handler& GetImpl() const noexcept
    {
        return static_cast<const Handler&>(*this);
    }

private:
    std::optional<State> m_expectedEnterCall;
    std::optional<State> m_expectedLeaveCall;
    std::optional<Tr<State>> m_expectedTransitionCall;
    std::optional<std::pair<Event, int /*data*/>> m_expectedEventCall;
    std::optional<Allowed<Event>> m_expectedEventAllowedCall;
    std::optional<Allowed<Tr<State>>> m_expectedTransitionAllowedCall;
};

}//sm::test

#define MOCK_ON_TRANSITION_COND(From, To, Cond) \
    template<auto From, auto To, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    void OnTransition(){ OnTransitionCall(From, To); } \

#define MOCK_ON_ENTER_STATE_COND(To, Cond) \
    template<auto To, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    void OnEnterState(){ OnEnterCall(To); } \

#define MOCK_ON_LEAVE_STATE_COND(From, Cond) \
    template<auto From, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    void OnLeaveState(){ OnLeaveCall(From); } \

#define MOCK_ON_EVENT_COND(Trigger, Cond) \
    template<auto Trigger, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    void OnEvent(int data){ OnEventCall(Trigger, data); } \

#define MOCK_ALLOW_EVENT_COND(Trigger, Cond) \
    template<auto Trigger, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    bool AllowEvent(){ return OnAllowEventCall(Trigger); } \

#define MOCK_ALLOW_TRANSITION_COND(From, To, Cond) \
    template<auto From, auto To, class = std::enable_if_t<csm::test::detail::EvalV<Cond>>> \
    bool AllowTransition(){ return OnAllowTransitionCall(From, To); } \

#define MOCK_ON_TRANSITION(From, To) MOCK_ON_TRANSITION_COND(From, To, true)
#define MOCK_ON_ENTER_STATE(To) MOCK_ON_ENTER_STATE_COND(To, true)
#define MOCK_ON_LEAVE_STATE(From) MOCK_ON_LEAVE_STATE_COND(From, true)
#define MOCK_ON_EVENT(Trigger) MOCK_ON_EVENT_COND(Trigger, true)
#define MOCK_ALLOW_EVENT(Trigger) MOCK_ALLOW_EVENT_COND(Trigger, true)
#define MOCK_ALLOW_TRANSITION(From, To) MOCK_ALLOW_TRANSITION_COND(From, To, true)

#endif // CSM_TEST_HANDLER_MOCK_H
