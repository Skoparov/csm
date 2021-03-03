#define CATCH_CONFIG_MAIN

#include "mocks.h"
#include "detail/transition_checker.h"

namespace csm::test{

SCENARIO("TransitionCheck", "[StateMachine]")
{
    GIVEN("Handler accepting all calls")
    {
        TransitionChecker<
            MockHandlerAcceptAll,
            TestTransitionTable> checker{ StartState };

        WHEN("All transitions triggered separately")
        {
            THEN("All callbacks are called and states changed correctly")
            {
                CheckTransitions(AllTransitions{}, checker);
            }
        }
        WHEN("Consequtive transitions triggered")
        {
            THEN("Corresponding callbacks are called and states changed correctly")
            {
                // S0 + E0 = S2
                checker.CheckCallsOnEvent<Event::_0>(EventData, State::_2);

                // S2 + E1 = S1
                checker.CheckCallsOnEvent<Event::_1>(EventData, State::_1);
            }
        }
        WHEN("Transition not found")
        {
            THEN("callbacks not called and state not changed")
            {
                checker.CheckCallsOnEvent<Event::Unused>(EventData);
            }
        }
    }

    GIVEN("State machine with no handler")
    {
        csm::StateMachine<TestTransitionTable, MockHandlerAcceptAll> sm{ StartState };
        WHEN("Transition triggered")
        {
            REQUIRE_NOTHROW(sm.ProcessEvent<Event::_0>());
            THEN("State changed")
            {
                REQUIRE(sm.GetState() == State::_2);
            }
        }
    }
}

SCENARIO("SelectiveConditionsCheck", "[StateMachine]")
{
    GIVEN("Handler accepting only selected calls")
    {
        TransitionChecker<
                MockHandlerSelective,
                TestTransitionTable> selectiveChecker{ StartState };

        WHEN("All transitions triggered separately")
        {
            THEN("Only selected callbacks are called and states changed correctly")
            {
                CheckTransitions(AllTransitions{}, selectiveChecker);
            }
        }
    }
}

SCENARIO("AllowedConditionsCheck", "[StateMachine]")
{
    GIVEN("Handler accepting all calls")
    {
        TransitionChecker<
            MockHandlerAllowedConditions,
            TestTransitionTable> checker{ StartState };

        WHEN("All transitions triggered separately")
        {
            THEN("Conditions checked, Allowed callbacks are called, states changed correctly")
            {
                CheckTransitions(AllTransitions{}, checker);
            }
        }

    }
}

}// csm::test
