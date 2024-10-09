#include "StateMachine.h"
#include <iostream>
#include "BannerPanel.h"

StateMachine::StateMachine(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel)
  : aircraft_(aircraft),
    panel_(panel) {

  panel_->CreateBanner(aircraft_);

  states_vector_.push_back(std::make_unique<InitialState>(InitialState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<TouchDownState>(TouchDownState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<MaintainSpeedState>(MaintainSpeedState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<StopState>(StopState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<IdleState>(IdleState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<RequestRunwayState>(RequestRunwayState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<RequestPushBackState>(RequestPushBackState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<PushBackState>(PushBackState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<TakeOffState>(TakeOffState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<HoldState>(HoldState(aircraft_, panel_)));
  states_vector_.push_back(std::make_unique<LeavingState>(LeavingState(aircraft_, panel_)));

  states_name_to_id_["Initial"] = 0;
  states_name_to_id_["TouchDown"] = 1;
  states_name_to_id_["MaintainSpeed"] = 2;
  states_name_to_id_["Stop"] = 3;
  states_name_to_id_["IdleState"] = 4;
  states_name_to_id_["RequestRunway"] = 5;
  states_name_to_id_["RequestPushBack"] = 6;
  states_name_to_id_["PushBack"] = 7;
  states_name_to_id_["TakeOff"] = 8;
  states_name_to_id_["Hold"] = 9;
  states_name_to_id_["Leaving"] = 10;

  current_state_id_ = 0;
  TransitToState("Initial");
}

int StateMachine::GetCurrentStateId() {
  return current_state_id_;
}

void StateMachine::Update(float dt) {
  std::string next_state_name = states_vector_[current_state_id_]->Update(dt);
  auto next_state_id = states_name_to_id_[next_state_name];
  if (current_state_id_ != next_state_id) {
    TransitToState(next_state_name);
  }
}

// TransitToState is called to trasit to a new state.
void StateMachine::TransitToState(std::string next_state_name) {
  states_vector_[current_state_id_]->Exit();
  std::cout << aircraft_->GetName() << " Transit to " << next_state_name << std::endl;
  auto next_state_id = states_name_to_id_[next_state_name];
  current_state_id_ = next_state_id;
  next_state_name = states_vector_[current_state_id_]->Entry();
  next_state_id = states_name_to_id_[next_state_name];
  if (next_state_id != current_state_id_) {
    TransitToState(next_state_name);
  }
}

StateMachine::~StateMachine()
{
  //dtor
}
