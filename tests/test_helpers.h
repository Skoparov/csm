#pragma once

#include <csm.h>
#include <catch/catch.hpp>

namespace csm::test{

enum class TestState{ _1, _2, _3, _4 };

struct Event{ int data; };
struct Event1 : Event{};
struct Event2 : Event{};
struct Event3 : Event{};

struct ActionBase
{
    template<class Impl, class Event>
    void operator()(Impl&, const Event&) const noexcept{}
};

struct Action1 : ActionBase{};
struct Action2 : ActionBase{};

struct DummyImpl{};

template<bool V>
struct Return
{
    template<class Impl>
    bool operator()(Impl&) const noexcept
    {
        return V;
    }
};

template<class Pred>
using Not = csm::detail::Not<DummyImpl, Pred>;

template<class... Preds>
using All = csm::detail::All<DummyImpl, Preds...>;

template<class... Preds>
using Any = csm::detail::Any<DummyImpl, Preds...>;

template<class... Preds>
using None = csm::detail::None<DummyImpl, Preds...>;

struct HasEnter
{
    template<TestState, class Impl, class E>
    void OnEnter(Impl&, const E&){}
};

struct HasLeave
{
    template<TestState, class Impl, class E>
    void OnLeave(Impl&, const E&){}
};

template<bool V>
struct WithEvent
{
    template<class E>
    static constexpr bool ContainsEvent{ V };
};

struct ImplBase : csm::StatefulObject<ImplBase>
{
    struct State1 : State<TestState::_1>{};
    struct State2 : State<TestState::_2>{};
    struct State3 : State<TestState::_3>{};
    struct State4 : State<TestState::_4>{};
};

struct TableSingle : ImplBase
{
    static constexpr auto Table{ csm::MakeTransitions(
        From<State1> && On<Event1> = To<State2>
    )};
};

struct TableMultiple : ImplBase
{
    static constexpr auto Table{ csm::MakeTransitions(
        From<State1, State2> && On<Event1, Event2> = To<State3>
    )};
};

struct TableOr : ImplBase
{
    static constexpr auto Table{ csm::MakeTransitions(
        From<State1> && On<Event1> ||
        From<State2> && On<Event2> ||
        From<State1> && On<Event3>
            = To<State3>
    )};
};

struct TableSeveral : ImplBase
{
    static constexpr auto Table{ csm::MakeTransitions(
        From<State1> && On<Event1> = To<State3>,
        From<State3> && On<Event2> = To<State1>
    )};
};

struct TableConditions : ImplBase
{
    static constexpr auto Table{ csm::MakeTransitions(
        From<State1> && On<Event1> && If<Return<false>> ||
        From<State1> && On<Event2>
            = To<State3>,
        From<State2> && On<Event1> ||
        From<State3> && On<Event2> && If<Return<true>>
            = To<State1>,
        From<State1> && On<Event1> && If<Return<true>> ||
        From<State3> && On<Event3> && If<Return<false>>
            = To<State2>
    )};
};

template<class Impl>
using MakeTable = detail::MakeTransitionTableT<Impl, std::decay_t<decltype(Impl::Table)>>;

struct TransitionCallbacks : csm::StatefulObject<TransitionCallbacks>
{
    struct WithEnter
    {
        template<TestState To, class Event>
        void OnEnter(TransitionCallbacks& impl,const Event& e)
        {
            impl.enterData.push_back(e.data);
        }
    };

    struct WithLeave
    {
        template<TestState To, class Event>
        void OnLeave(TransitionCallbacks& impl,const Event& e)
        {
            impl.leaveData.push_back(e.data);
        }
    };

    struct State1 : State<TestState::_1>, WithEnter{};
    struct State2 : State<TestState::_2>, WithLeave{};
    struct State3 : State<TestState::_3>, WithEnter, WithLeave{};
    struct State4 : State<TestState::_4>{};

    static constexpr auto Table{ csm::MakeTransitions(
        From<State1> && On<Event1> = To<State2>,
        From<State1> && On<Event2> = To<State4>,
        From<State2> && On<Event1> = To<State3>,
        From<State3> && On<Event1> = To<State1>,
        From<State4> && On<Event1> = To<State1>
    )};

    std::vector<int> enterData;
    std::vector<int> leaveData;
};

template<class Impl>
class SequentialTransitionChecker
{
public:
    explicit SequentialTransitionChecker(TestState startState) noexcept
        : m_sm(startState, m_impl)
        , m_startState(startState)
    {
        REQUIRE(m_sm.GetState() == m_startState);
    }

    template<class Event>
    SequentialTransitionChecker& CheckChangedOn(TestState to)
    {
        REQUIRE_NOTHROW(m_sm.ProcessEvent(Event{}));
        REQUIRE(m_sm.GetState() == to);
        return *this;
    }

    template<class... Events>
    SequentialTransitionChecker& CheckNotChangedOn()
    {
        TestState state{ m_sm.GetState() };
        auto check{ [this, state](const auto& event)
        {
            REQUIRE_NOTHROW(m_sm.ProcessEvent(event));
            REQUIRE(m_sm.GetState() == state);
        }};

        (check(Events{}), ...);
        return *this;
    }

    SequentialTransitionChecker& Reset() noexcept
    {
        return SetState(m_startState);
    }

    SequentialTransitionChecker& SetState(TestState state) noexcept
    {
        m_sm = StateMachine<Impl>(state, m_impl);
        return *this;
    }

private:
    Impl m_impl;
    StateMachine<Impl> m_sm;
    const TestState m_startState;
};

struct ActionRulesSingle : ImplBase
{
    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1> = Do<Action1>
    )};
};

struct ActionRulesMultiple : ImplBase
{
    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1, Event2> = Do<Action1, Action2>
    )};
};

struct ActionRulesOrAndCondition : ImplBase
{
    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1, Event2> && If<Return<true>> ||
        On<Event3>
            = Do<Action1, Action2>,
        On<Event1, Event2> ||
        On<Event3> && If<Return<true>>
            = Do<Action1, Action2>,
        On<Event1> && If<Return<true>> ||
        On<Event2> && If<Return<true>> ||
        On<Event3> && If<Return<true>>
            = Do<Action1, Action2>
    )};
};

template<class Impl>
using MakeRules = std::decay_t<decltype(Impl::ActionRules)>;

struct ActionsImplBase : csm::StatefulObject<ActionsImplBase>
{
    struct State1 : State<TestState::_1>{};
    struct State2 : State<TestState::_2>{};

    struct Add
    {
        template<class Event>
        void operator()(ActionsImplBase& impl, const Event& e) const noexcept
        {
            impl.data += e.data;
        }
    };

    struct Sub
    {
        template<class Event>
        void operator()(ActionsImplBase& impl, const Event& e) const noexcept
        {
            impl.data -= e.data;
        }
    };

    struct Mult
    {
        template<class Event>
        void operator()(ActionsImplBase& impl, const Event& e) const noexcept
        {
            impl.data *= e.data;
        }
    };

    static constexpr auto Table{ csm::MakeTransitions(
        From<State1> && On<Event1> = To<State2>
    )};

    int data{ 0 };
};

struct ActionsSingle : ActionsImplBase
{
    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1> = Do<Add>
    )};
};

struct ActionsMultiple : ActionsImplBase
{
    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1, Event2> = Do<Add, Mult>
    )};
};

struct ActionsConditions : ActionsImplBase
{
    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1> && If<Return<false>> ||
        On<Event2>
            = Do<Add, Mult>,
        On<Event1> ||
        On<Event3> && If<Return<true>>
            = Do<Sub>
    )};
};

}// csm::test
