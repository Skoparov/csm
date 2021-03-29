# Compact State Machine

An event based header-only lightweight state machine that extensively employs compile time to avoid virtual calls and simplify the declaration of transition tables/action rules.

The main motivation behind this project is to provide users with a means to declare extensible, compact and readable complex transition rules while avoiding constant repetition.
For example, if you have several states of a person:

```
enum class AttitudeState{ Friendly, Neutral, Hostile };
```

and several events:
```
struct MeleeAtack{};
struct RangedAttack{};
```

a typical declaration of the transition table would look akin to:
```
Friendly + MeleeAttack = Hostile
Friendly + RangedAttack = Hostile
Neutral + MeleeAttack = Hostile
Neutral + RangedAttack = Hostile
```
With the syntax provided with `csm` the equivalent declaration would be:
```
From<Friendly, Neutral> && On<MeleeAttack, RangedAttack> = To<Hostile>
```
The syntax allows to merge multiple conditions leading to the same outcome into logical groups. 

## Features
The library is currently under development and is more of a proof of concept. As of now, it supports:
* State transitioning
* Entry/exit actions
* Event actions 
* Flexible guards for both of the above

Planned features include:
* State observers
* State history

## Installation
The library is header-only, simply copy the header into your project's directory and include it.

## Wiki
* [General Overview](https://github.com/Skoparov/csm/wiki/General-overview)
* [Concepts](https://github.com/Skoparov/csm/wiki/Concepts)
  * [Syntax definitions](https://github.com/Skoparov/csm/wiki/Concepts/#syntax-definitions)
  * [State](https://github.com/Skoparov/csm/wiki/Concepts/#state)
  * [Event](https://github.com/Skoparov/csm/wiki/Concepts/#event)
  * [Guard](https://github.com/Skoparov/csm/wiki/Concepts/#guard)
  * [Action](https://github.com/Skoparov/csm/wiki/Concepts/#action)
  * [Transition table](https://github.com/Skoparov/csm/wiki/Concepts/#transition-table)
  * [Action Rules](https://github.com/Skoparov/csm/wiki/Concepts/#action-rules)
  * [Predicate wrappers and convenience typedefs](https://github.com/Skoparov/csm/wiki/Concepts/#predicate-wrappers-and-convenience-typedefs)

## Code example
A very basic canonical example of a swtich state machine might look like this:
```c++
#include <csm.h>

// External states
enum class SwitchState{ On, Off, Broken };

// Events
struct Press{};
struct Smash{};

struct Switch : csm::StateMachine<Switch, SwitchState>
{
    // Internal states
    struct StateOn : State<SwitchState::On>{};
    struct StateOff : State<SwitchState::Off>{};
    struct StateBroken : State<SwitchState::Broken>{};

    // Action
    struct Break{
        void operator()(Switch& s, const Smash&){ s.isBroken = true; }
    };

    // Guard
    struct IsBroken{
        bool operator()(Switch& s){ return s.isBroken; }
    };

    // Transition table and action rules
    static constexpr auto TransitionRules{ csm::MakeTransitionRules(
        From<StateOn> && On<Press> && IfNot<IsBroken> = To<StateOff>,
        From<StateOff> && On<Press> && IfNot<IsBroken> = To<StateOn>,
        From<StateOn, StateOff> && On<Smash> = To<StateBroken>
    )};

    static constexpr auto ActionRules{ csm::MakeActionRules(
        On<Smash> && IfNot<IsBroken> = Do<Break>
    )};

    bool isBroken{ false };
};

void UseSwitch()
{
    Switch sm{ SwitchState::Off };

    sm.ProcessEvent(Press{}); // Off -> On
    auto currState{ sm.GetState() }; // On

    sm.ProcessEvent(Smash{}); // On -> Broken
    currState = sm.GetState(); // Broken

    sm.ProcessEvent(Press{}); // Ignored
    currState = sm.GetState(); // Broken
}
```

For more code examples please refer to [examples](https://github.com/Skoparov/csm/tree/main/examples)