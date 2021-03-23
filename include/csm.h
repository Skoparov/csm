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
struct HasDupsT;

template<class T, class... Ts>
struct HasDupsT<T, Ts...> : std::conditional_t<
    (std::is_same_v<std::decay_t<T>, std::decay_t<Ts>> || ...),
    std::true_type,
    HasDupsT<Ts...>>
{};

template<>
struct HasDupsT<> : std::false_type {};

template<auto... V>
struct HasDupsV;

template<auto V, auto... Vs>
struct HasDupsV<V, Vs...> : std::conditional_t<
    ((V == Vs) || ...),
    std::true_type,
    HasDupsV<Vs...>>
{};

template<>
struct HasDupsV<> : std::false_type {};

template<class... Ts>
struct Pack
{
    static_assert (!HasDupsT<Ts...>::value);
    static constexpr size_t Size{ sizeof...(Ts) };

    template<class T>
    static constexpr bool Contains{ (std::is_same_v<T, Ts> || ...) };
};

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
    static_assert (!HasDupsT<Ts...>::value);
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

template<class Impl, class... Preds>
struct Do : TypesCheck<Preds...>
{
    template<class Event>
    void operator()(Impl& impl, const Event& e)
    {
        //static_assert((std::is_invocable_r_v<void, Preds, Impl&, const Event&> && ...));
        (Preds{}(impl, e), ...);
    }
};

template<class Events, class Cond>
struct EvExpr;

template<class Action, class EvExpr, class... EvExprs>
struct EvExprPack;

template<class... Events>
struct On : Pack<Events...>
{
    template<class Impl, class... Preds>
    constexpr auto operator=(Do<Impl, Preds...>) const noexcept
    {
        return EvExprPack<Do<Impl, Preds...>, EvExpr<Pack<Events...>, Dummy>>{};
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

    static_assert((std::is_same_v<Enum, typename States::Enum> && ... && true));

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
        if constexpr(IsInitalized<Cond>)
        {
            return Cond{}(impl);
        }
        else
        {
            static_cast<void>(impl);
        }

        return true;
    }
};

template<class Action, class EvExpr, class... EvExprs>
struct EvExprPack
{
    template<class Impl, class... Preds>
    constexpr auto operator=(Do<Impl, Preds...>) const noexcept
    {
        return EvExprPack<Do<Impl, Preds...>, EvExpr, EvExprs...>{};
    }

    template<class Impl, class Event>
    static constexpr bool Dispatch(Impl& impl, const Event& e)
    {
        static_assert(IsInitalized<Action>);
        if constexpr (EvExpr::template Contains<Event> ||
                     (EvExprs:: template Contains<Event> || ...))
        {
            if (EvExpr::template IsAllowed<Event>(impl) ||
               (EvExprs::template IsAllowed<Event>(impl) || ...))
            {
                Action{}(impl, e);
                return true;
            }
        }
        else
        {
            static_cast<void>(e);
            static_cast<void>(impl);
        }

        return false;
    }
};

template<class... Events, class Cond>
struct EvExpr<Pack<Events...>, Cond> : AllowedOn<Cond>, Pack<Events...>
{
    template<class Event, class Impl>
    static bool IsAllowed(Impl& impl)
    {
        return AllowedOn<Cond>::IsAllowed(impl);
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

    template<class Impl, class Event>
    static void OnEnter(Impl& impl, const Event& e)
    {
        if constexpr(HasOnEnterV<To, From::EnumValue, Impl, Event>)
        {
            To{}.template OnEnter<From::EnumValue>(impl, e);
        }
        else
        {
            static_cast<void>(impl);
            static_cast<void>(e);
        }
    }

    template<class Impl, class Event>
    static void OnLeave(Impl& impl, const Event& e)
    {
        if constexpr(HasOnLeaveV<From, To::EnumValue, Impl, Event>)
        {
            From{}.template OnLeave<To::EnumValue>(impl, e);
        }
        else
        {
            static_cast<void>(impl);
            static_cast<void>(e);
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

template<class Event, class Transition, class... Transitions, class... FittingTransitions>
constexpr auto GetPossibleTransitions(Pack<FittingTransitions...> = Pack<>{}) noexcept
{
    using Events = typename Transition::OnEvents;
    using Result = std::conditional_t<
        Events::template Contains<Event>,
        Pack<Transition, FittingTransitions...>,
        Pack<FittingTransitions...>>;

    if constexpr(sizeof...(Transitions) > 0)
    {
        return GetPossibleTransitions<Event, Transitions...>(Result{});
    }
    else
    {
        return Result{};
    }
}

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
constexpr auto operator||(TrExpr<From, On, Cond> expr, TrExprPack<Dummy, Conds...> pack) noexcept
{
    return pack || expr;
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
        , m_impl(impl)
    {}

    template<class Event>
    bool ProcessEvent(const Event& e)
    {
        static_cast<void>(e);
        static_cast<void>((EvExprPacks::Dispatch(m_impl, e) || ...));

        constexpr auto PossibleTransitions{ GetPossibleTransitions<Event, Transitions...>() };
        if constexpr(PossibleTransitions.Size > 0)
        {
            ProcessEvent(PossibleTransitions, e);
        }

        return true;
    }

    StateEnum GetState() const noexcept
    {
        return m_state;
    }

private:
    template<class Event, class Tr, class... Trs>
    void ProcessEvent(Pack<Tr, Trs...>, const Event& e)
    {
        using From = typename Tr::FromState;
        using To = typename Tr::ToState;

        if (m_state == From::EnumValue && Tr::IsAllowed(m_impl))
        {
            m_state = To::EnumValue;
            Tr::OnLeave(m_impl, e);
            Tr::OnEnter(m_impl, e);
            return;
        }

        if constexpr(sizeof...(Trs) != 0)
        {
            ProcessEvent(Pack<Trs...>{}, e);
        }
    }

private:
    StateEnum m_state;
    Impl& m_impl;
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
