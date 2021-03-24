#ifndef CSM_STATE_MACHINE
#define CSM_STATE_MACHINE

#include <cstddef>
#include <type_traits>

namespace csm {
namespace detail {

struct Dummy;

template<class T>
constexpr bool IsInitalized{ !std::is_same_v<T, Dummy> };

template<class T>
decltype(std::declval<T>()) As() noexcept;

template<class T, auto To, class Impl, class E, class = void>
struct HasOnEnter : std::false_type {};

template<class T, auto To, class Impl, class E>
struct HasOnEnter<T, To, Impl, E, std::void_t<
    decltype(As<T>().template OnEnter<To>(As<Impl&>(), As<E>()))>>
    : std::true_type
{
    using R = decltype(As<T>().template OnEnter<To>(As<Impl&>(), As<E>()));
    static_assert(std::is_same_v<R, void>, "OnEnter() should return void");
};

template<class T, auto To, class Impl, class E>
constexpr bool HasOnEnterV{ HasOnEnter<T, To, Impl, E>::value };

template<class T, auto To, class Impl, class E, class = void>
struct HasOnLeave : std::false_type {};

template<class T, auto To, class Impl, class E>
struct HasOnLeave<T, To, Impl, E, std::void_t<
    decltype(As<T>().template OnLeave<To>(As<Impl&>(), As<E>()))>> : std::true_type
{
    using R = decltype(As<T>().template OnLeave<To>(As<Impl&>(), As<E>()));
    static_assert(std::is_same_v<R, void>, "OnEnter() should return void");
};

template<class T, auto To, class Impl, class E>
constexpr bool HasOnLeaveV{ HasOnLeave<T, To, Impl, E>::value };

template<class... T>
struct HasDups;

template<class T, class... Ts>
struct HasDups<T, Ts...> : std::conditional_t<
    (std::is_same_v<std::decay_t<T>, std::decay_t<Ts>> || ...),
    std::true_type,
    HasDups<Ts...>>
{};

template<>
struct HasDups<> : std::false_type {};

template<class... Ts>
struct Pack
{
    static_assert (!HasDups<Ts...>::value);
    static constexpr size_t Size{ sizeof...(Ts) };

    template<class T>
    static constexpr bool Contains{ (std::is_same_v<T, Ts> || ...) };
};

template<class... T>
struct Merge;

template<class... T>
struct Merge<Pack<T...>>{ using Type = Pack<T...>; };

template<class... T, class... U, class... Tail>
struct Merge<Pack<T...>, Pack<U...>, Tail...>
{
    using Type = typename Merge<Pack<T..., U...>, Tail...>::Type;
};

template<class... T>
using MergeT = typename Merge<T...>::Type;

template<class Event, class... Handlers>
using FilterByEvent = MergeT<
    Pack<>,
    std::conditional_t<
        Handlers::template ContainsEvent<Event>,
        Pack<Handlers>,
        Pack<>>...>;

template<class State, class... States>
struct From : Pack<States...>
{
    using Type = typename State::Enum;
    static_assert(std::conjunction_v<std::is_same<Type, typename States::Enum>...>);
};

template<class... Ts>
struct TypesCheck
{
    static_assert (sizeof...(Ts) > 0);
    static_assert (!HasDups<Ts...>::value);
};

template<class Impl, class... Preds>
struct PredCheck : TypesCheck<Preds...>
{
    static_assert((std::is_invocable_r_v<bool, Preds, Impl&> && ...));
};

template<class Impl, class... Preds>
struct If : PredCheck<Impl, Preds...>
{
    bool operator()(Impl& impl)
    {
        return (Preds{}(impl) && ...);
    }
};

template<class Impl, class... Preds>
struct Any : PredCheck<Impl, Preds...>
{
    bool operator()(Impl& impl)
    {
        return (Preds{}(impl) || ...);
    }
};

template<class Impl, class... Preds>
struct All : PredCheck<Impl, Preds...>
{
    bool operator()(Impl& impl)
    {
        return (Preds{}(impl) && ...);
    }
};

template<class Impl, class... Preds>
struct None : PredCheck<Impl, Preds...>
{
    bool operator()(Impl& impl)
    {
        return (!Preds{}(impl) && ...);
    }
};

template<class Impl, class Pred>
struct Not : PredCheck<Impl, Pred>
{
    bool operator()(Impl& impl)
    {
        return !Pred{}(impl);
    }
};

template<class Impl, class... Actions>
struct Do : TypesCheck<Actions...>
{
    template<class Event>
    void operator()(Impl& impl, const Event& e)
    {
        static_assert((std::is_invocable_r_v<void, Actions, Impl&, const Event&> && ...));
        (Actions{}(impl, e), ...);
    }
};

template<class Events, class Cond>
struct EvExpr;

template<class Action, class EvExpr, class... EvExprs>
struct EvExprPack;

template<class... Events>
struct On : Pack<Events...>
{
    template<class Impl, class... Actions>
    constexpr auto operator=(Do<Impl, Actions...>) const noexcept
    {
        return EvExprPack<Do<Impl, Actions...>, EvExpr<Pack<Events...>, Dummy>>{};
    }
};

template<class State>
struct To {};

template<class ToState, class TrExpr, class... TrExprs>
struct TrExprPack
{
    using State = ToState;

    template<class State>
    constexpr auto operator=(To<State>) const noexcept
    {
        static_assert(!IsInitalized<ToState>);
        return TrExprPack<State, TrExpr, TrExprs...>{};
    }
};

template<class FromStates, class OnEvents, class CondPred>
struct TrExpr;

template<class State, class... States, class... Events, class CondPred>
struct TrExpr<From<State, States...>, On<Events...>, CondPred>
{
    using Enum = typename State::Enum;
    using Cond = CondPred;

    static_assert((std::is_same_v<Enum, typename States::Enum> && ...));

    template<class ToState>
    constexpr auto operator=(To<ToState>) const noexcept
    {
        return TrExprPack<ToState, TrExpr<From<State, States...>, On<Events...>, Cond>>{};
    }
};

template<class Cond>
struct AllowedOn
{
    template<class Impl>
    static bool IsAllowed(Impl& impl)
    {
        static_cast<void>(impl);
        if constexpr(IsInitalized<Cond>)
        {
            return Cond{}(impl);
        }

        return true;
    }
};

template<class Action, class EvExpr, class... EvExprs>
struct EvExprPack
{
    template<class Event>
    static constexpr bool ContainsEvent{
        EvExpr::template ContainsEvent<Event> ||
        (EvExprs:: template ContainsEvent<Event> || ...) };

    template<class Impl, class... Actions>
    constexpr auto operator=(Do<Impl, Actions...>) const noexcept
    {
        return EvExprPack<Do<Impl, Actions...>, EvExpr, EvExprs...>{};
    }

    template<class Impl, class Event>
    static bool Dispatch(Impl& impl, const Event& e)
    {
        static_assert(IsInitalized<Action>);
        using ExprsWithEvent = FilterByEvent<Event, EvExpr, EvExprs...>;
        return DispatchFiltered(impl, e, ExprsWithEvent{});
    }

private:
    template<class Impl, class Event, class... Exprs>
    static bool DispatchFiltered(Impl& impl, const Event& e, Pack<Exprs...>)
    {
        return (Exprs::template Dispatch<Action>(impl, e) || ...);
    }
};

template<class... Events, class Cond>
struct EvExpr<Pack<Events...>, Cond>
{
    template<class Event>
    static constexpr bool ContainsEvent{ Pack<Events...>::template Contains<Event> };

    template<class Action, class Impl, class Event>
    static bool Dispatch(Impl& impl, const Event& e)
    {
        if (AllowedOn<Cond>::IsAllowed(impl))
        {
            Action{}(impl, e);
            return true;
        }

        return false;
    }
};

template<class From, class To, class Events, class Cond>
struct Transition : AllowedOn<Cond>
{
    using OnEvents = Events;
    using FromState = From;
    using ToState = To;

    static_assert(std::is_same_v<typename From::Enum, typename To::Enum>);
    static_assert(From::EnumValue != To::EnumValue);

    template<class Event>
    static constexpr bool ContainsEvent{ OnEvents::template Contains<Event> };

    template<class Impl, class Event>
    static void OnEnter(Impl& impl, const Event& e)
    {
        static_cast<void>(impl);
        static_cast<void>(e);

        if constexpr(HasOnEnterV<To, From::EnumValue, Impl, Event>)
        {
            To{}.template OnEnter<From::EnumValue>(impl, e);
        }
    }

    template<class Impl, class Event>
    static void OnLeave(Impl& impl, const Event& e)
    {
        static_cast<void>(impl);
        static_cast<void>(e);

        if constexpr(HasOnLeaveV<From, To::EnumValue, Impl, Event>)
        {
            From{}.template OnLeave<To::EnumValue>(impl, e);
        }
    }
};

template<class StateEnum, class TrPack>
struct IsValidTrPack;

template<class StateEnum, class To, class... TrExprs>
struct IsValidTrPack<StateEnum, TrExprPack<To, TrExprs...>> :
    std::conjunction<
        std::is_same<StateEnum, typename To::Enum>,
        std::conjunction<std::is_same<StateEnum, typename TrExprs::Enum>...>> {};

template<class To, class If>
struct Expand;

template<class To, class... FromStates, class... Events, class CondPred>
struct Expand<To, TrExpr<From<FromStates...>, On<Events...>, CondPred>>
{
    using Type = Pack<Transition<FromStates, To, Pack<Events...>, CondPred>...>;
};

template<class T>
struct ExpandPack;

template<class To, class... IfPack>
struct ExpandPack<TrExprPack<To, IfPack...>>
{
    using Type = MergeT<typename Expand<To, IfPack>::Type...>;
};

template<class StateEnum, class... Packs>
struct ExpandPacks
{
    static_assert((IsValidTrPack<StateEnum, Packs>::value && ...));
    using Type = MergeT<typename ExpandPack<Packs>::Type...>;
};

template<class... States, class... Events>
constexpr auto operator&&(From<States...>, On<Events...>) noexcept
{
    return TrExpr<From<States...>, On<Events...>, Dummy>{};
}

template<class From, class On, class Impl, class... CondPreds>
constexpr auto operator&&(TrExpr<From, On, Dummy>, If<Impl, CondPreds...>) noexcept
{
    return TrExprPack<Dummy, TrExpr<From, On, If<Impl, CondPreds...>>>{};
}

template<class From1, class On1, class Cond1, class From2, class On2, class Cond2>
constexpr auto operator||(TrExpr<From1, On1, Cond1>, TrExpr<From2, On2, Cond2>) noexcept
{
    return TrExprPack<Dummy, TrExpr<From1, On1, Cond1>, TrExpr<From2, On2, Cond2>>{};
}

template<class... Conds, class From, class On, class Cond>
constexpr auto operator||(TrExprPack<Dummy, Conds...>, TrExpr<From, On, Cond>) noexcept
{
    return TrExprPack<Dummy, Conds..., TrExpr<From, On, Cond>>{};
}

template<class... Conds, class From, class On, class Cond>
constexpr auto operator||(TrExpr<From, On, Cond>, TrExprPack<Dummy, Conds...>) noexcept
{
    return TrExprPack<Dummy, TrExpr<From, On, Cond>, Conds...>{};
}

template<class... Conds1, class... Conds2>
constexpr auto operator||(TrExprPack<Dummy, Conds1...>, TrExprPack<Dummy, Conds2...> ) noexcept
{
    return TrExprPack<Dummy, Conds1..., Conds2...>{};
}

template<class... Events, class Impl, class... CondPreds>
constexpr auto operator&&(On<Events...>, If<Impl, CondPreds...>) noexcept
{
    return EvExprPack<Dummy, EvExpr<Pack<Events...>, If<Impl, CondPreds...>>>{};
}

template<class... EvExprs1, class... EvExprs2>
constexpr auto operator||(EvExprPack<Dummy, EvExprs1...>, EvExprPack<Dummy, EvExprs2...>) noexcept
{
    return EvExprPack<Dummy, EvExprs1..., EvExprs2...>{};
}

template<class... Events, class... EvExprs>
constexpr auto operator||(On<Events...>, EvExprPack<Dummy, EvExprs...> pack) noexcept
{
    return EvExprPack<Dummy, EvExpr<Pack<Events...>, Dummy>>{} || pack;
}

template<class... Events, class... EvExprs>
constexpr auto operator||(EvExprPack<Dummy, EvExprs...> pack, On<Events...> on) noexcept
{
    return on || pack;
}

template<class State, class Transitions>
struct Table;

template<class T, class Transitions>
struct MakeTransitionTable;

template<class T, class TrPack, class... TrPacks>
struct MakeTransitionTable<T, Pack<TrPack, TrPacks...>>
{
    using State = typename TrPack::State;
    using StateEnum = typename State::Enum;
    using Transitions = typename ExpandPacks<StateEnum, TrPack, TrPacks...>::Type;
    using Type = Table<StateEnum, Transitions>;
};

template<class T, class Table>
using MakeTransitionTableT = typename MakeTransitionTable<T, Table>::Type;

template<class Table, class EvExprs, class Impl>
class EventProcessor;

template<class StateEnumType, class... Transitions, class... EvExprPacks, class Impl>
class EventProcessor<Table<StateEnumType, Pack<Transitions...>>, Pack<EvExprPacks...>, Impl>
{
public:
    using StateEnum = StateEnumType;

public:
    EventProcessor(StateEnum startState, Impl& impl) noexcept
        : m_state(startState)
        , m_impl(&impl)
    {}

    template<class Event>
    void ProcessEvent(const Event& e)
    {
        static_cast<void>(e);

        using PossibleEventsExpressions = FilterByEvent<Event, EvExprPacks...>;
        if constexpr(PossibleEventsExpressions::Size > 0)
        {
            CallEventActions(e, PossibleEventsExpressions{});
        }

        using PossibleTransitions = FilterByEvent<Event, Transitions...>;
        if constexpr(PossibleTransitions::Size > 0)
        {
            ProcessTransitions(e, PossibleTransitions{});
        }
    }

    StateEnum GetState() const noexcept
    {
        return m_state;
    }

private:
    template<class Event, class... EvPacks>
    void CallEventActions(const Event& e, Pack<EvPacks...>)
    {
        static_cast<void>((EvPacks::Dispatch(*m_impl, e) || ...));
    }

    template<class Event, class Tr, class... Trs>
    void ProcessTransitions(const Event& e, Pack<Tr, Trs...>)
    {
        using From = typename Tr::FromState;
        using To = typename Tr::ToState;

        if (m_state == From::EnumValue && Tr::IsAllowed(*m_impl))
        {
            m_state = To::EnumValue;
            Tr::OnLeave(*m_impl, e);
            Tr::OnEnter(*m_impl, e);
            return;
        }

        if constexpr(sizeof...(Trs) != 0)
        {
            ProcessTransitions(e, Pack<Trs...>{});
        }
    }

private:
    StateEnum m_state;
    Impl* m_impl;
};

}//detail

template<class Impl>
struct StatefulObject
{
    template<auto V>
    struct State
    {
        using Enum = decltype(V);
        static constexpr auto EnumValue{ V };
    };

    template<class... FromStates>
    static constexpr detail::From<FromStates...> From{};

    template<class... OnEvents>
    static constexpr detail::On<OnEvents...> On{};

    template<class ToState>
    static constexpr detail::To<ToState> To{};

    template<class... Preds>
    static constexpr detail::If<Impl, Preds...> If{};

    template<class... Preds>
    using Any = detail::Any<Impl, Preds...>;

    template<class... Preds>
    using All = detail::All<Impl, Preds...>;

    template<class... Preds>
    using None = detail::None<Impl, Preds...>;

    template<class Pred>
    using Not = detail::Not<Impl, Pred>;

    template<class... Preds>
    static constexpr detail::If<Impl, Any<Preds...>> IfAny{};

    template<class... Preds>
    static constexpr detail::If<Impl, All<Preds...>> IfAll{};

    template<class... Preds>
    static constexpr detail::If<Impl, None<Preds...>> IfNone{};

    template<class Pred>
    static constexpr detail::If<Impl, Not<Pred>> IfNot{};

    template<class... Preds>
    static constexpr detail::Do<Impl, Preds...> Do{};
};

template<class Impl>
class StateMachine
{
    template<class T, class = void>
    struct EvExprs{ using Type = detail::Pack<>; };

    template<class T>
    struct EvExprs<T, std::void_t<decltype(T::ActionRules)>>
    {
        using Type = std::decay_t<decltype(T::ActionRules)>;
    };

    using Processor = detail::EventProcessor<
        detail::MakeTransitionTableT<Impl, std::decay_t<decltype(Impl::Table)>>,
        typename EvExprs<Impl>::Type,
        Impl>;

    using StateEnum = typename Processor::StateEnum;
    static_assert(std::is_enum_v<StateEnum>);

public:
    StateMachine(StateEnum startState, Impl& impl) noexcept
        : m_processor(startState, impl) {}

    template<class Event>
    void ProcessEvent(const Event& e)
    {
        m_processor.template ProcessEvent<Event>(e);
    }

    StateEnum GetState() const noexcept
    {
        return m_processor.GetState();
    }

private:
    Processor m_processor;
};

template<class... T>
constexpr auto MakeTransitions(T&&...) noexcept
{
    return detail::Pack<std::decay_t<T>...>{};
}

template<class... T>
constexpr auto MakeActionRules(T&&...) noexcept
{
    return detail::Pack<std::decay_t<T>...>{};
}

}// csm

#endif // CSM_STATE_MACHINE
