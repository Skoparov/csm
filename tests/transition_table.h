#ifndef CSM_TEST_TRANSITION_TABLE_H
#define CSM_TEST_TRANSITION_TABLE_H

#include <csm.h>

namespace csm::test{

enum class State{ _0, _1, _2, Unused };
enum class Event{ _0, _1, _2, Unused };

constexpr State StartState{ State::_0 };

struct TestTransitionTable : csm::TransitionTableBase<State, Event>
{
    static constexpr auto PossibleTransition{ csm::MakePossibleTransitions
    (
        (From<State::_0> + On<Event::_0, Event::_1>) ||
        (From<State::_0, State::_1, State::_2> + On<Event::_2>) = To<State::_2>,
        From<State::_2, State::_1> + On<Event::_0> = To<State::_0>,
        From<State::_2> + On<Event::_1> = To<State::_1>
    )};
};

namespace csmd = csm::detail;

// Ref transitions after expansion
using AllTransitions = csmd::Pack<
    csmd::Transition<State::_0, State::_2, csmd::ValPack<Event::_0, Event::_1>>,
    csmd::Transition<State::_0, State::_2, csmd::ValPack<Event::_2>>,
    csmd::Transition<State::_1, State::_2, csmd::ValPack<Event::_2>>,
    csmd::Transition<State::_2, State::_2, csmd::ValPack<Event::_2>>,
    csmd::Transition<State::_2, State::_0, csmd::ValPack<Event::_0>>,
    csmd::Transition<State::_1, State::_0, csmd::ValPack<Event::_0>>,
    csmd::Transition<State::_2, State::_1, csmd::ValPack<Event::_1>>>;

// Check that transition table is generated correctly
static_assert(std::is_same_v<
    csmd::Table<State, Event, AllTransitions>,
    csmd::MakeTransitionTableT<TestTransitionTable>>);

}// csm::test

#endif// CSM_TEST_TRANSITION_TABLE_H
