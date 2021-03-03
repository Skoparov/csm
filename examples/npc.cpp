#include <csm.h>
#include <cassert>
#include <iostream>

// Npc state machine example

enum State{ Friendly, Neutral, Disgruntled, Hostile, Dead };
enum Event{ Compliment, Insult, Bribe, Shove, Attack, PleadForMercy };

/* Regular transition table for comparison
Neutral + Compliment = Friendly
Disgruntled + Compliment = Friendly
Neutral + Bribe = Friendly
Disgruntled + Bribe = Friendly
Friendly + Insult = Neutral
Friendly + Shove = Neutral
Disgruntled + Compliment = Neutral
Disgruntled + Bribe = Neutral
Friendly + Insult = Disgruntled
Friendly + Shove = Disgruntled
Neutral + Insult = Disgruntled
Neutral + Shove = Disgruntled
Hostile + PleadForMercy = Disgruntled
Disgruntled + Insult = Hostile
Disgruntled + Shove = Hostile
Friendly + Attack = Hostile
Neutral + Attack = Hostile
Disgruntled + Attack = Hostile
Friendly + Attack = Dead
Neutral + Attack = Dead
Disgruntled + Attack = Dead
Hostile + Attack = Dead
 */

struct NpcTransitionTable : csm::TransitionTableBase<State, Event>
{
    static constexpr auto PossibleTransition{ csm::MakePossibleTransitions(
        From<Neutral, Disgruntled> + On<Compliment, Bribe>
            = To<Friendly>,
        (From<Friendly> + On<Insult, Shove>) ||
        (From<Disgruntled> + On<Compliment, Bribe>)
            = To<Neutral>,
        (From<Friendly, Neutral> + On<Insult, Shove>) ||
        (From<Hostile> + On<PleadForMercy>)
            = To<Disgruntled>,
        (From<Disgruntled> + On<Insult, Shove>) ||
        (From<Friendly, Neutral, Disgruntled> + On<Attack>)
            = To<Hostile>,
        From<Friendly, Neutral, Disgruntled, Hostile> + On<Attack>
            = To<Dead>
    )};
};

// Helpers to make callback more readable
template<State From, State To>
constexpr bool OnSpare{ From == Hostile && To != Dead };

template<State S>
constexpr bool HasPhraseOnEnter{ S == Friendly || S == Disgruntled || S == Hostile || S == Dead };

template<Event E>
constexpr bool IsAttitudeChangeEvent{ E == Compliment || E == Insult || E == Bribe || E == Shove };

template<State To>
constexpr bool NeedTransitionCheck{ To == Friendly || To == Neutral || To == Disgruntled || To == Dead };

class Npc : public csm::StateMachine<NpcTransitionTable, Npc>
{
public:
    Npc(State state, uint8_t maxHealth, uint8_t maxAttitude) noexcept
        : csm::StateMachine<NpcTransitionTable, Npc>(state, this)
        , m_health(maxHealth)
        , m_attitude(maxAttitude)
        , m_maxHealth(maxHealth)
        , m_maxAttitude(maxAttitude){}

public:
    template<State From, State To, std::enable_if_t<OnSpare<From, To>>...>
    void OnTransition()
    {
        std::cout << "I'll spare you..this time\n";
    }

    template<State S, std::enable_if_t<HasPhraseOnEnter<S>>...>
    void OnEnterState()
    {
        switch (S)
        {
        case Friendly: std::cout << "Hi new friend :)\n"; break;
        case Disgruntled: std::cout << "I hate you!\n"; break;
        case Hostile: std::cout << "Never should've come here!!\n";; break;
        case Dead: std::cout << "Arghhh!!!\n"; break;
        default: assert(false); break;
        }

    }

    template<State S, std::enable_if_t<S == Friendly>...>
    void OnLeaveState()
    {
        std::cout << "I thought we were friends :(\n";
    }

    template<Event E, std::enable_if_t<IsAttitudeChangeEvent<E>>...>
    void OnEvent() noexcept
    {
        int8_t attitudeChange{ 0 };
        switch (E)
        {
        case Compliment:
        {
            std::cout << "Oh, thanks!\n";
            attitudeChange = +1;
            break;
        }
        case Bribe:
        {
            std::cout << "Wow, that's very generous!\n";
            attitudeChange = +10;
            break;
        }
        case Insult:
        {
            std::cout << "Why would you say that?!\n";
            attitudeChange = -1;
            break;
        }
        case Shove:
        {
            std::cout << "Stop pushing me!\n";
            attitudeChange = -3;
            break;
        }
        default: assert(false); break;
        }

        ApplyChange(m_attitude, m_maxAttitude, attitudeChange);
    }

    template<Event E, std::enable_if_t<E == Attack>...>
    void OnEvent(uint8_t damage) noexcept
    {
        std::cout << "Ouch!\n";
        m_attitude = 0;
        ApplyChange(m_health, m_maxHealth, -damage);
    }

    template<State From, State To, std::enable_if_t<NeedTransitionCheck<To>>...>
    bool AllowTransition() const noexcept
    {
        switch (To)
        {
        case Friendly: return m_attitude == m_maxAttitude;
        case Neutral: return m_attitude > 0 && m_attitude < m_maxAttitude;
        case Disgruntled: return m_attitude == 0;
        case Dead: return m_health == 0;
        default: assert(false); return false;
        }
    }
    template<Event E, std::enable_if_t<IsAttitudeChangeEvent<E>>...>
    bool AllowEvent() const noexcept
    {
        return GetState() != Hostile;
    }

private:
    void ApplyChange(uint8_t& val, uint8_t max, int8_t change) noexcept
    {
        if (change >= 0)
        {
            val = (max - val >= change)? val + change : max;
        }
        else
        {
            val = (val >= std::abs(change))? val + change : 0;
        }
    }

private:
    uint8_t m_health{ 0 };
    uint8_t m_attitude{ 0 };

    const uint8_t m_maxHealth{ 0 };
    const uint8_t m_maxAttitude{ 0 };
};

int main()
{
    // MaxHealth: 10 | MaxAttitude: 7
    Npc npc{ Friendly, 10 /* maxHealth */, 7 /* maxAttitude */ };

    // Ignored, no transitions or event handlers
    npc.ProcessEvent<PleadForMercy>();

    // Attitude -1 (6)
    // Friendly -> Neutral
    // Says "Why would you say that?!" and "I thought we were friends :("
    npc.ProcessEvent<Insult>();

    // Attitude -3 (3)
    // Says "Stop pushing me!"
    npc.ProcessEvent<Shove>();

    // Attitude -3 (0)
    // Neutral -> Disgruntled
    // Says "Stop pushing me!" and "I hate you!"
    npc.ProcessEvent<Shove>();

    // Attitude +10 (7)
    // Disgruntled -> Friendly
    // Says "Wow, that's very generous!" and "Hi new friend :)"
    npc.ProcessEvent<Bribe>();

    // Health -5 (5)
    // Friendly -> Hostile
    // Says "Ouch!" and "Never should've come here!!"
    npc.ProcessEvent<Attack>(5);

    // Ignored, event not allowed
    npc.ProcessEvent<Bribe>();

    // Hostile -> Disgruntled
    // Says "I'll spare you..this time" and "I hate you!"
    npc.ProcessEvent<PleadForMercy>();

    // Health -7 (0)
    // Hostile -> Dead
    // Says "Ouch!" and "Argh!!!"
    npc.ProcessEvent<Attack>(7);
}
