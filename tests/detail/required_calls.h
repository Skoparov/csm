#ifndef CSM_TEST_REQUIRED_CALLS_H
#define CSM_TEST_REQUIRED_CALLS_H

#include <type_traits>

namespace csm::test{

template<class T, std::enable_if_t<std::is_enum_v<T>>...>
constexpr T operator|(T l, T r) noexcept
{
    return static_cast<T>(static_cast<int>(l) | static_cast<int>(r));
}

template<class T, std::enable_if_t<std::is_enum_v<T>>...>
constexpr T operator|=(T& l, T r) noexcept
{
    l = l | r;
    return l;
}

enum RequiredCalls
{
    NoCalls = 0,
    EnterCall = 1 << 0,
    TransitionCall = 1 << 1,
    LeaveCall = 1 << 2,
    TransitionAllowedCall = 1 << 3,
    EventAllowedCall = 1 << 4,
    EventCall = 1 << 5
};

}// namespace csm::test

#endif // CSM_TEST_REQUIRED_CALLS_H
