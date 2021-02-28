#include <csm.h>
#include <iostream>

// Law abiding npc state machine example

enum State{ Friendly, Neutral, Disgruntled, Hostile };
enum Event{ Compliment, Insult, Bribe, Shove, Attack, PleadForMercy };

/* Regular transition table
Friendly + Insult = Disgruntled
Friendly + Bribe = Disgruntled
Friendly + Shove = Disgruntled
Neutral + Insult = Disgruntled
Neutral + Bribe = Disgruntled
Neutral + Shove = Disgruntled
Hostile + PleadForMercy = Disgruntled
Disgruntled + Insult = Hostile
Disgruntled + Bribe = Hostile
Disgruntled + Shove = Hostile
Friendly + Attacked = Hostile
Neutral + Attacked = Hostile
Disgruntled + Attacked = Hostile
Neutral + Compliment = Friendly
Disgruntled + Compliment = Friendly
 */

struct LawAbidingNpcTransitionTable : csm::TransitionTableBase<State, Event>
{
    static constexpr auto Rules{ csm::MakeTransitionRules(
        (From<Friendly, Neutral> + On<Insult, Bribe, Shove>) ||
        (From<Hostile> + On<PleadForMercy>)
            = To<State::Disgruntled>,
        (From<Disgruntled> + On<Insult, Bribe, Shove>) ||
        (From<Friendly, Neutral, Disgruntled> + On<Attack>)
            = To<Hostile>,
        From<Neutral, Disgruntled> + On<Compliment>
            = To<State::Friendly>
    )};
};

class LawAbidingNpc
{
public:
    template<State From, State To, std::enable_if_t<
                 From == State::Disgruntled && To == State::Friendly>...>
    void OnTransition()
    {
        std::cout << "Hi new friend :)\n";
    }

    template<State S, std::enable_if_t<S == State::Hostile>...>
    void OnEnterState()
    {
        std::cerr << "Never should've come here!!\n";
    }

    template<State S, std::enable_if_t<S == State::Friendly>...>
    void OnLeaveState()
    {
        std::cout << "But I thought we were friends :(\n";
    }

    template<State S, std::enable_if_t<S == State::Hostile>...>
    void OnLeaveState()
    {
        std::cout << "I'll spare you..this time\n";
        ++m_fooledTimes;
    }

private:
    size_t m_fooledTimes{ 0 };
};

using LawAbidingNpcStateMachine = csm::StateMachine<LawAbidingNpcTransitionTable, LawAbidingNpc>;

int main()
{
    LawAbidingNpc npc;
    LawAbidingNpcStateMachine sm{ State::Friendly, &npc };

    // Friendly -> Disgruntled, says "But I thought we were friends :("
    sm.ProcessEvent<Event::Insult>();

    // Disgruntled -> Hostile, says "Never should've come here!!"
    sm.ProcessEvent<Event::Shove>();

    // Hostile -> Disgruntled, says "I'll spare you..this time"
    sm.ProcessEvent<Event::PleadForMercy>();

    // Disgruntled -> Friendly, says "Hi new friend :)"
    sm.ProcessEvent<Event::Compliment>();
}
