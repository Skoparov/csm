#ifndef CSM_STATE_MACHINE
#define CSM_STATE_MACHINE

#include <cstddef>
#include <type_traits>
#include <utility>

namespace csm {
namespace detail {

struct Dummy{};

template<class T>
constexpr bool IsInitalized{ !std::is_same_v<T, Dummy> };

template<class T>
constexpr decltype(std::declval<T>()) As() noexcept;

template<class T, auto To, class Object, class E, class = void>
struct HasOnEnter : std::false_type {};

template<class T, auto To, class Object, class E>
struct HasOnEnter<T, To, Object, E, std::void_t<
    decltype(As<T>().template OnEnter<To>(As<Object&>(), As<E>()))>>
    : std::true_type
{
    using R = decltype(As<T>().template OnEnter<To>(As<Object&>(), As<E>()));
    static_assert(std::is_same_v<R, void>, "OnEnter() should return void");
};

template<class T, auto To, class Object, class E>
constexpr bool HasOnEnterV{ HasOnEnter<T, To, Object, E>::value };

template<class T, auto To, class Object, class E, class = void>
struct HasOnLeave : std::false_type {};

template<class T, auto To, class Object, class E>
struct HasOnLeave<T, To, Object, E, std::void_t<
    decltype(As<T>().template OnLeave<To>(As<Object&>(), As<E>()))>>
    : std::true_type
{
    using R = decltype(As<T>().template OnLeave<To>(As<Object&>(), As<E>()));
    static_assert(std::is_same_v<R, void>, "OnLeave() should return void");
};

template<class T, auto To, class Object, class E>
constexpr bool HasOnLeaveV{ HasOnLeave<T, To, Object, E>::value };

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
    static_assert (!HasDups<Ts...>::value,
        "State/event/action packs should not contain duplicates");

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
struct From : Pack<State, States...>
{
    using Type = typename State::Enum;
    static_assert((std::is_same_v<Type, typename States::Enum> && ...),
        "All states should use the same state enum");
};

template<class... Ts>
struct TypesCheck
{
    static_assert((std::is_empty_v<Ts> && ...),
        "Guards/actions should be empty sturctures");

    static_assert(sizeof...(Ts) > 0,
        "Empty packs of guards/actions not allowed");

    static_assert(!HasDups<Ts...>::value,
        "Packs of guards/actions should not contain duplicates");
};

template<class T>
struct GuardChecker;

template<template<class...> class T, class... Preds>
struct GuardChecker<T<Preds...>> : TypesCheck<Preds...>
{
    template<class Object>
    bool operator()(const Object& obj)
    {
        static_assert((std::is_invocable_r_v<bool, Preds, const Object&> && ...),
            "Guards should implement bool operator()(const Object&)");

        return T<Preds...>::Check(obj);
    }
};

template<class... Preds>
struct If : GuardChecker<If<Preds...>>
{
    template<class Object>
    static bool Check(const Object& obj)
    {
        return (Preds{}(obj) && ...);
    }
};

template<class... Preds>
struct Any : GuardChecker<Any<Preds...>>
{
    template<class Object>
    static bool Check(const Object& obj)
    {
        return (Preds{}(obj) || ...);
    }
};

template<class... Preds>
struct All : GuardChecker<All<Preds...>>
{
    template<class Object>
    static bool Check(const Object& obj)
    {
        return (Preds{}(obj) && ...);
    }
};

template<class... Preds>
struct None : GuardChecker<None<Preds...>>
{
    template<class Object>
    static bool Check(const Object& obj)
    {
        return (!Preds{}(obj) && ...);
    }
};

template<class Pred>
struct Not : GuardChecker<Not<Pred>>
{
    template<class Object>
    static bool Check(const Object& obj)
    {
        return !Pred{}(obj);
    }
};

template<class... Actions>
struct Do : TypesCheck<Actions...>
{
    template<class Object, class Event>
    void operator()(Object& obj, const Event& e)
    {
        static_assert(
            (std::is_invocable_r_v<void, Actions, Object&, const Event&> && ...),
            "Actions should implement void operator()(Object&, const Event&)");

        (Actions{}(obj, e), ...);
    }
};

template<class Action, class... ActRules>
struct ActRulePack;

template<class Events, class Cond>
struct ActRule;

template<class... Events>
struct On : Pack<Events...>
{
    template<class... Actions>
    constexpr auto operator=(Do<Actions...>) const noexcept
    {
        return ActRulePack<Do<Actions...>, ActRule<Pack<Events...>, Dummy>>{};
    }
};

template<class State>
struct To {};

template<class Cond>
struct AllowedOn
{
    template<class Object>
    static bool IsAllowed(const Object& obj)
    {
        static_cast<void>(obj);
        if constexpr(IsInitalized<Cond>)
        {
            return Cond{}(obj);
        }

        return true;
    }
};

template<class ToState, class... TrRules>
struct TrRulePack
{
    static_assert(sizeof...(TrRules) > 0,
        "Transition rule pack should contain at least one rule");

    using State = ToState;

    template<class State>
    constexpr auto operator=(To<State>) const noexcept
    {
        static_assert(!IsInitalized<ToState>);
        return TrRulePack<State, TrRules...>{};
    }
};

template<class FromStates, class OnEvents, class CondPred>
struct TrRule;

template<class State, class... States, class... Events, class CondPred>
struct TrRule<From<State, States...>, On<Events...>, CondPred>
{
    using Enum = typename State::Enum;
    using Cond = CondPred;

    static_assert((std::is_same_v<Enum, typename States::Enum> && ...),
        "All states should use the smae state enum");

    template<class ToState>
    constexpr auto operator=(To<ToState>) const noexcept
    {
        return TrRulePack<ToState, TrRule<From<State, States...>, On<Events...>, Cond>>{};
    }
};

template<class Action, class... ActRules>
struct ActRulePack
{
    static_assert(sizeof...(ActRules) > 0,
        "Action rule pack should contain at least one rule");

    template<class Event>
    static constexpr bool ContainsEvent{
        (ActRules:: template ContainsEvent<Event> || ...) };

    template<class... Actions>
    constexpr auto operator=(Do<Actions...>) const noexcept
    {
        return ActRulePack<Do<Actions...>, ActRules...>{};
    }

    template<class Object, class Event>
    static bool Dispatch(Object& obj, const Event& e)
    {
        static_assert(IsInitalized<Action>);
        using ActRulesWithEvent = FilterByEvent<Event, ActRules...>;
        return DispatchFiltered(obj, e, ActRulesWithEvent{});
    }

private:
    template<class Object, class Event, class... Rules>
    static bool DispatchFiltered(Object& obj, const Event& e, Pack<Rules...>)
    {
        return (Rules::template Dispatch<Action>(obj, e) || ...);
    }
};

template<class... Events, class Cond>
struct ActRule<Pack<Events...>, Cond>
{
    template<class Event>
    static constexpr bool ContainsEvent{ Pack<Events...>::template Contains<Event> };

    template<class Action, class Object, class Event>
    static bool Dispatch(Object& obj, const Event& e)
    {
        if (AllowedOn<Cond>::IsAllowed(obj))
        {
            Action{}(obj, e);
            return true;
        }

        return false;
    }
};

template<class From, class To, class Events, class Cond>
struct Transition : AllowedOn<Cond>
{
    static_assert(std::is_same_v<typename From::Enum, typename To::Enum>,
        "All states should use the same state enum");

    static_assert(From::EnumValue != To::EnumValue,
        "Source and target state should not be the same");

    using StateEnum = typename From::Enum;

    template<class Event>
    static constexpr bool ContainsEvent{ Events::template Contains<Event> };

    template<class Object, class Event>
    static bool Dispatch(Object& obj, const Event& e, StateEnum& currState)
    {
        static_cast<void>(obj);
        static_cast<void>(e);

        if (currState == From::EnumValue && AllowedOn<Cond>::IsAllowed(obj))
        {
            if constexpr(HasOnLeaveV<From, To::EnumValue, Object, Event>)
            {
                From{}.template OnLeave<To::EnumValue>(obj, e);
            }

            currState = To::EnumValue;

            if constexpr(HasOnEnterV<To, From::EnumValue, std::decay_t<Object>, Event>)
            {
                To{}.template OnEnter<From::EnumValue>(obj, e);
            }

            return true;
        }

        return false;
    }
};

template<class StateEnum, class TrRulePack>
struct IsValidTrRulePack;

template<class StateEnum, class To, class... TrRules>
struct IsValidTrRulePack<StateEnum, TrRulePack<To, TrRules...>> :
    std::conjunction<
        std::is_same<StateEnum, typename To::Enum>,
        std::conjunction<std::is_same<StateEnum, typename TrRules::Enum>...>> {};

template<class To, class If>
struct Expand;

template<class To, class... FromStates, class... Events, class CondPred>
struct Expand<To, TrRule<From<FromStates...>, On<Events...>, CondPred>>
{
    using Type = Pack<Transition<FromStates, To, Pack<Events...>, CondPred>...>;
};

template<class T>
struct ExpandPack;

template<class To, class... IfPack>
struct ExpandPack<TrRulePack<To, IfPack...>>
{
    using Type = MergeT<typename Expand<To, IfPack>::Type...>;
};

template<class StateEnum, class... Packs>
struct ExpandPacks
{
    static_assert((IsValidTrRulePack<StateEnum, Packs>::value && ...),
        "All states should use the same state enum");

    using Type = MergeT<typename ExpandPack<Packs>::Type...>;
};

template<class obj>
struct MakeTransitions;

template<class TrPack, class... TrPacks>
struct MakeTransitions<Pack<TrPack, TrPacks...>>
{
    using State = typename TrPack::State;
    using StateEnum = typename State::Enum;
    using Type = typename ExpandPacks<StateEnum, TrPack, TrPacks...>::Type;
};

template<class T>
using MakeTransitionsT = typename MakeTransitions<T>::Type;

template<class... States, class... Events>
constexpr auto operator&&(From<States...>, On<Events...>) noexcept
{
    return TrRule<From<States...>, On<Events...>, Dummy>{};
}

template<class From, class On, class... CondPreds>
constexpr auto operator&&(TrRule<From, On, Dummy>, If<CondPreds...>) noexcept
{
    return TrRulePack<Dummy, TrRule<From, On, If<CondPreds...>>>{};
}

template<class From1, class On1, class Cond1, class From2, class On2, class Cond2>
constexpr auto operator||(TrRule<From1, On1, Cond1>, TrRule<From2, On2, Cond2>) noexcept
{
    return TrRulePack<Dummy, TrRule<From1, On1, Cond1>, TrRule<From2, On2, Cond2>>{};
}

template<class... Conds, class From, class On, class Cond>
constexpr auto operator||(TrRulePack<Dummy, Conds...>, TrRule<From, On, Cond>) noexcept
{
    return TrRulePack<Dummy, Conds..., TrRule<From, On, Cond>>{};
}

template<class... Conds, class From, class On, class Cond>
constexpr auto operator||(TrRule<From, On, Cond>, TrRulePack<Dummy, Conds...>) noexcept
{
    return TrRulePack<Dummy, TrRule<From, On, Cond>, Conds...>{};
}

template<class... Conds1, class... Conds2>
constexpr auto operator||(TrRulePack<Dummy, Conds1...>, TrRulePack<Dummy, Conds2...> ) noexcept
{
    return TrRulePack<Dummy, Conds1..., Conds2...>{};
}

template<class... Events, class... CondPreds>
constexpr auto operator&&(On<Events...>, If<CondPreds...>) noexcept
{
    return ActRulePack<Dummy, ActRule<Pack<Events...>, If<CondPreds...>>>{};
}

template<class... ActRules1, class... ActRules2>
constexpr auto operator||(ActRulePack<Dummy, ActRules1...>, ActRulePack<Dummy, ActRules2...>) noexcept
{
    return ActRulePack<Dummy, ActRules1..., ActRules2...>{};
}

template<class... Events, class... ActRules>
constexpr auto operator||(On<Events...>, ActRulePack<Dummy, ActRules...> pack) noexcept
{
    return ActRulePack<Dummy, ActRule<Pack<Events...>, Dummy>>{} || pack;
}

template<class... Events, class... ActRules>
constexpr auto operator||(ActRulePack<Dummy, ActRules...> pack, On<Events...> on) noexcept
{
    return on || pack;
}

}//detail

template<class StateEnum>
struct SyntaxDefinitions
{
    static_assert (std::is_enum_v<StateEnum>,
        "External states should be declared as enums");

    template<StateEnum V>
    struct State
    {
        using Enum = StateEnum;
        static constexpr StateEnum EnumValue{ V };
    };

    template<class... FromStates>
    static constexpr detail::From<FromStates...> From{};

    template<class... OnEvents>
    static constexpr detail::On<OnEvents...> On{};

    template<class ToState>
    static constexpr detail::To<ToState> To{};

    template<class... Preds>
    static constexpr detail::If<Preds...> If{};

    template<class... Preds>
    using Any = detail::Any<Preds...>;

    template<class... Preds>
    using All = detail::All<Preds...>;

    template<class... Preds>
    using None = detail::None<Preds...>;

    template<class Pred>
    using Not = detail::Not<Pred>;

    template<class... Preds>
    static constexpr detail::If<Any<Preds...>> IfAny{};

    template<class... Preds>
    static constexpr detail::If<None<Preds...>> IfNone{};

    template<class Pred>
    static constexpr detail::If<Not<Pred>> IfNot{};

    template<class... Preds>
    static constexpr detail::Do<Preds...> Do{};
};

namespace tags
{

struct NoSyntaxDefinitions;

}// tags

template<class Object, class StateEnum, class... Tags>
class StateMachine : public
    std::conditional_t<
        detail::Pack<Tags...>::template Contains<tags::NoSyntaxDefinitions>,
        detail::Dummy,
        SyntaxDefinitions<StateEnum>>
{
    static_assert (std::is_enum_v<StateEnum>,
        "External states should be declared as enums");

public:
    explicit StateMachine(StateEnum startState) noexcept
        : m_state(startState)
    {}

    template<class Event>
    void ProcessEvent(const Event& e)
    {
        using UnexpandedTable = std::decay_t<decltype(Object::TransitionRules)>;
        using TransitionTable = detail::MakeTransitionsT<UnexpandedTable>;
        using ActRulesTable = typename MakeActionRules<Object>::Type;

        ProcessEventInternal(e, TransitionTable{}, ActRulesTable{});
    }

    StateEnum GetState() const noexcept
    {
        return m_state;
    }

private:
    template<class Event, class... Transitions, class... ActionRules>
    void ProcessEventInternal(
            const Event& e,
            detail::Pack<Transitions...>,
            detail::Pack<ActionRules...>)
    {
        static_cast<void>(e);

        using PossibleActionRules = detail::FilterByEvent<Event, ActionRules...>;
        if constexpr(PossibleActionRules::Size > 0)
        {
            CallEventActions(e, PossibleActionRules{});
        }

        using PossibleTransitions = detail::FilterByEvent<Event, Transitions...>;
        if constexpr(PossibleTransitions::Size > 0)
        {
            ProcessTransitions(e, PossibleTransitions{});
        }
    }

    template<class Event, class... ActRules>
    void CallEventActions(const Event& e, detail::Pack<ActRules...>)
    {
        Object& obj{ static_cast<Object&>(*this) };
        static_cast<void>((ActRules::Dispatch(obj, e) || ...));
    }

    template<class Event, class... Transitions>
    void ProcessTransitions(const Event& e, detail::Pack<Transitions...>)
    {
        Object& obj{ static_cast<Object&>(*this) };
        static_cast<void>((Transitions::Dispatch(obj, e, m_state) || ...));
    }

private:
    template<class T, class = void>
    struct MakeActionRules{ using Type = detail::Pack<>; };

    template<class T>
    struct MakeActionRules<T, std::void_t<decltype(T::ActionRules)>>
    {
        using Type = std::decay_t<decltype(T::ActionRules)>;
    };

private:
    StateEnum m_state;
};

template<class... TransitionRules>
constexpr auto MakeTransitionRules(TransitionRules&&...) noexcept
{
    return detail::Pack<std::decay_t<TransitionRules>...>{};
}

template<class... ActionRules>
constexpr auto MakeActionRules(ActionRules&&...) noexcept
{
    return detail::Pack<std::decay_t<ActionRules>...>{};
}

}// csm

#endif // CSM_STATE_MACHINE
