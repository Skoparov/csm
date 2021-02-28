#ifndef CSM_TEST_MOCKS_H
#define CSM_TEST_MOCKS_H

#include "transition_table.h"
#include "detail/handler_mock.h"
#include "detail/transition_checker.h"

namespace csm::test{

struct MockHandlerAcceptAll : HandlerMock<MockHandlerAcceptAll, State, Event>
{
    MOCK_ON_TRANSITION(From, To)
    MOCK_ON_ENTER_STATE(From)
    MOCK_ON_LEAVE_STATE(To)

    RequiredCalls GetRequiredCallsImpl(State from, State to) const noexcept
    {
        RequiredCalls calls{ Transition };
        if (from != to)
        {
            calls = calls | Leave | Enter;
        }

        return calls;
    }
};

struct MockHandlerSelective : HandlerMock<MockHandlerSelective, State, Event>
{
    static constexpr State AllowedFrom{ State::_0 };
    static constexpr State AllowedTo{ State::_2 };

    MOCK_ON_TRANSITION_COND(From, To, From == AllowedFrom && To == AllowedTo)
    MOCK_ON_ENTER_STATE_COND(From, From == AllowedTo)
    MOCK_ON_LEAVE_STATE_COND(To, To == AllowedFrom)

    RequiredCalls GetRequiredCallsImpl(State from, State to) const noexcept
    {
        RequiredCalls calls{ None };
        if (from == AllowedFrom && from != to)
        {
            calls = calls | Leave;
        }

        if (to == AllowedTo && from != to)
        {
            calls = calls | Enter;
        }

        if (from == AllowedFrom && to == AllowedTo)
        {
            calls = calls | Transition;
        }

        return calls;
    }
};

}// csm::test

#endif //CSM_TEST_MOCKS_H
