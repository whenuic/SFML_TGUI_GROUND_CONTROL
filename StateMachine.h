#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <memory>
#include <unordered_map>
#include <vector>
#include "State.h"

class Aircraft;
class BannerPanel;
class State;
class InitialState;
class TouchDownState;
class MaintainSpeedState;
class StopState;
class IdleState;
class TakeOffState;
class LeavingState;

class StateMachine
{
  public:
    StateMachine(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    ~StateMachine();

    void Update(float dt);
    int GetCurrentStateId();
    void TransitToState(std::string new_state_name);

  protected:
    int current_state_id_;
    std::vector<std::unique_ptr<State>> states_vector_;
    std::unordered_map<std::string, int> states_name_to_id_;

    std::shared_ptr<Aircraft> aircraft_;
    std::shared_ptr<BannerPanel> panel_;
  private:
};

#endif // STATEMACHINE_H
