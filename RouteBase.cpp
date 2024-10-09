#include <iostream>
#include "RouteBase.h"
#include "Utils.h"
#include <math.h>
#include <limits.h>

#include "Aircraft.h"

RouteBase::RouteBase(sf::RenderWindow* app, sf::Font* font)
  : app_(app),
    font_(font) {
  text_.setFont(*font_);
}

sf::Vector2f RouteBase::GetStartPosition() {
  return center_points_.front().position;
}

float RouteBase::GetRotation(float dis, bool direction) {
  int index = dis / ds_;
  return center_points_[index].rotation + (direction ? 0 : 180);
}

sf::Vector2f RouteBase::GetBreakOutPosition(float dis) {
  int index = dis / ds_;
  return center_points_[index].position;
}

void RouteBase::AddHoldPoint(HoldPoint hold_point) {
  // 1. Add point to end
  bool direction = hold_point.direction;
  auto& vec = direction_to_hold_point_[direction];
  vec.push_back(hold_point);

  // 1.5 Add hold point RectangleShape
  sf::RectangleShape hp;
  hp.setFillColor(sf::Color::Yellow);
  hp.setSize(sf::Vector2f(width_*1.5, 5));
  hp.setOrigin(width_*1.5 / 2, 5/2);
  hp.setPosition(ToSfmlPosition(GetBreakOutPosition(hold_point.distance_on_route)));
  hp.setRotation(ToSfmlRotation(GetRotation(hold_point.distance_on_route, hold_point.direction)) + 90);
  if (hold_point.type != HoldPointType::LINEUP) {
    hold_point_rects_.push_back(hp);
  }

  // 2. Sort vector based on direction. + : sort ascend, - : sord descend.
  int n = vec.size();
  if (direction) {
    for (int i = 0; i < n; i++) {
      int target_index = i;
      float target_distance = vec[i].distance_on_route;
      for (int j = i + i; j < n; j++) {
        if (direction ? (vec[j].distance_on_route < target_distance) :
                        (vec[j].distance_on_route >  target_distance)) {
          target_distance = vec[j].distance_on_route;
          target_index = j;
        }
      }
      auto target_point = vec[target_index];
      vec[target_index] = vec[i];
      vec[i] = target_point;
    }
  }
}

std::vector<HoldPoint> RouteBase::GetHoldPoints(bool direction) {
  return direction_to_hold_point_[direction];
}

// Should be called in pair. A connects to B, then B connects to A.
void RouteBase::ConnectRoute(float my_break_out_distance,
                             bool direction_allowed_to_enter_next_route,
                             RouteBase* next_route,
                             float next_break_in_distance,
                             bool positive_entering_next_route) {
  connections_.insert({next_route->GetName(),
                       {direction_allowed_to_enter_next_route,
                        my_break_out_distance,
                        next_route,
                        next_break_in_distance,
                        positive_entering_next_route} });

  // update breakpoints_, and sort the vector
  InsertBreakpoint({BREAKPOINT_TYPE::OUT, next_route, my_break_out_distance});

  next_route->InsertBreakpoint({BREAKPOINT_TYPE::IN, this, next_break_in_distance });
}

void RouteBase::InsertBreakpoint(BreakpointInfo breakpoint_info) {
  breakpoints_.push_back(breakpoint_info);
  std::sort(breakpoints_.begin(), breakpoints_.end(), [](BreakpointInfo a, BreakpointInfo b) { return a.distance_on_current_route < b.distance_on_current_route; });

  // std::cout << GetName() << ": ";
  // for (auto& b : breakpoints_) {
  //   std::string type = b.type == BREAKPOINT_TYPE::IN ? "IN" : "OUT";
  //   std::cout << b.distance_on_current_route << "(" << type <<")";
  // }
  // std::cout << std::endl;
}

void RouteBase::CreateSegments() {
  // 1. count how many breakpoints, including 0 and end, determine how many segments are required.
  std::vector<float> bp_dist;
  // add zero
  bp_dist.push_back(0);
  for (auto& b : breakpoints_) {
    if (b.distance_on_current_route != bp_dist.back()) {
      bp_dist.push_back(b.distance_on_current_route);
    }
  }
  // add end
  if (GetLength() != bp_dist.back()) {
    bp_dist.push_back(GetLength());
  }

  int num_of_segment = bp_dist.size() - 1;
  int seg_name_appendix = 0;
  for (int i = 0; i < num_of_segment; i++) {
    // positive segment
    if (one_way_indicator_ >= 0) {
      SegmentInfo seg_info_positive;
      seg_info_positive.route = this;
      seg_info_positive.name = GetName() + "|" + std::to_string(seg_name_appendix) + "+";
      seg_info_positive.length = abs(bp_dist[i] - bp_dist[i + 1]);
      seg_info_positive.direction = true;
      seg_info_positive.start_distance = std::min(bp_dist[i], bp_dist[i + 1]);
      seg_info_positive.end_distance = std::max(bp_dist[i], bp_dist[i + 1]);
      segments_.push_back(seg_info_positive);
    }
    // negative segment
    if (one_way_indicator_ <= 0) {
      SegmentInfo seg_info_negative;
      seg_info_negative.route = this;
      seg_info_negative.name = GetName() + "|" + std::to_string(seg_name_appendix) + "-";
      seg_info_negative.length = abs(bp_dist[i] - bp_dist[i + 1]);
      seg_info_negative.direction = false;
      seg_info_negative.start_distance = std::max(bp_dist[i], bp_dist[i + 1]);
      seg_info_negative.end_distance = std::min(bp_dist[i], bp_dist[i + 1]);
      segments_.push_back(seg_info_negative);
    }
    seg_name_appendix++;
  }

  // 2. Complete in route connection relationship
  if (one_way_indicator_ == 0) {
    for (int i = 0; i < num_of_segment - 1; i++) {
      segments_[2 * i].out_segment.push_back(segments_[2 * i + 2].name);
    }
    for (int i = 1; i < num_of_segment; i++) {
      segments_[2 * i].in_segment.push_back(segments_[2 * i - 2].name);
    }
    for (int i = 0; i < num_of_segment - 1; i++) {
      segments_[2 * i + 1].in_segment.push_back(segments_[2 * i + 3].name);
    }
    for (int i = 1; i < num_of_segment; i++) {
      segments_[2 * i + 1].out_segment.push_back(segments_[2 * i - 1].name);
    }
  }
  else if (one_way_indicator_ == 1) {
    for (int i = 0; i < num_of_segment - 1; i++) {
      segments_[i].out_segment.push_back(segments_[i + 1].name);
    }
    for (int i = 1; i < num_of_segment; i++) {
      segments_[i].in_segment.push_back(segments_[i - 1].name);
    }
  } else {
    // one_way_indicator_ == -1
    for (int i = 0; i < num_of_segment - 1; i++) {
      segments_[i].in_segment.push_back(segments_[i + 1].name);
    }
    for (int i = 1; i < num_of_segment; i++) {
      segments_[i].out_segment.push_back(segments_[i - 1].name);
    }
  }
}

std::string RouteBase::GetBreakoutSegmentName(bool direction, float distance_to_break_out) {
  for (auto& s : segments_) {
    if (s.direction == direction && s.end_distance == distance_to_break_out) {
      return s.name;
    }
  }
  std::cout << "GetSegmentName didn't find result." << std::endl;
  return "";
}

std::string RouteBase::GetBreakinSegmentName(bool direction, float distance_to_break_in) {
  for (auto& s : segments_) {
    if (s.direction == direction && s.start_distance == distance_to_break_in) {
      return s.name;
    }
  }
  std::cout << "GetSegmentName didn't find result." << std::endl;
  return "";
}

std::string RouteBase::GetSegmentName(bool direction, float distance_on_route) {
  for (auto& s : segments_) {
    if (s.direction == direction) {
      if (direction) {
        if (distance_on_route <= s.end_distance && distance_on_route > s.start_distance) {
          return s.name;
        }
      } else {
        if (distance_on_route >= s.end_distance && distance_on_route < s.start_distance) {
          return s.name;
        }
      }
    }
  }
  std::cout << "Failed to GetSegmentName." << std::endl;
  return "";
}

void RouteBase::PopulateIntraRouteConnection() {
  // iterate through breakpoints to set in, out vector of SegmentInfo to complete
  // intra route connections. Ready for shortest route search.
  for (auto b : breakpoints_) {
    // find out which Segment on current route
    if (b.type == BREAKPOINT_TYPE::IN) {
      // need to find out peer ConnectionInfo.positive_entering_next_piece
      auto connection_info = b.peer->GetConnectionInfo(GetName());
      bool direction_on_current_route = connection_info.positive_entering_next_piece;
      int segment_index = -1;
      for (int i = 0; i < segments_.size(); i++) {
        if (segments_[i].direction == direction_on_current_route && segments_[i].start_distance == b.distance_on_current_route) {
          segment_index = i;
          break;
        }
      }
      if (segment_index < 0) {
        std::cout << "Segment on current route not found(IN type)." << std::endl;
      }
      // find out peer name and set "in" vec
      segments_[segment_index].in_segment.push_back(b.peer->GetBreakoutSegmentName(connection_info.direction_allowed_to_enter_next_route, connection_info.distance_to_break_out));
    } else {
      // find out which segment on current route
      bool direction = connections_[b.peer->GetName()].direction_allowed_to_enter_next_route;
      int segment_index = -1;
      for (int i = 0; i < segments_.size(); i++) {
        if (segments_[i].direction == direction && segments_[i].end_distance == b.distance_on_current_route) {
          segment_index = i;
          break;
        }
      }
      if (segment_index < 0) {
        std::cout << "Segment on current route not found(OUT type)." << std::endl;
      }
      //find out peer name and set "out" vec
      segments_[segment_index].out_segment.push_back(b.peer->GetBreakinSegmentName(connections_[b.peer->GetName()].positive_entering_next_piece, connections_[b.peer->GetName()].distance_to_break_in));
    }
  }
}

std::vector<SegmentInfo> RouteBase::GetSegments() {
  return segments_;
}

void RouteBase::InsertAircraft(std::shared_ptr<Aircraft> aircraft) {
  aircraft_on_route_.insert(aircraft);
}

void RouteBase::ClearAircraft(std::shared_ptr<Aircraft> aircraft) {
  if (aircraft_on_route_.count(aircraft) == 1) {
    aircraft_on_route_.erase(aircraft);
  }
}

// Update aircraft position.
void RouteBase::ComputePosition(std::shared_ptr<Aircraft> aircraft,
                                float & dist_at_current,
                                bool & direction,
                                const float delta_distance,
                                RouteBase** current,
                                std::list<std::string>& taxi_routes) {
  if (taxi_routes.empty()) return;
  // 1. compute new distantce
  float new_dist = dist_at_current + (direction ? delta_distance :
                                      -delta_distance);
  // 2. determine bound
  std::string second_route = "";
  ConnectionInfo* info = (taxi_routes.size() > 1 ? &connections_[*std::next(
                            taxi_routes.begin())] : nullptr);
  float bound = info ? info->distance_to_break_out : (direction ?
      this->GetLength() : 0);

  // 3. update dist_at_current
  if (direction ? (new_dist < bound) : (new_dist > bound)) {
    // A. no transition
    dist_at_current = new_dist;
  } else {
    // B. transition needed
    if (taxi_routes.size() == 1) {
      // Transition needed but only the current route is in the list,
      // no further routes. So stop at the end of the current route.
      taxi_routes.pop_front();
      dist_at_current = bound;
    } else {
      // The taxi_routes size must be > 1 here. Return empty at begining,
      // ==1 in the above if.
      DEBUG("Transition computed", 0);
      // Check if the current taxi direction is allowed to enter the next route
      if (info && (info->direction_allowed_to_enter_next_route != direction)) {
        std::cerr << "Taxi direction "
                  << (direction ? "Positive" : "Negative")
                  << " of " << taxi_routes.front() << " is not allowed to enter "
                  << *std::next(taxi_routes.begin())
                  << std::endl;
        return;
      }
      // 3.2.1 determine break in distance
      dist_at_current = info->distance_to_break_in;
      new_dist = direction ? (new_dist - bound) : (bound - new_dist);
      // 3.2.2 before update *current, free the aircraft and add it to new route
      (*current)->ClearAircraft(aircraft);

      // 3.2.3 call next piece's compute function
      *current = info->next_piece;
      (*current)->InsertAircraft(aircraft);
      taxi_routes.pop_front();
      direction = info->positive_entering_next_piece;
      info->next_piece->ComputePosition(aircraft, dist_at_current, direction, new_dist, current,
                                        taxi_routes);
    }
  }
}

std::shared_ptr<Aircraft> RouteBase::ClosestAircraftInWay(
                            std::shared_ptr<Aircraft> aircraft,
                            float& search_dist,
                            bool direction,
                            float dist) {
  std::shared_ptr<Aircraft> aircraft_in_the_way = nullptr;
  const std::list<std::string> taxi_routes = aircraft->GetTaxiRoutes();
  float target_distance_from_me = 0;
  float res = 0;
  if (taxi_routes.empty()) {
    std::cerr << "Taxi routes shouldn't be empty" << std::endl;
  }
  RouteBase* current_route = this;
  float end_dist;
  float min_dist = search_dist;

  ConnectionInfo info;
  for (auto iter = taxi_routes.begin(); iter != taxi_routes.end(); ++iter) {
    std::string next_route_string = std::next(iter)!=taxi_routes.end()? *(std::next(iter)) : "";
    ConnectionInfo info = current_route->GetConnectionInfo(next_route_string);
    end_dist = (std::next(iter) != taxi_routes.end()) ? info.distance_to_break_out : (direction ? current_route->GetLength() : 0);
    float low_bound = direction ? std::min(dist, end_dist) : std::max(dist, end_dist);
    float up_bound = direction ? std::max(dist, end_dist) : std::min(dist, end_dist);

    // check if current route has aircraft, find the closest one. if not, update res. if res<0 return nullptr.
    auto vec = current_route->GetAircraftOnRoute();
    if (!vec.empty()) {
      for (auto& a : vec) {
        if (a == aircraft) {
          continue;
        }
        // here we need to use max and min to fix a bug for travel on the negative direction
        if (a->GetDistanceOnRoute() <= std::max(up_bound, low_bound) && a->GetDistanceOnRoute() >= std::min(low_bound, up_bound)) {
          target_distance_from_me = abs(a->GetDistanceOnRoute() - dist) + res;
          if (target_distance_from_me < min_dist) {
            min_dist = target_distance_from_me;
            aircraft_in_the_way = a;
          }
        }
      }
      if (aircraft_in_the_way != nullptr) {
        search_dist = min_dist;
        return aircraft_in_the_way;
      }
    }

    res += abs(end_dist - dist);
    if (res >= search_dist) {
      return nullptr;
    } else {
      if (std::next(iter) != taxi_routes.end()) {
        dist = info.distance_to_break_in;
        direction = info.positive_entering_next_piece;
        current_route = info.next_piece;
      } else {
        return nullptr;
      }
    }
  }
  return nullptr;
}

std::vector<std::shared_ptr<Aircraft>> RouteBase::GetAircraftOnRoute() {
  std::vector<std::shared_ptr<Aircraft>> res;
  for (auto aircraft : aircraft_on_route_) {
    res.push_back(aircraft);
  }
  return res;
}

float RouteBase::GetDistanceToNextHold(const std::list<std::string>& taxi_routes,
                                const float& distance_on_route,
                                const bool& direction_on_route,
                                HoldPointType& next_hold_type) {
  float res = 0; // output
  if (taxi_routes.empty()) {
    std::cerr << "Taxi routes shouldn't be empty" << std::endl;
  }
  RouteBase* current_route = this;
  bool direction = direction_on_route;
  float dist = distance_on_route;
  float end_dist;
  ConnectionInfo info;
  for (auto iter = taxi_routes.begin(); iter != taxi_routes.end(); ++iter) {
    // determine range(end_dist)
    std::string next_route_string = std::next(iter)!=taxi_routes.end()? *(std::next(iter)) : "";
    ConnectionInfo info = current_route->GetConnectionInfo(next_route_string);
    end_dist = (std::next(iter) != taxi_routes.end()) ? info.distance_to_break_out : (direction ? current_route->GetLength() : 0);
    float low_bound = direction ? std::min(dist, end_dist) : std::max(dist, end_dist);
    float up_bound = direction ? std::max(dist, end_dist) : std::min(dist, end_dist);
    // std::cout << "In the loop: lowb " << low_bound << " upb " << up_bound << std::endl;
    auto vec = current_route->GetHoldPoints(direction);

    // If there are multiple hold points on the same route, need to find the closest one.
    float min_dist_to_hold_point_on_route = INT_MAX;
    for (auto& p : vec) {
      if (p.distance_on_route <= std::max(up_bound, low_bound) && p.distance_on_route >= std::min(low_bound, up_bound)) {
        if (abs(dist-p.distance_on_route) < min_dist_to_hold_point_on_route) {
          next_hold_type = p.type;
          min_dist_to_hold_point_on_route = abs(dist-p.distance_on_route);
        }
      }
    }
    if (min_dist_to_hold_point_on_route < INT_MAX) {
      return res + min_dist_to_hold_point_on_route;
    }
    res += abs(end_dist - dist);

    if (std::next(iter) != taxi_routes.end()) {
      dist = info.distance_to_break_in;
      direction = info.positive_entering_next_piece;
      current_route = info.next_piece;
    }
  }
  if (res == 0) {
    next_hold_type = HoldPointType::NOTSET;
  }
  return res;
}

void RouteBase::Reset() {
  aircraft_on_route_.clear();
}

bool RouteBase::AllowTravelInDirection(bool direction) {
  if (one_way_indicator_ == 0) {
    return true;
  }
  else if (one_way_indicator_ == -1) {
    return !direction;
  } else {
    return direction;
  }
}

void RouteBase::SetOneWayDirection(bool direction) {
  if (direction) {
    one_way_indicator_ = 1;
  } else {
    one_way_indicator_ = -1;
  }
}

std::vector<RouteBase*> RouteBase::GetConnectedRouteBreakOutAt(float distance) {
  std::vector<RouteBase*> res;
  for (auto& entry : connections_) {
    if (entry.second.distance_to_break_out == distance) {
      res.push_back(entry.second.next_piece);
    }
  }
  return res;
}

Straightway::Straightway(LineParameter param, sf::RenderWindow* app, sf::Font* font)
  : RouteBase(app, font) {
  route_type_ = param.type;
  length_ = param.length;
  width_ = param.width;
  name_ = param.name;
  ground_color_ = param.ground_color;

  // 1. Determine num_of_points and ds
  int num_of_points = ceil(length_ / param.ds) + 1;
  ds_ = length_ / float(num_of_points - 1);
  DEBUG(ds_, 0);
  DEBUG(num_of_points, 0);

  // 2. Init points
  float theta = param.start_direction / 180 * PI;
  float cos_theta = cos(theta);
  float sin_theta = sin(theta);
  for (int i=0; i<num_of_points; i++) {
    Point p;
    p.index = i;
    p.position = sf::Vector2f(param.start_point.x + ds_ * i * cos_theta, param.start_point.y + ds_ * i * sin_theta);
    p.display_circle.setPointCount(10);
    p.display_circle.setRadius(param.width/2);
    p.display_circle.setOrigin(param.width/2, param.width/2);
    DEBUG(p.display_circle.getRadius(), 0);
    p.display_circle.setPosition(ToSfmlPosition(p.position));
    p.rotation = param.start_direction;
    DEBUG(p.display_circle.getPosition().x, 0);
    DEBUG(p.display_circle.getPosition().y, 0);
    center_points_.push_back(p);
  }

  // 3. Init rect shape
  straightway_rect_.setSize(sf::Vector2f(length_, width_));
  straightway_rect_.setOrigin(0, width_ / 2);
  straightway_rect_.rotate(-1 * param.start_direction);
  straightway_rect_.move(ToSfmlPosition(GetStartPosition()));
  straightway_rect_.setFillColor(ground_color_);
}

Runway::Runway(LineParameter param, RunwayDetailsParam details_param, sf::RenderWindow* app, sf::Font* font, std::string airport_letter)
  : Straightway(param, app, font) {
  taxi_speed_limit_ = param.taxi_speed_limit;
  SetupText();
  runway_names_.push_back("+"+param.name);
  runway_names_.push_back("-"+param.name);
  InitializeRunwayInfo(param, details_param, airport_letter);
  InitializeDisplayDetails(param, details_param);
}

std::vector<std::string> Runway::GetRunwayNames() {
  return runway_names_;
}

std::vector<std::string> Runway::GetRunwayCallingNames() {
  return calling_names_;
}

void Runway::InitializeRunwayInfo(LineParameter param, RunwayDetailsParam details_param, std::string airport_letter) {
  RunwayInfo info_positive, info_negative;
  info_positive.route = this; info_negative.route = this;
  info_positive.direction = true; info_negative.direction = false;
  info_positive.airport_letter = airport_letter;
  info_negative.airport_letter = airport_letter == "L" ? "R" : (airport_letter == "C" ? "C" : "L");
  info_positive.degree = param.start_direction; info_negative.degree = param.start_direction + 180;
  info_positive.internal_name = "+" + param.name; info_negative.internal_name = "-" + param.name;

  // compute runway number
  int degree = int(param.start_direction) % 360;
  if (degree < 0) { degree += 360; }
  degree -= 90;
  degree = 360 - degree;
  degree = degree % 180;
  degree = degree / 10;
  int degree_2 = degree + 18;
  info_positive.runway_number = degree; info_negative.runway_number = degree_2;

  info_positive.calling_name = std::to_string(degree) + info_positive.airport_letter;
  info_negative.calling_name = std::to_string(degree_2) + info_negative.airport_letter;

  info_positive.touch_down_distance_range = std::vector<float>(3);
  info_negative.touch_down_distance_range = std::vector<float>(3);
  info_positive.touch_down_distance_range[0] = details_param.touch_down_indicator_to_runway_end_distance;
  info_positive.touch_down_distance_range[1] = details_param.touch_down_indicator_to_runway_end_distance +
                                               (details_param.touch_down_indicator_main_number - 1) * details_param.touch_down_indicator_interval +
                                               details_param.touch_down_indicator_length * details_param.touch_down_indicator_main_to_normal_ratio / 2;
  info_positive.touch_down_distance_range[2] = details_param.touch_down_indicator_to_runway_end_distance +
                                               (details_param.num_of_touch_down_indicator - 1) * details_param.touch_down_indicator_interval +
                                               details_param.touch_down_indicator_length / 2;
  info_negative.touch_down_distance_range[0] = param.length - info_positive.touch_down_distance_range[0];
  info_negative.touch_down_distance_range[1] = param.length - info_positive.touch_down_distance_range[1];
  info_negative.touch_down_distance_range[2] = param.length - info_positive.touch_down_distance_range[2];

  runway_info_.push_back(std::make_shared<RunwayInfo>(info_positive));
  runway_info_.push_back(std::make_shared<RunwayInfo>(info_negative));
}

void Runway::InitializeDisplayDetails(LineParameter param, RunwayDetailsParam details_param) {
  // 1. threshold markings
  float interval = (param.width/2 - details_param.num_of_threshold_markings/2 * details_param.threshold_marking_width - 2*details_param.threshold_marking_out_interval) / (details_param.num_of_threshold_markings/2 -1);

  for (int i=0; i<details_param.num_of_threshold_markings; i++) {
    sf::RectangleShape mark, mark1;
    mark = sf::RectangleShape(sf::Vector2f(details_param.threshold_marking_length, details_param.threshold_marking_width));
    mark.setFillColor(sf::Color::White);
    float origin_y;
    if (i < details_param.num_of_threshold_markings / 2 ) {
      origin_y = details_param.threshold_marking_out_interval + (i+1)*(details_param.threshold_marking_width + interval) - interval;
    } else {
      origin_y = - details_param.threshold_marking_out_interval - (i- details_param.num_of_threshold_markings / 2)*(details_param.threshold_marking_width+interval);
    }
    mark.setOrigin(0, origin_y);
    mark.rotate(-1*param.start_direction);
    mark1 = mark;
    mark.move(ToSfmlPosition(this->GetBreakOutPosition(details_param.threshold_marking_to_runway_end_distance)));
    mark1.move(ToSfmlPosition(this->GetBreakOutPosition(param.length - details_param.threshold_marking_to_runway_end_distance - details_param.threshold_marking_length)));
    threshold_markings_.push_back(mark);
    threshold_markings_.push_back(mark1);
  }

  // 2. runway number
  int degree = runway_info_[0]->runway_number;
  int degree_2 = runway_info_[1]->runway_number;
  sf::Text t1, t2, l1, l2;
  t1.setFont(*font_);
  t1.setCharacterSize(details_param.runway_number_character_size);
  t2 = t1;
  l1 = t1;
  l2 = t1;

  t1.setString(std::to_string(degree));
  t2.setString(std::to_string(degree_2));
  t1.setOrigin(t1.getLocalBounds().width/2 + 3, 0);
  t2.setOrigin(t2.getLocalBounds().width/2 + 3, 0);
  t1.rotate(degree * 10);
  t2.rotate(degree_2 * 10);
  t1.setPosition(ToSfmlPosition(this->GetBreakOutPosition(details_param.runway_number_to_runway_end_distance)));
  t2.setPosition(ToSfmlPosition(this->GetBreakOutPosition(param.length - details_param.runway_number_to_runway_end_distance)));

  l1.setString(runway_info_[0]->airport_letter);
  l2.setString(runway_info_[1]->airport_letter);
  l1.setOrigin(t1.getLocalBounds().width/2 - 6, 0);
  l2.setOrigin(t2.getLocalBounds().width/2 - 6, 0);
  l1.rotate(degree * 10);
  l2.rotate(degree_2 * 10);
  l1.setPosition(ToSfmlPosition(this->GetBreakOutPosition(details_param.runway_number_letter_to_runway_end_distance)));
  l2.setPosition(ToSfmlPosition(this->GetBreakOutPosition(param.length - details_param.runway_number_letter_to_runway_end_distance)));

  runway_numbers_.push_back(t1);
  runway_numbers_.push_back(t2);
  calling_names_.push_back(runway_info_[0]->calling_name);
  calling_names_.push_back(runway_info_[1]->calling_name);
  runway_letters_.push_back(l1);
  runway_letters_.push_back(l2);

  // 3. Touch down indicator
  for (int i = 0; i< details_param.num_of_touch_down_indicator; i++) {
    sf::RectangleShape rt, rt2, rt3, rt4;
    rt.setSize(sf::Vector2f(details_param.touch_down_indicator_length, details_param.touch_down_indicator_width));
    rt.setOrigin(details_param.touch_down_indicator_length / 2, details_param.touch_down_indicator_width / 2);
    rt.rotate(-1*param.start_direction);
    if (i == details_param.touch_down_indicator_main_number - 1) {
      rt.setScale(details_param.touch_down_indicator_main_to_normal_ratio, details_param.touch_down_indicator_main_to_normal_ratio);
    }
    rt3 = rt;
    rt.setPosition(ToSfmlPosition(this->GetBreakOutPosition(details_param.touch_down_indicator_length / 2 + details_param.touch_down_indicator_to_runway_end_distance + i*details_param.touch_down_indicator_interval)));
    rt2 = rt;
    rt3.setPosition(ToSfmlPosition(this->GetBreakOutPosition(param.length - details_param.touch_down_indicator_length / 2 - details_param.touch_down_indicator_to_runway_end_distance - i*details_param.touch_down_indicator_interval)));
    rt4 = rt3;
    rt.move(ToSfmlPosition(sf::Vector2f(param.width*cos((param.start_direction + 90) * PI / 180) / 4, param.width*sin((param.start_direction+90) * PI / 180) / 4)));
    rt2.move(ToSfmlPosition(sf::Vector2f(param.width*cos((param.start_direction - 90) * PI / 180) / 4, param.width*sin((param.start_direction-90) * PI / 180) / 4)));
    rt3.move(ToSfmlPosition(sf::Vector2f(param.width*cos((param.start_direction + 90) * PI / 180) / 4, param.width*sin((param.start_direction+90) * PI / 180) / 4)));
    rt4.move(ToSfmlPosition(sf::Vector2f(param.width*cos((param.start_direction - 90) * PI / 180) / 4, param.width*sin((param.start_direction-90) * PI / 180) / 4)));
    touch_down_indicators_.push_back(rt);
    touch_down_indicators_.push_back(rt2);
    touch_down_indicators_.push_back(rt3);
    touch_down_indicators_.push_back(rt4);
  }

  // 4. center line
  float cl_pos = details_param.center_line_to_runway_end_distance;
  while (cl_pos <= param.length - details_param.center_line_to_runway_end_distance) {
    sf::RectangleShape cl;
    cl.setSize(sf::Vector2f(details_param.center_line_length, details_param.center_line_width));
    cl.setFillColor(sf::Color::White);
    cl.setOrigin(0, details_param.center_line_width/2);
    cl.rotate(-1*param.start_direction);
    cl.move(ToSfmlPosition(this->GetBreakOutPosition(cl_pos)));
    center_lines_.push_back(cl);
    cl_pos += details_param.center_line_length * 2;
  }
}

void Runway::SetupText() {
  int point_index = center_points_.size() / 2;
  text_.setString(name_);
  text_.setCharacterSize(50);
  text_.setFillColor(sf::Color::Red);
  text_.setPosition(ToSfmlPosition(center_points_[point_index].position));
  text_.setOrigin(sf::Vector2f(text_.getLocalBounds().width / 2,
                               text_.getLocalBounds().height / 2));
}

void Runway::Draw(bool display_text) {
  app_->draw(straightway_rect_);
  if (display_text) {
    app_->draw(text_);
  }
  for (auto& t : threshold_markings_) {
    app_->draw(t);
  }
  for (auto& t : runway_numbers_) {
    app_->draw(t);
  }
  for (auto& l : runway_letters_) {
    app_->draw(l);
  }
  for (auto& t : touch_down_indicators_) {
    app_->draw(t);
  }
  for (auto& t : center_lines_) {
    app_->draw(t);
  }
  for (auto& hp : hold_point_rects_) {
    app_->draw(hp);
  }
}

std::vector<std::shared_ptr<RunwayInfo>> Runway::GetRunwayInfo() {
  return runway_info_;
}

Runway::~Runway() {

}

Taxiway::Taxiway(LineParameter param, sf::RenderWindow* app, sf::Font* font)
  : Straightway(param, app, font) {
  taxi_speed_limit_ = param.taxi_speed_limit;
  SetupText();
}

void Taxiway::Draw(bool display_text) {
  app_->draw(straightway_rect_);
  if (display_text) {
    app_->draw(text_);
  }
  for (auto& hp : hold_point_rects_) {
    app_->draw(hp);
  }
}

Taxiway::~Taxiway() {

}

void Taxiway::SetupText() {
  int point_index = center_points_.size() / 2;
  text_.setString(name_);
  text_.setCharacterSize(50);
  text_.setFillColor(sf::Color::Red);
  text_.setPosition(ToSfmlPosition(center_points_[point_index].position));
  text_.setOrigin(sf::Vector2f(text_.getLocalBounds().width/2, text_.getLocalBounds().height/2));
}

Arcway::Arcway(ArcParameter param, sf::RenderWindow* app, sf::Font* font)
  : RouteBase(app, font) {
  route_type_ = param.type;
  length_ = param.radius * param.center_angle / 180 * PI;
  width_ = param.width;
  name_ = param.name;
  ground_color_ = param.ground_color;
  taxi_speed_limit_ = param.taxi_speed_limit;

  // 1. determine arcway center position
  float start_to_center_degree = param.start_direction + (param.left_curve ? 90 : -90);
  float cos_center = cos(start_to_center_degree / 180 * PI);
  float sin_center = sin(start_to_center_degree / 180 * PI);
  float center_x = param.start_point.x + param.radius * cos_center;
  float center_y = param.start_point.y + param.radius * sin_center;
  center_position_.x = center_x;
  center_position_.y = center_y;

  // 2. determine num_of_points and ds
  int num_of_points = ceil(length_ / param.ds) + 1;
  ds_ = length_ / float(num_of_points - 1);
  DEBUG(ds_, 0);
  DEBUG(num_of_points, 0);

  // 3. init points
  float dtheta = ds_ / param.radius;
  float center_to_start_radian = (start_to_center_degree + 180) / 180 * PI;
  for (int i=0; i<num_of_points; i++) {
    Point p;
    p.index = i;
    float theta = center_to_start_radian + (param.left_curve ? i : -i) * dtheta;
    float x_pos = center_x + param.radius * cos(theta);
    float y_pos = center_y + param.radius * sin(theta);
    p.position = sf::Vector2f(x_pos, y_pos);
    p.display_circle.setPointCount(10);
    p.display_circle.setRadius(param.width / 2);
    p.display_circle.setOrigin(param.width / 2, param.width / 2);
    DEBUG(p.display_circle.getRadius(), 0);
    p.display_circle.setPosition(ToSfmlPosition(p.position));

    p.rotation = param.start_direction + (param.left_curve ? i : -i) * dtheta * 180.0 / PI;

    DEBUG(p.display_circle.getPosition().x, 0);
    DEBUG(p.display_circle.getPosition().y, 0);
    center_points_.push_back(p);
  }

  // 3.5 init arcway point's convexshape
  for (int i=0; i<num_of_points; i++) {
    float theta = center_to_start_radian + (param.left_curve ? i : -i) * dtheta;

    // push back outer vertex
    sf::Vertex v;
    v.color = ground_color_;
    float x_pos = center_x + ( param.radius + param.width/2) * cos(theta);
    float y_pos = center_y + ( param.radius + param.width/2) * sin(theta);
    v.position = ToSfmlPosition(sf::Vector2f(x_pos, y_pos));
    vertices_.push_back(v);

    // push back inner vertex
    x_pos = center_x + ( param.radius - param.width/2) * cos(theta);
    y_pos = center_y + ( param.radius - param.width/2) * sin(theta);
    v.position = ToSfmlPosition(sf::Vector2f(x_pos, y_pos));
    vertices_.push_back(v);
  }

  // 4. setup text
  SetupText();
}

void Arcway::SetupText() {
  text_.setFont(*font_);
  text_.setString(name_);
  text_.setCharacterSize(50);
  text_.setFillColor(sf::Color::Red);
  text_.setPosition(ToSfmlPosition(center_position_));
  text_.setOrigin(sf::Vector2f(text_.getLocalBounds().width/2, text_.getLocalBounds().height/2));
}

void Arcway::Draw(bool display_text) {
  // The new way to draw arcway: draw TriangleStrip. This saves draw call and
  // uses much less memory.
  app_->draw(&vertices_[0], vertices_.size(), sf::TriangleStrip);
  if (display_text) {
    app_->draw(text_);
  }
  for (auto& hp : hold_point_rects_) {
    app_->draw(hp);
  }
}

Gate::Gate(GateParameter param, sf::RenderWindow* app, sf::Font* font)
  : Straightway({param.ds, param.start_direction, param.length, param.width,
                 param.type, param.start_point, param.name}, app, font),
    display_length_(param.display_length) {
    straightway_rect_.setSize(sf::Vector2f(display_length_, width_));
    straightway_rect_.setFillColor(param.ground_color);
    straightway_rect_.setOutlineColor(sf::Color::White);
    straightway_rect_.setOutlineThickness(-2);
    taxi_speed_limit_ = param.taxi_speed_limit;
    size_ = param.size;
    SetupText();
}

void Gate::Draw(bool display_text) {
  app_->draw(straightway_rect_);
  app_->draw(text_);
  for (auto& hp : hold_point_rects_) {
    app_->draw(hp);
  }
}

void Gate::SetupText() {
  text_.setFont(*font_);
  text_.setString(name_);
  text_.setCharacterSize(30);
  text_.setFillColor(sf::Color::Green);
  text_.setPosition(ToSfmlPosition(center_points_.back().position));
  text_.setOrigin(sf::Vector2f(text_.getLocalBounds().width/2, text_.getLocalBounds().height/2));
}

void Gate::AddPushBackRoute(std::string runway, RouteBase* route_) {
  push_back_route_[runway] = route_;
}

RouteBase* Gate::GetPushBackRoute(std::string runway) {
  return (push_back_route_.count(runway) == 1) ? push_back_route_[runway] : nullptr;
}

void Gate::AddTaxiToRunwayList(std::string runway, std::list<std::string> taxi_to_runway_list) {
  taxi_to_runway_route_[runway] = taxi_to_runway_list;
}

std::list<std::string> Gate::GetTaxiToRunwayList(std::string runway) {
  return (taxi_to_runway_route_.count(runway) == 1) ? taxi_to_runway_route_[runway] : std::list<std::string>();
}

bool Gate::IsAvailable() {
  return assigned_aircraft_.empty();
}

void Gate::AssignAircraft(std::shared_ptr<Aircraft> aircraft) {
  assigned_aircraft_.push_back(aircraft);
  text_.setFillColor(sf::Color::Red);
}

void Gate::Free(std::shared_ptr<Aircraft> aircraft) {
  auto iter = assigned_aircraft_.begin();
  while (iter != assigned_aircraft_.end()) {
    if (*iter == aircraft) {
      break;
    }
    iter++;
  }
  if (iter != assigned_aircraft_.end()) {
    assigned_aircraft_.erase(iter);
  }
  if (assigned_aircraft_.empty()) {
    text_.setFillColor(sf::Color::Green);
  }
}

int Gate::GetSize() {
  return size_;
}

int Gate::GetAssignedAircraftNumber() {
  return assigned_aircraft_.size();
}

void Gate::Reset() {
  RouteBase::Reset();
  assigned_aircraft_.clear();
  text_.setFillColor(sf::Color::Green);
}
