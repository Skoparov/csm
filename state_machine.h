#ifndef CP_STATE_MACHINE
#define CP_STATE_MACHINE
 
#include <type_traits>
 
namespace state_machine {
namespace detail {
 
template<class... Transitions>
struct TypePack;
 
template<class T, T... Values>
struct ValuePack;
 
template<class T, T Target, class Pack>
struct ContainsValue : std::false_type {};
 
template<class T, T Target, T Curr, T... Values>
struct ContainsValue<T, Target, ValuePack<T, Curr, Values...>> : std::conditional_t<
    Target == Curr, std::true_type, ContainsValue<T, Target, ValuePack<T, Values...>>> {};
 
template<class>
struct IsValuePack : std::false_type{};
 
template<class T, T... Values>
struct IsValuePack<ValuePack<T, Values...>> : std::true_type{};
 
template<class State, class FromStatesList, State To, class EventsList>
struct TransitionPack
{
    using OnEvents = EventsList;
    using FromStates = FromStatesList;
    static constexpr State ToState{ To };
 
    static_assert(IsValuePack<EventsList>::value, "Use OnEvents<> pack to describe events");
    static_assert(IsValuePack<FromStates>::value, "Use FromStates<> pack to describe source states");
};
 
template<class State, State From, State To, class EventsList>
struct Transition
{
    using OnEvents = EventsList;
    static constexpr State FromState{ From };
    static constexpr State ToState{ To };
};
 
template<class T, class U>
struct MergePacks;
 
template<class... Pack1, class... Pack2>
struct MergePacks<TypePack<Pack1...>, TypePack<Pack2...>>
{
    using type = TypePack<Pack1..., Pack2...>;
};
 
template<class State, class FromStatesList, State ToState, class OnEvents>
struct MakeTransitionsForEachFromState;
 
template<class State, State... States, State ToState, class OnEvents>
struct MakeTransitionsForEachFromState<State, ValuePack<State, States...>, ToState, OnEvents>
{
    using type = TypePack<Transition<State, States, ToState, OnEvents>...>;
};
 
template<class State, class T>
struct TransitionTableExpander;
 
template<class State, class TransitionPack, class... TransitionPacks>
struct TransitionTableExpander<State, TypePack<TransitionPack, TransitionPacks...>>
{
    using type = typename MergePacks<
        typename MakeTransitionsForEachFromState
        <
            State,
            typename TransitionPack::FromStates,
            TransitionPack::ToState,
            typename TransitionPack::OnEvents
        >::type,
        typename TransitionTableExpander<State, TypePack<TransitionPacks...>>::type>::type;
};
 
template<class State>
struct TransitionTableExpander<State, TypePack<>>
{
    using type = TypePack<>;
};
 
template<class Transitions>
using ExpandTransitions = typename TransitionTableExpander<
    typename Transitions::StateType,
    typename Transitions::Table>::type;
 
template<class T, class StateType, class EventType, class HandlerType>
struct StateMachineImpl;
 
template<class... Transitions, class StateType, class EventType, class HandlerType>
struct StateMachineImpl<TypePack<Transitions...>, StateType, EventType, HandlerType>
{
public:
    StateMachineImpl(HandlerType& handler, StateType startState) noexcept
        : m_state(startState)
        , m_handler(handler)
    {}
 
    template<EventType Event>
    void ProcessEvent()
    {
        ProcessEventImpl<Event, Transitions...>();
    }
 
    StateType GetState() const noexcept
    {
        return m_state;
    }
 
private:
    template<EventType Event, class Transition, class... RemainingTransitions>
    void ProcessEventImpl()
    {
        if (TransitionFits<Transition, Event>())
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
            ProcessEventImpl<Event, RemainingTransitions...>();
        }
    }
 
    template<EventType Event, class... RemainingTransitions>
    std::enable_if_t<sizeof...(RemainingTransitions) == 0> ProcessEventImpl() noexcept {}
 
    template<StateType From, StateType To, class Handler>
    static auto OnTransition(Handler& handler) -> decltype(handler.template OnTransition<From, To>(), void())
    {
        static_cast<void>(handler.template OnTransition<From, To>());
    }
 
    template<StateType, StateType>
    static void OnTransition(...) noexcept{}
 
    template<StateType State, class Handler>
    static auto OnEnterState(Handler& handler) -> decltype(handler.template OnEnterState<State>(), void())
    {
        static_cast<void>(handler.template OnEnterState<State>());
    }
 
    template<StateType>
    static void OnEnterState(...) noexcept {}
 
    template<StateType State, class Handler>
    static auto OnLeaveState(Handler& handler) -> decltype(handler.template OnLeaveState<State>(), void())
    {
        static_cast<void>(handler.template OnLeaveState<State>());
    }
 
    template<StateType>
    static void OnLeaveState(...) noexcept {}
 
    template<class Transition, EventType Event>
    bool TransitionFits() const noexcept
    {
        return
            m_state == Transition::FromState &&
            ContainsValue<EventType, Event, typename Transition::OnEvents>::value;
    }
 
private:
    StateType m_state;
    HandlerType& m_handler;
};
 
}// detail
 
template<class... Transitions>
using TransitionTable = detail::TypePack<Transitions...>;
 
template<class State, class Event>
struct TransitionsInfoBase
{
    using StateType = State;
    using EventType = Event;
 
    template<State... States>
    using FromStates = detail::ValuePack<State, States...>;
 
    template<Event... Events>
    using OnEvents = detail::ValuePack<Event, Events...>;
 
    template<State To, class FromStatesList, class OnEvents>
    using TransitionTo = detail::TransitionPack<State, FromStatesList, To, OnEvents>;
};
 
template<class TransitionsInfo, class Handler>
using StateMachine = detail::StateMachineImpl<
    detail::ExpandTransitions<TransitionsInfo>,
    typename TransitionsInfo::StateType,
    typename TransitionsInfo::EventType,
    Handler>;
 
}// state_machine
 
#endif // CP_STATE_MACHINE