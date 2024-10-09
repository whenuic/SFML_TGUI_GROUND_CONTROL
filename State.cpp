#include "State.h"
#include <iostream>
#include <math.h>
#include <limits.h>
#include "Aircraft.h"
#include "BannerPanel.h"

InitialState::InitialState(std::shared_ptr<Aircraft> aircraft,
                           std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "Initial";
}

std::string InitialState::Entry() {
  panel_->RequestGate(aircraft_);
  aircraft_->request_of_gate_sent_ = true;
  return state_name_;
}

std::string InitialState::Update(float dt) {
  if ( timer_ < before_landing_interval ) {
    auto banner = panel_->GetBanner(aircraft_);
    banner->SetText(aircraft_->GetName() + "|" + aircraft_->GetModel() + "|IN:" + std::to_string(int(round(before_landing_interval-timer_))) + "s", 1);
    timer_ += dt;
    return state_name_;
  } else {
    return "TouchDown";
  }
}

TouchDownState::TouchDownState(std::shared_ptr<Aircraft> aircraft,
                               std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "TouchDown";
}

std::string TouchDownState::Entry() {
  auto landing_runway_info = aircraft_->GetLandingRunwayInfo();
  aircraft_->SetGroundRoute(landing_runway_info->route, landing_runway_info->direction, landing_runway_info->touch_down_distance_range[1]);
  aircraft_->Activate();
  aircraft_->SetSpeed(KnotsToMetersPerSecond(160));
  auto banner = panel_->GetBanner(aircraft_);
  auto airport = banner->GetAirport();
  if (aircraft_->gate_assigned_.empty()) {
    // TODO: auto assign gate goes here.
    auto gates_vec = airport->GetGatesWithExactSize(aircraft_->GetSize());
    int num_of_aircrafts = INT_MAX;
    int index = 0;
    for (int i = 0; i < gates_vec.size(); i++) {
      int n = gates_vec[i]->GetAssignedAircraftNumber();
      if (n < num_of_aircrafts) {
        index = i;
        num_of_aircrafts = n;
      }
    }
    aircraft_->gate_assigned_ = gates_vec[index]->GetName();
    gates_vec[index]->AssignAircraft(aircraft_);
  }
  if (!aircraft_->gate_assigned_.empty()) {
    aircraft_->SetTaxiRoutes(airport->GetRoute(aircraft_->GetRoute(), aircraft_->GetDirectionOnRoute(),
                                               aircraft_->GetDistanceOnRoute(),
                                               airport->GetRoutePtr(aircraft_->gate_assigned_),
                                               true, 1));
    banner->DisableGateSelector();
  }
  return "MaintainSpeed";
}

MaintainSpeedState::MaintainSpeedState(std::shared_ptr<Aircraft> aircraft,
                                       std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "MaintainSpeed";
}

std::string MaintainSpeedState::Entry() {
  if (aircraft_->taxi_routes_.empty()) {
    if (aircraft_->clearance_of_line_up_received_ && aircraft_->clearance_of_take_off_received_) {
      return "TakeOff";
    } else {
      return "IdleState";
    }
  }
  current_route_ = aircraft_->route_;
  current_route_speed_limit_ = KnotsToMetersPerSecond(current_route_->GetTaxiSpeedLimit());
  if (aircraft_->taxi_routes_.size() > 1) {
    next_route_ = current_route_->GetConnectionInfo(*std::next(aircraft_->taxi_routes_.begin())).next_piece;
    next_route_speed_limit_ = KnotsToMetersPerSecond(next_route_->GetTaxiSpeedLimit());
  } else {
    next_route_ = nullptr;
    next_route_speed_limit_ = 0;
  }
  if (aircraft_->distance_to_next_hold_ <= 0) {
    aircraft_->distance_to_next_hold_ = aircraft_->route_->GetDistanceToNextHold(aircraft_->taxi_routes_,
                                                                               aircraft_->distance_on_route_,
                                                                               aircraft_->direction_on_route_,
                                                                               aircraft_->next_hold_type_);
  }

  panel_->TurnOnManualTaxiHold(aircraft_);

  return state_name_;
}

std::string MaintainSpeedState::Update(float dt) {
  //meters, need to make caution if a flight is within this distance in the way
  // make this adaptive to the speed of the aircraft
  float caution_follow_distance = 100 + aircraft_->GetLength() / 2 + aircraft_->GetSpeed() * 7;
  float result_search_distance = caution_follow_distance;
  auto leading_aircraft = aircraft_->route_->ClosestAircraftInWay(aircraft_,
                                             result_search_distance,
                                             aircraft_->GetDirectionOnRoute(),
                                             aircraft_->GetDistanceOnRoute());

  // break ahead of
  float brake_ahead_distance = 200 + aircraft_->GetLength() / 2; // meters

  // 1.4 Setup panel display
  auto banner = panel_->GetBanner(aircraft_);
  banner->SetText(aircraft_->GetName() + "|" + aircraft_->GetModel() + "|SPD:" + std::to_string(int(round(aircraft_->GetSpeed()))), 1);
  banner->SetText("TAXI TO " + aircraft_->GetTaxiRoutes().back(), 2);
  banner->SetText(aircraft_->GetTaxiRoutesString(), 3);

  // 1.5 Compute target speed and acceleration
  float target_speed = current_route_speed_limit_;
  if (aircraft_->taxi_routes_.size() > 1) {
    float break_out_dist = aircraft_->route_->GetConnectionInfo(
      next_route_->GetName()).distance_to_break_out;
    bool close_to_next_route = aircraft_->direction_on_route_ ?
    (aircraft_->distance_on_route_ + brake_ahead_distance >= break_out_dist) :
    (aircraft_->distance_on_route_ - brake_ahead_distance <= break_out_dist);
    if (close_to_next_route &&
        target_speed > next_route_speed_limit_) {
      target_speed = next_route_speed_limit_;
    }
  }
  if (leading_aircraft) {
    target_speed = std::min(target_speed, leading_aircraft->GetSpeed());
  }
  if (aircraft_->manual_taxi_hold_) {
    target_speed = 0;
  }

  if (aircraft_->speed_ > target_speed + KnotsToMetersPerSecond(20)) {
    aircraft_->acceleration_ = aircraft_->DetermineAcceleration(
                                 target_speed, dt,
                                 aircraft_->soft_ground_acceleration_,
                                 aircraft_->max_ground_deacceleration_);
  } else {
    aircraft_->acceleration_ = aircraft_->DetermineAcceleration(
                                 target_speed, dt,
                                 aircraft_->soft_ground_acceleration_,
                                 aircraft_->soft_ground_deacceleration_);
  }

  // 2. update speed, distance, distance_to_next_hold
  float dv = aircraft_->acceleration_ * dt;
  float dist = (aircraft_->speed_ + dv / 2.0) * dt;
  aircraft_->speed_ += dv;
  aircraft_->distance_to_next_hold_ -= dist;
  if (aircraft_->distance_to_next_hold_ <= -aircraft_->length_ / 2) {
    aircraft_->distance_to_next_hold_ = aircraft_->route_->GetDistanceToNextHold(aircraft_->taxi_routes_,
                                                                               aircraft_->distance_on_route_,
                                                                               aircraft_->direction_on_route_,
                                                                               aircraft_->next_hold_type_);
  }

  // 3. update position
  aircraft_->route_->ComputePosition(aircraft_, aircraft_->distance_on_route_, aircraft_->direction_on_route_, dist, &(aircraft_->route_), aircraft_->taxi_routes_);
  aircraft_->sprite_.setPosition(ToSfmlPosition(aircraft_->route_->GetBreakOutPosition(
                                         aircraft_->distance_on_route_)));
  aircraft_->sprite_.setRotation(ToSfmlRotation(aircraft_->route_->GetRotation(
                                         aircraft_->distance_on_route_, aircraft_->direction_on_route_)));

  // 3.5 Update current_route if needed
  if (aircraft_->taxi_routes_.empty()) {
    if (aircraft_->clearance_of_take_off_received_) {
      return "TakeOff";
    }
    return "IdleState";
  }
  if (aircraft_->route_ != current_route_) {
    current_route_ = aircraft_->route_;
    if (current_route_->GetRouteType() == RouteType::GATE && !aircraft_->gate_) {
      aircraft_->gate_ = current_route_;
    }
    current_route_speed_limit_ = KnotsToMetersPerSecond(current_route_->GetTaxiSpeedLimit());
    if (aircraft_->taxi_routes_.size() > 1) {
      next_route_ = current_route_->GetConnectionInfo(*std::next(aircraft_->taxi_routes_.begin())).next_piece;
      next_route_speed_limit_ = KnotsToMetersPerSecond(next_route_->GetTaxiSpeedLimit());
    } else {
      next_route_ = nullptr;
      next_route_speed_limit_ = 0;
    }
  }

  // 3.6 if this is the last route and distance to the end <= v^2/(2*a) || distance to hold <= v^2/(2*a), to StopState
  // Ignore hold and stop if clearance of take off received
  if (!aircraft_->clearance_of_take_off_received_) {
    float break_distance = abs(aircraft_->speed_ * aircraft_->speed_ / 2 / aircraft_->soft_ground_deacceleration_);
    if ((aircraft_->taxi_routes_.size() == 1) &&
      (break_distance >= (aircraft_->direction_on_route_ ? aircraft_->route_->GetLength() - aircraft_->distance_on_route_ : aircraft_->distance_on_route_))) {
      return "Stop";
    }
    // HoldPointType next_hold_type = HoldPointType::NOTSET;
    // should compute use nose position, add this compensation
    // float compensate_distance = aircraft_->direction_on_route_ ? aircraft_->length_/2 : -aircraft_->length_/2;
    float compensate_distance = aircraft_->length_ / 2;
    if (aircraft_->speed_ <= KnotsToMetersPerSecond(aircraft_->route_->GetTaxiSpeedLimit()) &&
        break_distance + compensate_distance >= aircraft_->distance_to_next_hold_ && aircraft_->next_hold_type_ != HoldPointType::NOTSET) {
      if (aircraft_->next_hold_type_ == HoldPointType::TAKEOFF) {
        return "Hold";
      } else if (aircraft_->next_hold_type_ == HoldPointType::TRAFFIC) {
        aircraft_->manual_taxi_hold_ = true;
        aircraft_->next_hold_type_ = HoldPointType::NOTSET;
        banner->TurnOnManualTaxiResume();
      }
    }
  }
  // 4. return
  return state_name_;
}

void MaintainSpeedState::Exit() {
  panel_->TurnOffManualTaxiHold(aircraft_);
}

StopState::StopState(std::shared_ptr<Aircraft> aircraft,
                     std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "Stop";
}

std::string StopState::Entry() {
  return state_name_;
}

std::string StopState::Update(float dt) {
  // Banner
  auto banner = panel_->GetBanner(aircraft_);
  banner->SetText(aircraft_->GetName() + "|" + aircraft_->GetModel() + "|SPD:" + std::to_string(int(round(aircraft_->GetSpeed()))), 1);
  banner->SetText("TAXI", 2);
  banner->SetText(aircraft_->GetTaxiRoutesString(), 3);

  // handle final stop
  // since already calculated within final stop range, just use brake to stop
  // after make full stop, transit to idlestate
  aircraft_->acceleration_ = aircraft_->soft_ground_deacceleration_;
  float dv = aircraft_->acceleration_ * dt;
  float dist = (aircraft_->speed_ + dv / 2.0) * dt;

  aircraft_->speed_ += dv;
  if (aircraft_->speed_ < 0) {
    return "IdleState";
  } else {
    aircraft_->route_->ComputePosition(aircraft_, aircraft_->distance_on_route_, aircraft_->direction_on_route_, dist, &(aircraft_->route_), aircraft_->taxi_routes_);
    aircraft_->sprite_.setPosition(ToSfmlPosition(aircraft_->route_->GetBreakOutPosition(
                                          aircraft_->distance_on_route_)));
    aircraft_->sprite_.setRotation(ToSfmlRotation(aircraft_->route_->GetRotation(
                                          aircraft_->distance_on_route_, aircraft_->direction_on_route_)));
  }
  return state_name_;
}

void StopState::Exit() {
  aircraft_->speed_ = 0;
  aircraft_->acceleration_ = 0;
  aircraft_->taxi_routes_.clear();
}

IdleState::IdleState(std::shared_ptr<Aircraft> aircraft,
                     std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "IdleState";
}

std::string IdleState::Entry() {
  aircraft_->speed_ = 0;
  aircraft_->acceleration_ = 0;
  return state_name_;
}

std::string IdleState::Update(float dt) {
  auto banner = panel_->GetBanner(aircraft_);
  banner->SetText(aircraft_->GetName() + "|" + aircraft_->GetModel() + "|SPD:" + std::to_string(int(round(aircraft_->GetSpeed()))), 1);
  banner->SetText("IDLE", 2);
  banner->SetText(aircraft_->GetTaxiRoutesString(), 3);

  if (aircraft_->gate_ && !aircraft_->request_of_runway_sent_) {
    timer_ += dt;
    if (timer_ > aircraft_->max_time_at_gate_) {
      return "RequestRunway";
    }
  }
  if (aircraft_->request_of_take_off_sent_) {
    if (aircraft_->clearance_of_line_up_received_ && !aircraft_->GetTaxiRoutes().empty()) {
      return "MaintainSpeed";
    }
    if (aircraft_->clearance_of_take_off_received_) {
      return "TakeOff";
    }
  }

  return state_name_;
}

void IdleState::Exit() {
  timer_ = 0;
}

RequestRunwayState::RequestRunwayState(std::shared_ptr<Aircraft> aircraft,
                                       std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "RequestRunway";
}

std::string RequestRunwayState::Entry() {
  auto banner = panel_->GetBanner(aircraft_);
  banner->SetText("", 3);
  aircraft_->request_of_runway_sent_ = true;
  aircraft_->take_off_runway_assigned_ = false;
  panel_->RequestRunway(aircraft_);
  return state_name_;
}

std::string RequestRunwayState::Update(float dt) {
  if (aircraft_->take_off_runway_assigned_) {
    return "RequestPushBack";
  }
  return state_name_;
}

RequestPushBackState::RequestPushBackState(std::shared_ptr<Aircraft> aircraft,
                                           std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "RequestPushBack";
}

std::string RequestPushBackState::Entry() {
  aircraft_->request_of_push_back_sent_ = true;
  aircraft_->clearance_of_push_back_received_ = false;
  panel_->RequestPushBack(aircraft_);
  return state_name_;
}

std::string RequestPushBackState::Update(float dt) {
  if (aircraft_->clearance_of_push_back_received_) {
    return "PushBack";
  }
  return state_name_;
}

PushBackState::PushBackState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "PushBack";
}

std::string PushBackState::Entry() {
  // setup push back route;
  std::list<std::string> push_back_routes;
  push_back_routes.push_back(aircraft_->gate_->GetName());
  push_back_routes.push_back(static_cast<Gate*>(aircraft_->gate_)->GetPushBackRoute(aircraft_->take_off_runway_)->GetName());
  aircraft_->SetTaxiRoutes(push_back_routes);
  aircraft_->direction_on_route_ = !aircraft_->direction_on_route_;
  push_back_speed_ = KnotsToMetersPerSecond(aircraft_->gate_->GetTaxiSpeedLimit());
  // setup panel
  return state_name_;
}

std::string PushBackState::Update(float dt) {
  // 1. update banner
  auto banner = panel_->GetBanner(aircraft_);
  banner->SetText(aircraft_->GetName() + "|" + aircraft_->GetModel() + "|SPD:" + std::to_string(int(round(aircraft_->GetSpeed()))), 1);
  banner->SetText("PUSH_BACK", 2);
  banner->SetText(aircraft_->GetTaxiRoutesString(), 3);

  // 2. update dynamics
  aircraft_->acceleration_ = aircraft_->DetermineAcceleration(
                                 push_back_speed_, dt,
                                 aircraft_->soft_ground_acceleration_,
                                 aircraft_->soft_ground_deacceleration_);

  // 2. update speed and distance
  float dv = aircraft_->acceleration_ * dt;
  float dist = (aircraft_->speed_ + dv / 2.0) * dt;
  aircraft_->speed_ += dv;
  // 3. update position
  aircraft_->route_->ComputePosition(aircraft_, aircraft_->distance_on_route_, aircraft_->direction_on_route_, dist, &(aircraft_->route_), aircraft_->taxi_routes_);
  aircraft_->sprite_.setPosition(ToSfmlPosition(aircraft_->route_->GetBreakOutPosition(
                                         aircraft_->distance_on_route_)));
  aircraft_->sprite_.setRotation(ToSfmlRotation(aircraft_->route_->GetRotation(
                                         aircraft_->distance_on_route_, aircraft_->direction_on_route_) + 180));
  // 3.5 Update current_route if needed
  if (aircraft_->taxi_routes_.empty()) {
    return "MaintainSpeed";
  }

  return state_name_;
}

void PushBackState::Exit() {
  // free gate
  static_cast<Gate*>(aircraft_->gate_)->Free(aircraft_);

  // set banner

  // set position for taxi to runway
  // 1.get push back last route
  auto push_back_route = static_cast<Gate*>(aircraft_->gate_)->GetPushBackRoute(aircraft_->take_off_runway_);
  // 2.get connection route
  auto next_routes = push_back_route->GetConnectedRouteBreakOutAt(push_back_route->GetLength());
  if (next_routes.back() == aircraft_->gate_) {
    next_routes = push_back_route->GetConnectedRouteBreakOutAt(0);
  }
  std::string first_taxi_to_runway_route_name = next_routes.back()->GetName();
  // This line was used to be the first line in this function. Still can be the first if caching search result of taxi to runway from gate.
  // std::string first_taxi_to_runway_route_name = static_cast<Gate*>(aircraft_->gate_)->GetTaxiToRunwayList(aircraft_->take_off_runway_).front();
  auto break_in_distance = aircraft_->route_->GetConnectionInfo(first_taxi_to_runway_route_name).distance_to_break_in;
  auto break_in_direction = aircraft_->route_->GetConnectionInfo(first_taxi_to_runway_route_name).positive_entering_next_piece;
  auto next_route = aircraft_->route_->GetConnectionInfo(first_taxi_to_runway_route_name).next_piece;
  aircraft_->SetGroundRoute(next_route, !break_in_direction, break_in_distance);

  // set taxi_routes
  // aircraft_->SetTaxiRoutes(static_cast<Gate*>(aircraft_->gate_)->GetTaxiToRunwayList(aircraft_->take_off_runway_));
  auto banner = panel_->GetBanner(aircraft_);
  auto airport = banner->GetAirport();
  HoldPoint line_up_point = airport->GetLineUpPoint(aircraft_->take_off_runway_);
  aircraft_->SetTaxiRoutes(airport->GetRoute(aircraft_->GetRoute(), aircraft_->GetDirectionOnRoute(),
                                               aircraft_->GetDistanceOnRoute(),
                                               line_up_point.route, line_up_point.direction, line_up_point.distance_on_route));
}

TakeOffState::TakeOffState(std::shared_ptr<Aircraft> aircraft,
                           std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "TakeOff";
}

std::string TakeOffState::Entry() {
  // set groundroute
  aircraft_->taxi_routes_.push_back(aircraft_->route_->GetName());
  aircraft_->taxi_routes_.push_back(aircraft_->take_off_runway_.substr(1,aircraft_->take_off_runway_.size()-1));
  //
  return state_name_;
}

std::string TakeOffState::Update(float dt) {
  auto banner = panel_->GetBanner(aircraft_);
  banner->SetText(aircraft_->GetName() + "|" + aircraft_->GetModel() + "|SPD:" + std::to_string(int(round(aircraft_->GetSpeed()))), 1);
  banner->SetText("TAKE_OFF", 2);
  banner->SetText(aircraft_->GetTaxiRoutesString(), 3);

  float take_off_speed = KnotsToMetersPerSecond(170);

  aircraft_->acceleration_ = aircraft_->DetermineAcceleration(take_off_speed, dt,
                                            aircraft_->max_ground_acceleration_,
                                            aircraft_->soft_ground_deacceleration_);
  float dv = aircraft_->acceleration_ * dt;
  float dist = (aircraft_->speed_ + dv / 2.0) * dt;
  aircraft_->speed_ += dv;
  // update position
  aircraft_->route_->ComputePosition(aircraft_, aircraft_->distance_on_route_, aircraft_->direction_on_route_,
                          dist, &aircraft_->route_, aircraft_->taxi_routes_);
  aircraft_->sprite_.setPosition(ToSfmlPosition(aircraft_->route_->GetBreakOutPosition(
                                       aircraft_->distance_on_route_)));
  aircraft_->sprite_.setRotation(ToSfmlRotation(aircraft_->route_->GetRotation(
                                       aircraft_->distance_on_route_, aircraft_->direction_on_route_)));

  if (aircraft_->speed_ >= take_off_speed) {
    return "Leaving";
  }
  return state_name_;
}

HoldState::HoldState(std::shared_ptr<Aircraft> aircraft,
                           std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "Hold";
}

std::string HoldState::Entry() {
  if (aircraft_->next_hold_type_ == HoldPointType::TAKEOFF) {
    panel_->RequestTakeOff(aircraft_);
    aircraft_->request_of_take_off_sent_ = true;
    aircraft_->acceleration_ = aircraft_->soft_ground_deacceleration_;
    aircraft_->next_hold_type_ = HoldPointType::NOTSET;
    return state_name_;
  }
  return state_name_;
}

std::string HoldState::Update(float dt) {
  // after hold clearance, either transit to maintain speed(hold before align runway, hold for traffic, hold for cross runway) or take off(hold align runway)
  float dv = aircraft_->acceleration_ * dt;
  float dist = (aircraft_->speed_ + dv / 2.0) * dt;
  aircraft_->distance_to_next_hold_ -= dist;
  aircraft_->speed_ += dv;
  if (aircraft_->speed_ < 0) {
    return "IdleState";
  } else {
    aircraft_->route_->ComputePosition(aircraft_, aircraft_->distance_on_route_, aircraft_->direction_on_route_, dist, &(aircraft_->route_), aircraft_->taxi_routes_);
    aircraft_->sprite_.setPosition(ToSfmlPosition(aircraft_->route_->GetBreakOutPosition(
                                          aircraft_->distance_on_route_)));
    aircraft_->sprite_.setRotation(ToSfmlRotation(aircraft_->route_->GetRotation(
                                          aircraft_->distance_on_route_, aircraft_->direction_on_route_)));
  }
  if (aircraft_->clearance_of_line_up_received_) {
    return "MaintainSpeed";
  }
  if (aircraft_->clearance_of_take_off_received_) {
    return "TakeOff";
  }
  return state_name_;
}

LeavingState::LeavingState(std::shared_ptr<Aircraft> aircraft,
                           std::shared_ptr<BannerPanel> panel)
  : State(aircraft, panel) {
  state_name_ = "Leaving";
}

std::string LeavingState::Update(float dt) {
  return state_name_;
}

std::string LeavingState::Entry() {
  panel_->RemoveBanner(aircraft_);
  aircraft_->Deactivate();
  aircraft_->Delete();
  return state_name_;
}
