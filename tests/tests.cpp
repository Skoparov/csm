#define CATCH_CONFIG_MAIN

#include "test_helpers.h"

namespace csm::test{

TEST_CASE("Traits check", "[Details]")
{
    using namespace detail;
    static_assert(HasOnEnterV<HasEnter, TestState::_1, DummyObject, Event1>);
    static_assert(!HasOnLeaveV<HasEnter, TestState::_1, DummyObject, Event1>);
    static_assert(!HasOnEnterV<HasLeave, TestState::_1, DummyObject, Event1>);
    static_assert(HasOnLeaveV<HasLeave, TestState::_1, DummyObject, Event1>);

    static_assert(!HasDups<int>::value);
    static_assert(!HasDups<int, double>::value);
    static_assert(HasDups<int, int>::value);

    static_assert(std::is_same_v<FilterByEvent<Event1>, Pack<>>);
    static_assert(std::is_same_v<
            FilterByEvent<Event1,WithEvent<true>, WithEvent<false>>,
            Pack<WithEvent<true>>>);

    static_assert(std::is_same_v<
            FilterByEvent<Event1, WithEvent<false>, WithEvent<false>>,
            Pack<>>);
}

TEST_CASE("Predicates check", "[Details]" )
{
    using namespace detail;
    DummyObject object{};

    SECTION("Not")
    {
        REQUIRE(Not<Return<true>>{}(object) == false);
        REQUIRE(Not<Return<false>>{}(object) == true);
        REQUIRE(Not<Not<Return<false>>>{}(object) == false);
    }

    SECTION("All")
    {
        REQUIRE(All<Return<true>>{}(object) == true);
        REQUIRE(All<Return<true>, Not<Return<false>>>{}(object) == true);
        REQUIRE(All<Return<false>>{}(object) == false);
        REQUIRE(All<Return<true>, Return<false>>{}(object) == false);
        REQUIRE(All<Return<false>, Not<Return<true>>>{}(object) == false);
    }

    SECTION("Any")
    {
        REQUIRE(Any<Return<true>>{}(object) == true);
        REQUIRE(Any<Return<true>, Not<Return<false>>>{}(object) == true);
        REQUIRE(Any<Return<false>>{}(object) == false);
        REQUIRE(Any<Return<true>, Return<false>>{}(object) == true);
        REQUIRE(Any<Return<false>, Not<Return<true>>>{}(object) == false);
    }

    SECTION("None")
    {
        REQUIRE(None<Return<true>>{}(object) == false);
        REQUIRE(None<Return<true>, Not<Return<false>>>{}(object) == false);
        REQUIRE(None<Return<false>>{}(object) == true);
        REQUIRE(None<Return<true>, Return<false>>{}(object) == false);
        REQUIRE(None<Return<false>, Not<Return<true>>>{}(object) == true);
    }
}

TEST_CASE("Transitions generation", "[Details]" )
{
    using namespace detail;
    static_assert(std::is_same_v<
        MakeTransitionsPack<TransitionsSingle>,
        Pack<Transition<StatesBase::State1, StatesBase::State2, Pack<Event1>, Dummy>>>);

    static_assert(std::is_same_v<
        MakeTransitionsPack<TransitionsMultiple>,
        Pack<
            Transition<TransitionsMultiple::State1, TransitionsMultiple::State3, Pack<Event1, Event2>, Dummy>,
            Transition<TransitionsMultiple::State2, TransitionsMultiple::State3, Pack<Event1, Event2>, Dummy>>>);

    static_assert(std::is_same_v<
        MakeTransitionsPack<TransitionsOr>,
        Pack<
            Transition<TransitionsOr::State1, TransitionsOr::State3, Pack<Event1>, Dummy>,
            Transition<TransitionsOr::State2, TransitionsOr::State3, Pack<Event2>, Dummy>,
            Transition<TransitionsOr::State1, TransitionsOr::State3, Pack<Event3>, Dummy>>>);

    static_assert(std::is_same_v<
        MakeTransitionsPack<TransitionsSeveral>,
        Pack<
            Transition<TransitionsSeveral::State1, TransitionsSeveral::State3, Pack<Event1>, Dummy>,
            Transition<TransitionsSeveral::State3, TransitionsSeveral::State1, Pack<Event2>, Dummy>>>);

    static_assert(std::is_same_v<
        MakeTransitionsPack<TransitionsConditions>,
        Pack<
            Transition<StatesBase::State1, StatesBase::State3, Pack<Event1>, If<Return<false>>>,
            Transition<StatesBase::State1, StatesBase::State3, Pack<Event2>, Dummy>,
            Transition<StatesBase::State2, StatesBase::State1, Pack<Event1>, Dummy>,
            Transition<StatesBase::State3, StatesBase::State1, Pack<Event2>, If<Return<true>>>,
            Transition<StatesBase::State1, StatesBase::State2, Pack<Event1>, If<Return<true>>>,
            Transition<StatesBase::State3, StatesBase::State2, Pack<Event3>, If<Return<false>>>>>);
}

TEST_CASE("Action rules generation", "[Details]" )
{
    using namespace detail;
    static_assert(std::is_same_v<
        MakeActionRules<ActionRulesSingle>,
        Pack<ActRulePack<Do<Action1>, ActRule<Pack<Event1>, Dummy>>>>);

    static_assert(std::is_same_v<
        MakeActionRules<ActionRulesMultiple>,
        Pack<ActRulePack<Do<Action1, Action2>, ActRule<Pack<Event1, Event2>, Dummy>>>>);

    static_assert(std::is_same_v<
        MakeActionRules<ActionRulesOrAndCondition>,
        Pack<
            ActRulePack<Do<Action1, Action2>,
                ActRule<Pack<Event3>, Dummy>,
                ActRule<Pack<Event1, Event2>, If<Return<true>>>>,
            ActRulePack<Do<Action1, Action2>,
                ActRule<Pack<Event1, Event2>, Dummy>,
                ActRule<Pack<Event3>, If<Return<true>>>>,
            ActRulePack<Do<Action1, Action2>,
                ActRule<Pack<Event1>, If<Return<true>>>,
                ActRule<Pack<Event2>, If<Return<true>>>,
                ActRule<Pack<Event3>, If<Return<true>>>>>>);
}

TEST_CASE("Check state transitions", "[StateMachine]" )
{
    SECTION("Single")
    {
        SequentialTransitionChecker<TransitionsSingle> checker{ TestState::_1 };
        checker.CheckNotChangedOn<Event2, Event3>();
        checker.CheckChangedOn<Event1>(TestState::_2);
        checker.CheckNotChangedOn<Event1, Event2, Event3>();
    }

    SECTION("Multiple")
    {
        SequentialTransitionChecker<TransitionsMultiple> checker{ TestState::_1 };
        checker.CheckNotChangedOn<Event3>();
        checker.CheckChangedOn<Event1>(TestState::_3);
        checker.CheckNotChangedOn<Event1, Event2, Event3>();
        checker.SetState(TestState::_1);
        checker.CheckChangedOn<Event2>(TestState::_3);
        checker.SetState(TestState::_2);
        checker.CheckNotChangedOn<Event3>();
        checker.CheckChangedOn<Event1>(TestState::_3);
        checker.SetState(TestState::_2);
        checker.CheckChangedOn<Event2>(TestState::_3);
    }

    SECTION("Or")
    {
        SequentialTransitionChecker<TransitionsOr> checker{ TestState::_1 };
        checker.CheckNotChangedOn<Event2>();
        checker.CheckChangedOn<Event1>(TestState::_3);
        checker.CheckNotChangedOn<Event1, Event2, Event3>();
        checker.SetState(TestState::_1);
        checker.CheckChangedOn<Event3>(TestState::_3);
        checker.SetState(TestState::_2);
        checker.CheckNotChangedOn<Event1, Event3>();
    }

    SECTION("Several")
    {
        SequentialTransitionChecker<TransitionsSeveral> checker{ TestState::_1 };
        checker.CheckNotChangedOn<Event2, Event3>();
        checker.CheckChangedOn<Event1>(TestState::_3);
        checker.CheckNotChangedOn<Event1, Event3>();
        checker.CheckChangedOn<Event2>(TestState::_1);
    }

    SECTION("TransitionsConditions")
    {
        SequentialTransitionChecker<TransitionsConditions> checker{ TestState::_1 };
        checker.CheckNotChangedOn<Event3>();
        checker.CheckChangedOn<Event1>(TestState::_2);
        checker.CheckNotChangedOn<Event2, Event3>();
        checker.CheckChangedOn<Event1>(TestState::_1);
        checker.CheckChangedOn<Event2>(TestState::_3);
        checker.CheckNotChangedOn<Event1, Event3>();
        checker.CheckChangedOn<Event2>(TestState::_1);
    }
}

TEST_CASE("Check state transition callbacks", "[StateMachine]" )
{
    TransitionsCallbacks sm{ TestState::_1 };
    constexpr int data{ 42 };

    REQUIRE_NOTHROW(sm.ProcessEvent(Event3{})); // 1 -> x
    REQUIRE(sm.enterData.empty());
    REQUIRE(sm.leaveData.empty());

    REQUIRE_NOTHROW(sm.ProcessEvent(Event1{})); // 1 -> 2
    REQUIRE(sm.enterData.empty());
    REQUIRE(sm.leaveData.empty());

    REQUIRE_NOTHROW(sm.ProcessEvent(Event1{ {data} })); // 2 -> 3
    REQUIRE(sm.leaveData.size() == 1);
    REQUIRE(sm.enterData.size() == 1);
    REQUIRE(sm.leaveData.back() == data);
    REQUIRE(sm.enterData.back() == data);
    sm.enterData.clear();
    sm.leaveData.clear();

    REQUIRE_NOTHROW(sm.ProcessEvent(Event1{ {data} })); // 3 -> 1
    REQUIRE(sm.leaveData.size() == 1);
    REQUIRE(sm.enterData.size() == 1);
    REQUIRE(sm.leaveData.back() == data);
    REQUIRE(sm.enterData.back() == data);
    sm.enterData.clear();
    sm.leaveData.clear();

    REQUIRE_NOTHROW(sm.ProcessEvent(Event2{ {data} })); // 1 -> 4
    REQUIRE(sm.enterData.empty());
    REQUIRE(sm.leaveData.empty());
}

TEST_CASE("Check action dispatch", "[StateMachine]" )
{
    SECTION("Single")
    {
        ActionsSingle sm{ TestState::_1 };

        sm.ProcessEvent(Event2{ {1} });
        REQUIRE(sm.data == 0);

        sm.ProcessEvent(Event1{ {1} });
        REQUIRE(sm.data == 1);
    }

    SECTION("Multiple")
    {
        ActionsMultiple sm{ TestState::_1 };

        sm.ProcessEvent(Event3{ {1} });
        REQUIRE(sm.data == 0);

        sm.ProcessEvent(Event1{ {2} });
        REQUIRE(sm.data == 4);

        sm.ProcessEvent(Event2{ {2} });
        REQUIRE(sm.data == 12);
    }

    SECTION("Conditions")
    {
        ActionsConditions  sm{ TestState::_1 };

        sm.ProcessEvent(Event1{ {1} });
        REQUIRE(sm.data == -1);

        sm.ProcessEvent(Event2{ {2} });
        REQUIRE(sm.data == 2);

        sm.ProcessEvent(Event3{ {3} });
        REQUIRE(sm.data == -1);
    }
}

}// csm::test
