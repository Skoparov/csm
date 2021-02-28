#ifndef CSM_TEST_REQUIRED_CALLS_H
#define CSM_TEST_REQUIRED_CALLS_H

#include <type_traits>

namespace csm::test{

template<class T, std::enable_if_t<std::is_enum_v<T>>...>
constexpr T operator|(T l, T r) noexcept
{
    return static_cast<T>(static_cast<int>(l) | static_cast<int>(r));
}

enum RequiredCalls{ None = 0, Enter = 1, Transition = 2, Leave = 4 };

}// namespace csm::test

#endif // CSM_TEST_REQUIRED_CALLS_H
