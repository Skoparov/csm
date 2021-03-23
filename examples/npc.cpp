#include <csm.h>
#include <array>
#include <cassert>
#include <iostream>

// Npc state machine example
// The Npc class implements relatively complex behaviour
// based on two parameters: attitude and health
// Depending on attitude the npc can say different things
// with the state changing accodringly
// Health determines the phisical state of the npc, if it reaches 0 they die.

// Public state enum used by external entities
enum class NpcState{ Friendly, Neutral, Disgruntled, Hostile, Dead };

struct AttitudeChange
{
    int attitudeChange;
};

// Events
struct Compliment : AttitudeChange{};
struct Insult : AttitudeChange{};
struct Bribe : AttitudeChange{};
struct Shove : AttitudeChange{};
struct Attack{ int damage; };
struct PleadForMercy{};
struct Wave{};

class Npc : csm::StatefulObject<Npc>
{
    friend class csm::StateMachine<Npc>;
    using Health = int;
    using Attitude = int;

private: // States
    struct Friendly : State<NpcState::Friendly>
    {
        template<NpcState From, class Event>
        void OnEnter(Npc& /* npc */, const Event& /* e */) const noexcept
        {
            std::cout << "Hi new friend :)\n";
        }

        template<NpcState To, class Event, std::enable_if_t<To != NpcState::Dead>...>
        void OnLeave(Npc& /* npc */, const Event& /* e */) const noexcept
        {
            std::cout << "But I thought we were friends :(\n";
        }
    };

    struct Neutral : State<NpcState::Neutral>{};

    struct Disgruntled : State<NpcState::Disgruntled>
    {
        template<NpcState From, class Event>
        void OnEnter(Npc& npc, const Event& /* e */) noexcept
        {
            if (From == NpcState::Hostile)
            {
                npc.m_attitude = 1;
            }
            else
            {
                std::cout << "I hate you!\n";
            }
        }
    };

    struct Hostile : State<NpcState::Hostile>
    {
        template<NpcState From, class Event>
        void OnEnter(Npc& /* npc */, const Event& /* e */) noexcept
        {
            std::cout << "Never should've come here!\n";
        }
    };

    struct Dead : State<NpcState::Dead>
    {
        template<NpcState From, class Event>
        void OnEnter(Npc& npc, const Event& /* e */) noexcept
        {
            std::cout << "Arghhh!!!\n";
            npc.m_attitude = 0;
        }
    };

private: // Actions
    struct DealDamage
    {
        void operator()(Npc& npc, const Attack& attack)
        {
            npc.m_health = std::max(npc.m_health - attack.damage, 0);
        }
    };

    struct React
    {
        template<class Event>
        void operator()(Npc& /* npc */, const Event& /* event */) const noexcept
        {
            if constexpr(std::is_same_v<Event, Compliment>)
            {
                std::cout << "Oh, thanks!\n";
            }
            else if constexpr(std::is_same_v<Event, Insult>)
            {
                std::cout << "Why would you say that?!\n";
            }
            else if constexpr(std::is_same_v<Event, Bribe>)
            {
                std::cout << "Wow, that's very generous!\n";
            }
            else if constexpr(std::is_same_v<Event, Shove>)
            {
                std::cout << "Stop pushing me!\n";
            }
            else if constexpr(std::is_same_v<Event, PleadForMercy>)
            {
                std::cout << "I'll spare you...this time.\n";
            }
            else if constexpr(std::is_same_v<Event, Attack>)
            {
                std::cout << "Ouch!\n";
            }
            else if constexpr(std::is_same_v<Event, Wave>)
            {
                std::cout << "*Waves back*\n";
            }
        }
    };

    struct ChangeAttitude
    {
        void operator()(Npc& npc, const AttitudeChange& e) const noexcept
        {
            npc.m_attitude = std::max(npc.m_attitude + e.attitudeChange, 0);
        }
    };

private: // Traits
    struct IsDead
    {
        bool operator()(Npc& npc) const noexcept
        {
            return npc.m_health == 0;
        }
    };

    template<NpcState State>
    struct AttittudeFits
    {
        bool operator()(Npc& npc) const noexcept
        {
            switch (State)
            {
            case NpcState::Friendly: return npc.m_attitude >= 10;
            case NpcState::Neutral: return npc.m_attitude >= 5 && npc.m_attitude < 10;
            case NpcState::Disgruntled: return npc.m_attitude >= 1 && npc.m_attitude < 5;
            case NpcState::Hostile: return npc.m_attitude == 0;
            default: assert(!"Unexpected state"); return false;
            }
        }
    };

    using IsAttitudeFriendly = AttittudeFits<NpcState::Friendly>;
    using IsAttitudeNeutral = AttittudeFits<NpcState::Neutral>;
    using IsAttitudeDisgruntled = AttittudeFits<NpcState::Disgruntled>;
    using IsAttitudeHostile = AttittudeFits<NpcState::Hostile>;

private:
    static constexpr auto Table{ csm::MakeTransitions(
        From<Neutral, Disgruntled> && On<Compliment, Bribe> && If<IsAttitudeFriendly>
            = To<Friendly>,
        From<Friendly, Disgruntled> && On<Compliment, Bribe, Insult, Shove> && If<IsAttitudeNeutral>
            = To<Neutral>,
        From<Friendly, Neutral> && On<Insult, Shove> && If<IsAttitudeDisgruntled> ||
        From<Hostile> && On<PleadForMercy>
            = To<Disgruntled>,
        From<Friendly, Neutral, Disgruntled> && On<Insult, Shove> && If<IsAttitudeHostile> ||
        From<Friendly, Neutral, Disgruntled> && On<Attack> && IfNot<IsDead>
            = To<Hostile>,
        From<Friendly, Neutral, Disgruntled, Hostile> && On<Attack> && If<IsDead>
            = To<Dead>
        ) };

    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Compliment, Bribe, Insult, Shove> && IfNone<IsDead, IsAttitudeHostile>
            = Do<ChangeAttitude, React>,
        On<Attack> && IfNot<IsDead>
            = Do<DealDamage, React>,
        On<PleadForMercy> && IfAll<Not<IsDead>, IsAttitudeHostile> ||
        On<Wave>
            = Do<React>
        ) };

private:
    Health m_health{ 10 };
    Attitude m_attitude{ 10 };
};

int main()
{
    // Health: 10 | Attitude: 10 | State:  Friendly
    Npc npc;
    csm::StateMachine<Npc> sm{ NpcState::Friendly, npc };

    // Health: 10 | Attitude: 7 | State: Neutral
    // Says "Why would you say that?!" and "But I thought we were friends :("
    sm.ProcessEvent(Insult{ -3 });

    // Health: 10 | Attitude: 6 | State: Neutral
    // Says "Stop pushing me!"
    sm.ProcessEvent(Shove{ -1 });

    // Health: 10 | Attitude: 1 | State: Disgruntled
    // Says "Stop pushing me!" and "I hate you!"
    sm.ProcessEvent(Shove{ -5 });

    // Health: 10 | Attitude: 0 | State: Hostile
    // Says "Why would you say that?!" and "Never should've come here!"
    sm.ProcessEvent(Insult{ -3 });

    // Health: 5 | Attitude: 0 | State: Hostile
    // Says "Ouch!"
    sm.ProcessEvent(Attack{ 5 });

    // Health: 5 | Attitude: 0 | State: Hostile
    // No reaction due to conditions
    sm.ProcessEvent(Insult{ -3 });

    // Health: 5 | Attitude: 1 | State: Disgruntled
    // Says "I'll spare you...this time."
    sm.ProcessEvent(PleadForMercy{});

    // Health: 5 | Attitude: 11 | State: Friendly
    // Says "Oh, thanks!" and "Hi new friend :)"
    sm.ProcessEvent(Compliment{ 10 });

    // Health: 0 | Attitude: 0 | State: Dead
    // Says "Ouch!" and "Arghhh!!! :("
    sm.ProcessEvent(Attack{ 5 });

    return 0;
}
