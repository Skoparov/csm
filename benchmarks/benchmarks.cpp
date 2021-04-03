#include <csm.h>
#include <chrono>
#include <iostream>

struct Play{};
struct EndPause{};
struct Stop{};
struct Pause{};
struct OpenClose{};
struct CdDetected{};

enum class PlayerState{ Empty, Open, Stopped, Playing, Pause };

struct Player : public csm::StateMachine<Player, PlayerState>
{
    using csm::StateMachine<Player, PlayerState>::StateMachine;

    struct Empty : State<PlayerState::Empty>{};
    struct Open : State<PlayerState::Open>{};
    struct Stopped : State<PlayerState::Stopped>{};
    struct Playing : State<PlayerState::Playing>{};
    struct Pause : State<PlayerState::Pause>{};

    struct StartPlayback{
        template<class E>
        void operator()(Player& p, const E&){
            ++p.data;
            //std::cout << "StartPlayback" << std::endl;
        } };

    struct ResumePlayback{
        template<class E>
        void operator()(Player& p, const E&){
            ++p.data;
            //std::cout << "ResumePlayback" << std::endl;
        } };

    struct CloseDrawer{
        template<class E>
        void operator()(Player& p, const E&){
            ++p.data;
            //std::cout << "CloseDrawer" << std::endl;
        } };

    struct OpenDrawer{
        template<class E>
        void operator()(Player&, const E&){
            //std::cout << "OpenDrawer" << std::endl;
        } };

    struct StopAndOpen{
        template<class E>
        void operator()(Player& p, const E&){
            ++p.data;
            //std::cout << "StopAndOpen" << std::endl;
        } };

    struct StoppedAgain{
        template<class E>
        void operator()(Player& p, const E&){
            ++p.data;
            //std::cout << "StoppedAgain" << std::endl;
        } };

    struct StoreCdInfo{
        template<class E>
        void operator()(Player& p, const E&){
            ++p.data;
            //std::cout << "StoreCdInfo" << std::endl;
        } };

    struct PausePlayback{
        template<class E>
        void operator()(Player&, const E&){
            //std::cout << "PausePlayback" << std::endl;
        } };

    struct StopPlayback{
        template<class E>
        void operator()(Player& p, const E&){
            ++p.data;
            //std::cout << "StopPlayback" << std::endl;
        } };

    template<PlayerState State>
    struct IsState{
        bool operator()(const Player& p){
            return p.GetState() == State;
        } };

    static constexpr auto TransitionRules = csm::MakeTransitionRules(
        (From<Stopped> && On<Play>) || (From<Pause> && On<EndPause>) = To<Playing>,
        From<Open> && On<OpenClose> = To<Empty>,
        From<Empty, Pause, Stopped, Playing> && On<OpenClose> = To<Open>,
        From<Playing> && On<Pause> = To<Pause>,
        (From<Playing, Pause> && On<Stop>) || (From<Empty> && On<CdDetected>) = To<Stopped>
    );

    static constexpr auto ActionRules = csm::MakeActionRules(
        On<Play> && If<IsState<PlayerState::Stopped>> = Do<StartPlayback>,
        On<EndPause> && If<IsState<PlayerState::Pause>> = Do<ResumePlayback>,
        On<OpenClose> && If<IsState<PlayerState::Open>> = Do<CloseDrawer>,
        On<OpenClose> && IfAny<IsState<PlayerState::Empty>, IsState<PlayerState::Stopped>> = Do<OpenDrawer>,
        On<OpenClose> && IfAny<IsState<PlayerState::Pause>, IsState<PlayerState::Playing>> = Do<StopAndOpen>,
        On<Pause> && If<IsState<PlayerState::Playing>> = Do<PausePlayback>,
        On<Stop> && IfAny<IsState<PlayerState::Pause>, IsState<PlayerState::Playing>> = Do<StopPlayback>,
        On<CdDetected> && If<IsState<PlayerState::Empty>> = Do<StoreCdInfo>,
        On<Stop> && If<IsState<PlayerState::Stopped>> = Do<StoppedAgain>
    );

    int data{ 0 };
};

int main()
{
    auto begin{ std::chrono::steady_clock::now() };

    Player sm{ PlayerState::Empty };
    for (size_t i{ 0 }; i < 1'000'000; ++i)
    {
        sm.ProcessEvent(OpenClose{});
        sm.ProcessEvent(OpenClose{});
        sm.ProcessEvent(CdDetected{});
        sm.ProcessEvent(Play{});
        sm.ProcessEvent(Pause{});
        sm.ProcessEvent(EndPause{});
        sm.ProcessEvent(Pause{});
        sm.ProcessEvent(Stop{});
        sm.ProcessEvent(Stop{});
        sm.ProcessEvent(OpenClose{});
        sm.ProcessEvent(OpenClose{});
    }

    auto end{ std::chrono::steady_clock::now() };
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
}
