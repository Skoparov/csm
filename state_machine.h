#ifndef CPSM
#define CPSM

#include <type_traits>

namespace cpsm {
namespace detail {

template<class... T>
struct Pack {};

// ad-hoc as MSVC can't properly deduce ValPack<auto Val, auto... Vals>
template<auto Val, auto... Vals>
struct ValType
{
    using Type = decltype(Val);
    static_assert(std::conjunction_v<std::is_same<Type, decltype(Vals)>...>);
};

template<auto... Vals>
struct ValPack
{
    using Type = typename ValType<Vals...>::Type;
};

template<auto Target, class Pack>
struct ContainsValue : std::false_type {};

template<auto Target, auto Curr, auto... Values>
struct ContainsValue<Target, ValPack<Curr, Values...>>
    : std::conditional_t<
        Target == Curr,
        std::true_type,
        ContainsValue<Target, ValPack<Values...>>> {};

template<class FromStates, class OnEvents>
struct If;

template<auto... From, auto... Events>
struct If<ValPack<From...>, ValPack<Events...>>
{
    using State = typename ValPack<From...>::Type;
    using Event = typename ValPack<Events...>::Type;
};

template<auto To, class If, class... IfPack>
struct TransitionPack
{
    using State = typename If::State;
    using Event = typename If::Event;
};

template<auto From, auto To, class EventsList>
struct Transition
{
    using OnEvents = EventsList;
    static constexpr auto FromState{ From };
    static constexpr auto ToState{ To };
};

template<class State, class Event, class TransitionPack>
struct IsValidPack;

template<class State, class Event, auto To, class... IfPack>
struct IsValidPack<State, Event, TransitionPack<To, IfPack...>> :
        std::conjunction<
            std::is_same<State, decltype(To)>,
            std::conjunction<std::is_same<State, typename IfPack::State>...>,
            std::conjunction<std::is_same<Event, typename IfPack::Event>...>> {};

template<class... T>
struct Merge;

template<class... T>
struct Merge<Pack<T...>>
{
    using Type = Pack<T...>;
};

template<class... T, class... U, class... Tail>
struct Merge<Pack<T...>, Pack<U...>, Tail...>
{
    using Type = typename Merge<Pack<T..., U...>, Tail...>::Type;
};

template<class... T>
using MergeT = typename Merge<T...>::Type;

template<auto To, class If>
struct Expand;

template<auto To, auto... FromStates, auto... Events>
struct Expand<To, If<ValPack<FromStates...>, ValPack<Events...>>>
{
    using Type = Pack<Transition<FromStates, To, ValPack<Events...>>...>;
};

template<class T>
struct ExpandPack;

template<auto To, class... IfPack>
struct ExpandPack<TransitionPack<To, IfPack...>>
{
    using Type = MergeT<typename Expand<To, IfPack>::Type...>;
};

template<class State, class Event, class... TransitionPacks>
struct ExpandPacks
{
    static_assert(std::conjunction_v<
        IsValidPack<State, Event, TransitionPacks>...>);

    using Type = MergeT<typename ExpandPack<TransitionPacks>::Type...>;
};

template<class State, class Events, class Transitions>
struct Table;

template<auto Event, class... Transitions>
struct GetPossibleTransitions;

template<auto Event, class Transition, class... Transitions>
struct GetPossibleTransitions<Event, Transition, Transitions...>
{
    using Type = std::conditional_t<
        ContainsValue<Event, typename Transition::OnEvents>::value,
        MergeT<Pack<Transition>, typename GetPossibleTransitions<Event, Transitions...>::Type>,
        typename GetPossibleTransitions<Event, Transitions...>::Type>;
};

template<auto Event>
struct GetPossibleTransitions<Event>
{
    using Type = Pack<>;
};

}//detail

template<class Table, class Handler>
class StateMachine;

template<class State, class Event, class... Transitions, class Handler>
class StateMachine<detail::Table<State, Event, detail::Pack<Transitions...>>, Handler>
{
    static_assert(std::is_enum_v<State>);
    static_assert(std::is_enum_v<Event>);

public:
    StateMachine(Handler& handler, State startState) noexcept
        : m_state(startState)
        , m_handler(handler)
    {}

    template<Event E>
    void ProcessEvent()
    {
        using PossibleTransitions =
            typename detail::GetPossibleTransitions<E, Transitions...>::Type;

        constexpr PossibleTransitions possibleTransitions;
        Dispatch<E>(possibleTransitions);
    }

    State GetState() const noexcept
    {
        return m_state;
    }

private:
    template<Event E, class... PossibleTransitions>
    void Dispatch(detail::Pack<PossibleTransitions...>)
    {
        Dispatch<E, PossibleTransitions...>();
    }

    template<Event E, class Transition, class... Ts>
    void Dispatch()
    {
        if (m_state == Transition::FromState)
        {
            if (m_state != Transition::ToState)
            {
                OnLeaveState<Transition::FromState>(m_handler);
            }

            OnTransition<Transition::FromState, Transition::ToState>(m_handler);

            if (m_state != Transition::ToState)
            {
                OnEnterState<Transition::ToState>(m_handler);
                m_state = Transition::ToState;
            }
        }
        else
        {
            Dispatch<E, Ts...>();
        }
    }

    template<Event E, class... Ts>
    std::enable_if_t<!sizeof...(Ts)> Dispatch() noexcept {}

    template<State From, State To, class H>
    static auto OnTransition(H& handler)
        -> decltype(handler.template OnTransition<From, To>())
    {
        static_assert(std::is_same_v<
                decltype(handler.template OnTransition<From, To>()),
                void>);

        handler.template OnTransition<From, To>();
    }

    template<State, State>
    static void OnTransition(...) noexcept{}

    template<State S, class H>
    static auto OnEnterState(H& handler)
        -> decltype(handler.template OnEnterState<S>())
    {
        static_assert(std::is_same_v<
                decltype(handler.template OnEnterState<S>()),
                void>);

        handler.template OnEnterState<S>();
    }

    template<State>
    static void OnEnterState(...) noexcept {}

    template<State S, class H>
    static auto OnLeaveState(H& handler)
        -> decltype(handler.template OnLeaveState<S>())
    {
        static_assert(std::is_same_v<
                decltype(handler.template OnLeaveState<S>()),
                void>);

        handler.template OnLeaveState<S>();
    }

    template<State S>
    static void OnLeaveState(...) noexcept {}

private:
    State m_state;
    Handler& m_handler;
};

template<class TransitionPack, class... TransitionPacks>
using TransitionTable = detail::Table<
    typename TransitionPack::State,
    typename TransitionPack::Event,
    typename detail::ExpandPacks<
        typename TransitionPack::State,
        typename TransitionPack::Event,
        TransitionPack, TransitionPacks...>::Type>;

template<auto... States>
using From = detail::ValPack<States...>;

template<auto... Events>
using On = detail::ValPack<Events...>;

template<class FromStates, class OnEvents>
using If = detail::If<FromStates, OnEvents>;

template<auto ToState, class... IfPack>
using To = detail::TransitionPack<ToState, IfPack...>;

}// state_machine

#endif // CPSM
