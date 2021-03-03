#ifndef CSM_STATE_MACHINE
#define CSM_STATE_MACHINE

#include <cstddef>
#include <type_traits>

namespace csm {
namespace detail {

template<class... Ts>
struct Pack
{
    static constexpr size_t Size{ sizeof...(Ts) };
};

template<auto Val, auto... Vals>
struct ValPack
{
    using Type = decltype(Val);

    template<auto Target>
    static constexpr bool Contains{
        ((Target == Vals) || ... || (Target == Val)) };
};

template<auto... States>
struct From : ValPack<States...> {};

template<auto... Events>
struct On : ValPack<Events...> {};

template<auto State>
struct To {};

template<auto To, class If, class... IfPack>
struct TransitionPack
{
    using State = typename If::State;
    using Event = typename If::Event;
};

template<class FromStates, class OnEvents>
struct If;

template<auto... States, auto... Events>
struct If<From<States...>, On<Events...>>
{
    using State = typename From<States...>::Type;
    using Event = typename On<Events...>::Type;

    template<State ToState>
    constexpr auto operator=(To<ToState>) noexcept
    {
        return TransitionPack<ToState, If<From<States...>, On<Events...>>>{};
    }
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
struct Expand<To, If<From<FromStates...>, On<Events...>>>
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

template<class State, class Event, class... Packs>
struct ExpandPacks
{
    static_assert(std::conjunction_v<IsValidPack<State, Event, Packs>...>);
    using Type = MergeT<typename ExpandPack<Packs>::Type...>;
};

template<auto Event, class Transition, class... Transitions, class... FittingTransitions>
constexpr auto GetPossibleTransitions(Pack<FittingTransitions...> = Pack<>{}) noexcept
{
    using Events = typename Transition::OnEvents;
    using Type = std::conditional_t<
        Events::template Contains<Event>,
        Pack<Transition, FittingTransitions...>,
        Pack<FittingTransitions...>>;

    if constexpr(sizeof...(Transitions) != 0)
    {
        return GetPossibleTransitions<Event, Transitions...>(Type{});
    }
    else
    {
        return Type{};
    }
}

template<class... Ifs>
struct IfPack
{
    template<auto State>
    constexpr auto operator=(To<State>) noexcept
    {
        return TransitionPack<State, Ifs...>{};
    }
};

template<class FromStates1, class OnEvents1, class FromStates2, class OnEvents2>
constexpr auto operator||(If<FromStates1, OnEvents1>, If<FromStates2, OnEvents2>) noexcept
{
    return IfPack<If<FromStates1, OnEvents1>, If<FromStates2, OnEvents2>>{};
}

template<class... Ifs, class FromStates, class OnEvents>
constexpr auto operator||(IfPack<Ifs...>, If<FromStates, OnEvents>) noexcept
{
    return IfPack<Ifs..., If<FromStates, OnEvents>>{};
}

template<auto... States, auto... Events>
constexpr auto operator+(From<States...>, On<Events...>) noexcept
{
    return If<From<States...>, On<Events...>>{};
}

template<class... T>
constexpr auto MakePack(T&&...) noexcept
{
    return Pack<std::decay_t<T>...>{};
}

template<class State, class Events, class Transitions>
struct Table;

template<class T>
struct MakeTransitionTable;

template<class TransitionPack, class... Packs>
struct MakeTransitionTable<Pack<TransitionPack, Packs...>>
{
    using State = typename TransitionPack::State;
    using Event = typename TransitionPack::Event;
    using Transitions =
        typename ExpandPacks<State, Event, TransitionPack, Packs...>::Type;

    using Type = Table<State, Event, Transitions>;
};

template<class T>
using MakeTransitionTableT = typename MakeTransitionTable<std::decay_t<decltype(T::PossibleTransition)>>::Type;

template<class Handler, auto From, auto To, class = void>
struct HasOnTransition : std::false_type {};

template<class Handler, auto From, auto To>
struct HasOnTransition<Handler, From, To,
    std::void_t<decltype(std::declval<Handler&>().template OnTransition<From, To>())>> : std::true_type
{
    using R = decltype(std::declval<Handler&>().template OnTransition<From, To>());
    static_assert(std::is_same_v<R, void>, "OnTransition() should return void");
};

template<class Handler, auto From, auto To>
constexpr bool HasOnTransitionV{ HasOnTransition<Handler, From, To>::value };

template<class Handler, auto S, class = void>
struct HasOnEnter : std::false_type {};

template<class Handler, auto S>
struct HasOnEnter<Handler, S,
    std::void_t<decltype(std::declval<Handler&>().template OnEnterState<S>())>> : std::true_type
{
    using R = decltype(std::declval<Handler&>().template OnEnterState<S>());
    static_assert(std::is_same_v<R, void>, "OnEnterState() should return void");
};

template<class Handler, auto S>
constexpr bool HasOnEnterV{ HasOnEnter<Handler, S>::value };

template<class Handler, auto S, class = void>
struct HasOnLeave : std::false_type {};

template<class Handler, auto S>
struct HasOnLeave<Handler, S,
    std::void_t<decltype(std::declval<Handler&>().template OnLeaveState<S>())>> : std::true_type
{
    using R = decltype(std::declval<Handler&>().template OnLeaveState<S>());
    static_assert(std::is_same_v<R, void>, "OnLeaveState() should return void");
};

template<class Handler, auto S>
constexpr bool HasOnLeaveV{ HasOnLeave<Handler, S>::value };

template<class Handler, auto S, class ArgsPack, class = void>
struct HasOnEvent : std::false_type {};

template<class Handler, auto E, class... Args>
struct HasOnEvent<Handler, E, Pack<Args...>,
    std::void_t<decltype(std::declval<Handler&>().template OnEvent<E>(std::declval<Args>()...))>> : std::true_type
{
    using R = decltype(std::declval<Handler&>().template OnEvent<E>(std::declval<Args>()...));
    static_assert(std::is_same_v<R, void>, "OnEvent() should return void");
};

template<class Handler, auto E, class ArgsPack>
constexpr bool HasOnEventV{ HasOnEvent<Handler, E, ArgsPack>::value };

template<class Handler, auto From, auto To, class = void>
struct HasAllowTransition : std::false_type {};

template<class Handler, auto From, auto To>
struct HasAllowTransition<Handler, From, To,
    std::void_t<decltype(std::declval<Handler&>().template AllowTransition<From, To>())>> : std::true_type
{
    using R = decltype(std::declval<Handler&>().template AllowTransition<From, To>());
    static_assert(std::is_same_v<R, bool>, "AllowTransition() should return bool");
};

template<class Handler, auto From, auto To>
constexpr bool HasAllowTransitionV{ HasAllowTransition<Handler, From, To>::value };

template<class Handler, auto E, class = void>
struct HasAllowEvent : std::false_type {};

template<class Handler, auto E>
struct HasAllowEvent<Handler, E,
    std::void_t<decltype(std::declval<Handler&>().template AllowEvent<E>())>> : std::true_type
{
    using R = decltype(std::declval<Handler&>().template AllowEvent<E>());
    static_assert(std::is_same_v<R, bool>, "AllowEvent() should return bool");
};

template<class Handler, auto E>
constexpr bool HasAllowEventV{ HasAllowEvent<Handler, E>::value };

template<class Table, class Handler>
class EventProcessor;

template<class State, class Event, class... Transitions, class Handler>
class EventProcessor<Table<State, Event, Pack<Transitions...>>, Handler>
{
public:
    using StateT = State;
    using EventT = Event;

public:
    EventProcessor(State startState, Handler* handler) noexcept
        : m_state(startState)
        , m_handler(handler)
    {}

    template<auto E, class... Args>
    void ProcessEvent(Args&&... args)
    {
        if constexpr(HasAllowEventV<Handler, E>)
        {
            if (m_handler && !m_handler-> template AllowEvent<E>())
                return;
        }

        if constexpr(HasOnEventV<Handler, E, Pack<Args...>>)
        {
            if (m_handler)
            {
                m_handler->template OnEvent<E>(std::forward<Args>(args)...);
            }
        }

        constexpr auto PossibleTransitions{ GetPossibleTransitions<E, Transitions...>() };
        if constexpr(PossibleTransitions.Size != 0)
        {
            ProcessEvent<E>(PossibleTransitions, std::forward<Args>(args)...);
        }
    }

    State GetState() const noexcept
    {
        return m_state;
    }

    void SetHandler(Handler* h) noexcept
    {
        m_handler = h;
    }

private:
    template<Event E, class Tr, class... Trs, class... Args>
    void ProcessEvent(Pack<Tr, Trs...>, Args&&...args)
    {
        if (m_state == Tr::FromState)
        {
            bool transitionAllowed{ true };
            if (m_handler)
            {
                if constexpr(HasAllowTransitionV<Handler, Tr::FromState, Tr::ToState>)
                {
                    transitionAllowed = m_handler->template AllowTransition<Tr::FromState, Tr::ToState>();
                }

                if (transitionAllowed)
                {
                    constexpr bool stateChanged{ Tr::FromState != Tr::ToState };
                    if constexpr(HasOnLeaveV<Handler, Tr::FromState> && stateChanged)
                    {
                        m_handler->template OnLeaveState<Tr::FromState>();
                    }

                    if constexpr(HasOnTransitionV<Handler, Tr::FromState, Tr::ToState>)
                    {
                        m_handler->template OnTransition<Tr::FromState, Tr::ToState>();
                    }

                    if constexpr(HasOnEnterV<Handler, Tr::ToState> && stateChanged)
                    {
                        m_handler->template OnEnterState<Tr::ToState>();
                    }
                }
            }

            if (transitionAllowed)
            {
                m_state = Tr::ToState;
                return;
            }
        }

        if constexpr(sizeof...(Trs) != 0)
        {
            ProcessEvent<E>(Pack<Trs...>{}, std::forward<Args>(args)...);
        }
        else
        {
            (static_cast<void>(args), ...);
        }
    }

private:
    State m_state;
    Handler* m_handler;
};

template<class TransitionTable, class Handler>
using EventProcessorT = EventProcessor<
    MakeTransitionTableT<TransitionTable>,
    Handler>;

}//detail

template<class TransitionTable, class Handler>
class StateMachine
{
public:
    using Processor = detail::EventProcessorT<TransitionTable, Handler>;

public:
    using State = typename Processor::StateT;
    using Event = typename Processor::EventT;

public:
    StateMachine(State startState, Handler* handler = nullptr) noexcept
        : m_processor(startState, handler) {}

    template<auto E, class... Args>
    void ProcessEvent(Args&&... args)
    {
        m_processor.template ProcessEvent<E>(std::forward<Args>(args)...);
    }

    State GetState() const noexcept
    {
        return m_processor.GetState();
    }

    void SetHandler(Handler* handler) noexcept
    {
        m_processor.SetHandler(handler);
    }

private:
    Processor m_processor;
};

template<class State, class Event>
struct TransitionTableBase
{
    static_assert(std::is_enum_v<State>);
    static_assert(std::is_enum_v<Event>);

    template<State... FromStates>
    static constexpr detail::From<FromStates...> From{};

    template<Event... OnEvents>
    static constexpr detail::On<OnEvents...> On{};

    template<State ToState>
    static constexpr detail::To<ToState> To{};
};

// These transitions describe possible ways the state may change
// If a transition is only allowed upon meeting a certain condition
// i.e transition to Dead is only allowed if "health == 0"
// you can impose this restriction in the handler

template<class... T>
constexpr auto MakePossibleTransitions(T&&...) noexcept
{
    return detail::Pack<std::decay_t<T>...>{};
}

}// csm

#endif // CSM_STATE_MACHINE
