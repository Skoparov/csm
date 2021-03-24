#define CATCH_CONFIG_MAIN

#include "test_helpers.h"

namespace csm::test{

TEST_CASE("Traits check", "[Details]")
{
    using namespace detail;
    static_assert(HasOnEnterV<HasEnter, TestState::_1, DummyImpl, Event1>);
    static_assert(!HasOnLeaveV<HasEnter, TestState::_1, DummyImpl, Event1>);
    static_assert(!HasOnEnterV<HasLeave, TestState::_1, DummyImpl, Event1>);
    static_assert(HasOnLeaveV<HasLeave, TestState::_1, DummyImpl, Event1>);

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
    DummyImpl impl{};

    SECTION("Not")
    {
        REQUIRE(Not<Return<true>>{}(impl) == false);
        REQUIRE(Not<Return<false>>{}(impl) == true);
        REQUIRE(Not<Not<Return<false>>>{}(impl) == false);
    }

    SECTION("All")
    {
        REQUIRE(All<Return<true>>{}(impl) == true);
        REQUIRE(All<Return<true>, Not<Return<false>>>{}(impl) == true);
        REQUIRE(All<Return<false>>{}(impl) == false);
        REQUIRE(All<Return<true>, Return<false>>{}(impl) == false);
        REQUIRE(All<Return<false>, Not<Return<true>>>{}(impl) == false);
    }

    SECTION("Any")
    {
        REQUIRE(Any<Return<true>>{}(impl) == true);
        REQUIRE(Any<Return<true>, Not<Return<false>>>{}(impl) == true);
        REQUIRE(Any<Return<false>>{}(impl) == false);
        REQUIRE(Any<Return<true>, Return<false>>{}(impl) == true);
        REQUIRE(Any<Return<false>, Not<Return<true>>>{}(impl) == false);
    }

    SECTION("None")
    {
        REQUIRE(None<Return<true>>{}(impl) == false);
        REQUIRE(None<Return<true>, Not<Return<false>>>{}(impl) == false);
        REQUIRE(None<Return<false>>{}(impl) == true);
        REQUIRE(None<Return<true>, Return<false>>{}(impl) == false);
        REQUIRE(None<Return<false>, Not<Return<true>>>{}(impl) == true);
    }
}

TEST_CASE("Transitions generation", "[Details]" )
{
    using namespace detail;
    static_assert(std::is_same_v<
        MakeTable<TableSingle>,
        Table<TestState, Pack<Transition<ImplBase::State1, ImplBase::State2, Pack<Event1>, Dummy>>>>);

    static_assert(std::is_same_v<
        MakeTable<TableMultiple>,
        Table<TestState, Pack<
            Transition<ImplBase::State1, ImplBase::State3, Pack<Event1, Event2>, Dummy>,
            Transition<ImplBase::State2, ImplBase::State3, Pack<Event1, Event2>, Dummy>>>>);

    static_assert(std::is_same_v<
        MakeTable<TableOr>,
        Table<TestState, Pack<
            Transition<ImplBase::State1, ImplBase::State3, Pack<Event1>, Dummy>,
            Transition<ImplBase::State2, ImplBase::State3, Pack<Event2>, Dummy>,
            Transition<ImplBase::State1, ImplBase::State3, Pack<Event3>, Dummy>>>>);

    static_assert(std::is_same_v<
        MakeTable<TableSeveral>,
        Table<TestState, Pack<
            Transition<ImplBase::State1, ImplBase::State3, Pack<Event1>, Dummy>,
            Transition<ImplBase::State3, ImplBase::State1, Pack<Event2>, Dummy>>>>);

    static_assert(std::is_same_v<
        MakeTable<TableConditions>,
        Table<TestState, Pack<
            Transition<ImplBase::State1, ImplBase::State3, Pack<Event1>, If<ImplBase, Return<false>>>,
            Transition<ImplBase::State1, ImplBase::State3, Pack<Event2>, Dummy>,
            Transition<ImplBase::State2, ImplBase::State1, Pack<Event1>, Dummy>,
            Transition<ImplBase::State3, ImplBase::State1, Pack<Event2>, If<ImplBase, Return<true>>>,
            Transition<ImplBase::State1, ImplBase::State2, Pack<Event1>, If<ImplBase, Return<true>>>,
            Transition<ImplBase::State3, ImplBase::State2, Pack<Event3>, If<ImplBase, Return<false>>>>>>);
}

TEST_CASE("Action rules generation", "[Details]" )
{
    using namespace detail;
    static_assert(std::is_same_v<
        MakeRules<ActionRulesSingle>,
        Pack<EvExprPack<Do<ImplBase, Action1>, EvExpr<Pack<Event1>, Dummy>>>>);

    static_assert(std::is_same_v<
        MakeRules<ActionRulesMultiple>,
        Pack<EvExprPack<Do<ImplBase, Action1, Action2>, EvExpr<Pack<Event1, Event2>, Dummy>>>>);

    static_assert(std::is_same_v<
        MakeRules<ActionRulesOrAndCondition>,
        Pack<
            EvExprPack<Do<ImplBase, Action1, Action2>,
                EvExpr<Pack<Event3>, Dummy>,
                EvExpr<Pack<Event1, Event2>, If<ImplBase, Return<true>>>>,
            EvExprPack<Do<ImplBase, Action1, Action2>,
                EvExpr<Pack<Event1, Event2>, Dummy>,
                EvExpr<Pack<Event3>, If<ImplBase, Return<true>>>>,
            EvExprPack<Do<ImplBase, Action1, Action2>,
                EvExpr<Pack<Event1>, If<ImplBase, Return<true>>>,
                EvExpr<Pack<Event2>, If<ImplBase, Return<true>>>,
                EvExpr<Pack<Event3>, If<ImplBase, Return<true>>>>>>);
}

TEST_CASE("Check state transitions", "[StateMachine]" )
{
    SECTION("Single")
    {
        SequentialTransitionChecker<TableSingle> checker{ TestState::_1 };
        checker.CheckNotChangedOn<Event2, Event3>();
        checker.CheckChangedOn<Event1>(TestState::_2);
        checker.CheckNotChangedOn<Event1, Event2, Event3>();
    }

    SECTION("Multiple")
    {
        SequentialTransitionChecker<TableMultiple> checker{ TestState::_1 };
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
        SequentialTransitionChecker<TableOr> checker{ TestState::_1 };
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
        SequentialTransitionChecker<TableSeveral> checker{ TestState::_1 };
        checker.CheckNotChangedOn<Event2, Event3>();
        checker.CheckChangedOn<Event1>(TestState::_3);
        checker.CheckNotChangedOn<Event1, Event3>();
        checker.CheckChangedOn<Event2>(TestState::_1);
    }

    SECTION("TableConditions")
    {
        SequentialTransitionChecker<TableConditions> checker{ TestState::_1 };
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
    TransitionCallbacks cb;
    StateMachine<TransitionCallbacks> sm{ TestState::_1, cb };

    constexpr int data{ 42 };

    REQUIRE_NOTHROW(sm.ProcessEvent(Event3{})); // 1 -> x
    REQUIRE(cb.enterData.empty());
    REQUIRE(cb.leaveData.empty());

    REQUIRE_NOTHROW(sm.ProcessEvent(Event1{})); // 1 -> 2
    REQUIRE(cb.enterData.empty());
    REQUIRE(cb.leaveData.empty());

    REQUIRE_NOTHROW(sm.ProcessEvent(Event1{ data })); // 2 -> 3
    REQUIRE(cb.leaveData.size() == 1);
    REQUIRE(cb.enterData.size() == 1);
    REQUIRE(cb.leaveData.back() == data);
    REQUIRE(cb.enterData.back() == data);
    cb.enterData.clear();
    cb.leaveData.clear();

    REQUIRE_NOTHROW(sm.ProcessEvent(Event1{ data })); // 3 -> 1
    REQUIRE(cb.leaveData.size() == 1);
    REQUIRE(cb.enterData.size() == 1);
    REQUIRE(cb.leaveData.back() == data);
    REQUIRE(cb.enterData.back() == data);
    cb.enterData.clear();
    cb.leaveData.clear();

    REQUIRE_NOTHROW(sm.ProcessEvent(Event2{ data })); // 1 -> 4
    REQUIRE(cb.enterData.empty());
    REQUIRE(cb.leaveData.empty());
}

TEST_CASE("Check action dispatch", "[StateMachine]" )
{
    SECTION("Single")
    {
        ActionsSingle actions;
        StateMachine<ActionsSingle> sm(TestState::_1, actions);

        sm.ProcessEvent(Event2{ 1 });
        REQUIRE(actions.data == 0);

        sm.ProcessEvent(Event1{ 1 });
        REQUIRE(actions.data == 1);
    }

    SECTION("Multiple")
    {
        ActionsMultiple actions;
        StateMachine<ActionsMultiple> sm(TestState::_1, actions);

        sm.ProcessEvent(Event3{ 1 });
        REQUIRE(actions.data == 0);

        sm.ProcessEvent(Event1{ 2 });
        REQUIRE(actions.data == 4);

        sm.ProcessEvent(Event2{ 2 });
        REQUIRE(actions.data == 12);
    }

    SECTION("Conditions")
    {
        ActionsConditions actions;
        StateMachine<ActionsConditions> sm(TestState::_1, actions);

        sm.ProcessEvent(Event1{ 1 });
        REQUIRE(actions.data == -1);

        sm.ProcessEvent(Event2{ 2 });
        REQUIRE(actions.data == 2);

        sm.ProcessEvent(Event3{ 3 });
        REQUIRE(actions.data == -1);
    }
}

}// csm::test
