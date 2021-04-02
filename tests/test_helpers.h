#pragma once

#include <csm.h>
#include <catch/catch.hpp>

namespace csm::test{

enum class TestState{ _1, _2, _3, _4 };

struct Event{ int data; };
struct Event1 : Event{};
struct Event2 : Event{};
struct Event3 : Event{};

struct Action
{
    template<class Object, class Event>
    void operator()(Object&, const Event&) const noexcept{}
};

struct Action1 : Action{};
struct Action2 : Action{};

template<bool V>
struct Return
{
    template<class Object>
    bool operator()(const Object&) const noexcept
    {
        return V;
    }
};

struct DummyObject{};

template<class Pred>
using Not = csm::detail::Not<Pred>;

template<class... Preds>
using All = csm::detail::All<Preds...>;

template<class... Preds>
using Any = csm::detail::Any<Preds...>;

template<class... Preds>
using None = csm::detail::None<Preds...>;

struct HasEnter
{
    template<TestState, class Object, class E>
    void OnEnter(Object&, const E&){}
};

struct HasLeave
{
    template<TestState, class Object, class E>
    void OnLeave(Object&, const E&){}
};

template<bool V>
struct WithEvent
{
    template<class E>
    static constexpr bool ContainsEvent{ V };
};

struct StatesBase : csm::SyntaxDefinitions<TestState>
{
    struct State1 : State<TestState::_1>{};
    struct State2 : State<TestState::_2>{};
    struct State3 : State<TestState::_3>{};
    struct State4 : State<TestState::_4>{};
};

template<class T>
using TestStateMachine = csm::StateMachine<T, TestState, csm::tags::NoSyntaxDefinitions>;

struct TransitionsSingle : StatesBase, TestStateMachine<TransitionsSingle>
{
    using TestStateMachine<TransitionsSingle>::StateMachine;

    static constexpr auto TransitionRules{ MakeTransitionRules(
        From<State1> && On<Event1> = To<State2>
    )};
};

struct TransitionsMultiple : StatesBase, TestStateMachine<TransitionsMultiple>
{
    using TestStateMachine<TransitionsMultiple>::StateMachine;

    static constexpr auto TransitionRules{ MakeTransitionRules(
        From<State1, State2> && On<Event1, Event2> = To<State3>
    )};
};

struct TransitionsOr : StatesBase, TestStateMachine<TransitionsOr>
{
    using TestStateMachine<TransitionsOr>::StateMachine;

    static constexpr auto TransitionRules{ MakeTransitionRules(
        (From<State1> && On<Event1>) ||
        (From<State2> && On<Event2>) ||
        (From<State1> && On<Event3>)
            = To<State3>
    )};
};

struct TransitionsSeveral : StatesBase, TestStateMachine<TransitionsSeveral>
{
    using TestStateMachine<TransitionsSeveral>::StateMachine;

    static constexpr auto TransitionRules{ MakeTransitionRules(
        From<State1> && On<Event1> = To<State3>,
        From<State3> && On<Event2> = To<State1>
    )};
};

struct TransitionsConditions : StatesBase, TestStateMachine<TransitionsConditions>
{
    using TestStateMachine<TransitionsConditions>::StateMachine;

    static constexpr auto TransitionRules{ MakeTransitionRules(
        (From<State1> && On<Event1> && If<Return<false>>) ||
        (From<State1> && On<Event2>)
            = To<State3>,
        (From<State2> && On<Event1>) ||
        (From<State3> && On<Event2> && If<Return<true>>)
            = To<State1>,
        (From<State1> && On<Event1> && If<Return<true>>) ||
        (From<State3> && On<Event3> && If<Return<false>>)
            = To<State2>
    )};
};

template<class Object>
using MakeTransitionsPack = csm::detail::MakeTransitionsT<std::decay_t<decltype(Object::TransitionRules)>>;

struct TransitionsCallbackSequenceCheck :
        csm::StateMachine<TransitionsCallbackSequenceCheck, TestState>
{
    TransitionsCallbackSequenceCheck()
        : StateMachine<TransitionsCallbackSequenceCheck, TestState>(TestState::_1)
    {}

    struct State1 : State<TestState::_1>
    {
        template<TestState From, class Event>
        void OnEnter(TransitionsCallbackSequenceCheck&, const Event&)
        {
            REQUIRE(false);
        }

        template<TestState To>
        void OnLeave(TransitionsCallbackSequenceCheck& obj, const Event1&)
        {
            REQUIRE((!obj.enterCalled && !obj.leaveCalled));
            obj.leaveCalled = true;
            REQUIRE(To == TestState::_2);
            REQUIRE(obj.GetState() == TestState::_1);
        }
    };

    struct State2 : State<TestState::_2>
    {
        template<TestState From>
        void OnEnter(TransitionsCallbackSequenceCheck& obj, const Event1&)
        {
            REQUIRE((!obj.enterCalled && obj.leaveCalled));
            obj.enterCalled = true;
            REQUIRE(From == TestState::_1);
            REQUIRE(obj.GetState() == TestState::_2);
        }

        template<TestState To, class Event>
        void OnLeave(TransitionsCallbackSequenceCheck&, const Event&)
        {
            REQUIRE(false);
        }
    };

    static constexpr auto TransitionRules{ MakeTransitionRules(
        From<State1> && On<Event1> = To<State2>
    )};

    bool leaveCalled{ false };
    bool enterCalled{ false };
};

struct TransitionsCallbacks :
        csm::StateMachine<TransitionsCallbacks, TestState>
{
    using StateMachine<TransitionsCallbacks, TestState>::StateMachine;

    struct WithEnter
    {
        template<TestState To, class Event>
        void OnEnter(TransitionsCallbacks& obj, const Event& e)
        {
            obj.enterData.push_back(e.data);
        }
    };

    struct WithLeave
    {
        template<TestState From, class Event>
        void OnLeave(TransitionsCallbacks& obj, const Event& e)
        {
            obj.leaveData.push_back(e.data);
        }
    };

    struct State1 : State<TestState::_1>, WithEnter{};
    struct State2 : State<TestState::_2>, WithLeave{};
    struct State3 : State<TestState::_3>, WithEnter, WithLeave{};
    struct State4 : State<TestState::_4>{};

    static constexpr auto TransitionRules{ MakeTransitionRules(
        From<State1> && On<Event1> = To<State2>,
        From<State1> && On<Event2> = To<State4>,
        From<State2> && On<Event1> = To<State3>,
        From<State3> && On<Event1> = To<State1>,
        From<State4> && On<Event1> = To<State1>
    )};

    std::vector<int> enterData;
    std::vector<int> leaveData;
};

template<class StateMachine>
class SequentialTransitionChecker
{
public:
    explicit SequentialTransitionChecker(TestState startState) noexcept
        : m_sm(startState)
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
        m_sm = StateMachine{ state };
        return *this;
    }

private:
    StateMachine m_sm;
    const TestState m_startState;
};

struct ActionRulesBase : StatesBase
{
    static constexpr auto TransitionRules{ MakeTransitionRules(
        From<State1> && On<Event1> = To<State2>
    )};
};

struct ActionRulesSingle : ActionRulesBase, TestStateMachine<ActionRulesSingle>
{
    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1> = Do<Action1>
    )};
};

struct ActionRulesMultiple : ActionRulesBase, TestStateMachine<ActionRulesMultiple>
{
    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1, Event2> = Do<Action1, Action2>
    )};
};

struct ActionRulesOrAndCondition : ActionRulesBase, TestStateMachine<ActionRulesOrAndCondition>
{   
    static constexpr auto ActionRules{ csm::MakeActionRules(
        (On<Event1, Event2> && If<Return<true>>) ||
        On<Event3>
            = Do<Action1, Action2>,
        On<Event1, Event2> ||
        (On<Event3> && If<Return<true>>)
            = Do<Action1, Action2>,
        (On<Event1> && If<Return<true>>) ||
        (On<Event2> && If<Return<true>>) ||
        (On<Event3> && If<Return<true>>)
            = Do<Action1, Action2>
    )};
};

template<class Object>
using MakeActionRules = std::decay_t<decltype(Object::ActionRules)>;

struct ActionsBase : ActionRulesBase
{    
    struct Add
    {
        template<class Event>
        void operator()(ActionsBase& obj, const Event& e) const noexcept
        {
            obj.data += e.data;
        }
    };

    struct Sub
    {
        template<class Event>
        void operator()(ActionsBase& obj, const Event& e) const noexcept
        {
            obj.data -= e.data;
        }
    };

    struct Mult
    {
        template<class Event>
        void operator()(ActionsBase& obj, const Event& e) const noexcept
        {
            obj.data *= e.data;
        }
    };

    int data{ 0 };
};

struct ActionsSingle : ActionsBase, TestStateMachine<ActionsSingle>
{
    using TestStateMachine<ActionsSingle>::StateMachine;

    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1> = Do<Add>
    )};
};

struct ActionsMultiple : ActionsBase, TestStateMachine<ActionsMultiple>
{
    using TestStateMachine<ActionsMultiple>::StateMachine;

    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Event1, Event2> = Do<Add, Mult>
    )};
};

struct ActionsConditions : ActionsBase, TestStateMachine<ActionsConditions>
{
    using TestStateMachine<ActionsConditions>::StateMachine;

    static constexpr auto ActionRules{ csm::MakeActionRules(
        (On<Event1> && If<Return<false>>) ||
        On<Event2>
            = Do<Add, Mult>,
        On<Event1> ||
        (On<Event3> && If<Return<true>>)
            = Do<Sub>
    )};
};

}// csm::test
