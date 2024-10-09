#include "Aircraft.h"
#include "Utils.h"
#include "math.h"

Aircraft::Aircraft(AircraftIdentification id, sf::RenderWindow* app, sf::Font* font, std::shared_ptr<Airport> airport)
  : app_(app),
    font_(font),
    name_(id.name),
    model_(id.model),
    length_(id.length),
    width_(id.width),
    max_ground_deacceleration_(id.max_ground_deacceleration),
    airport_(airport) {
  if (!texture_.loadFromFile(id.texture_file)) {
    std::cout << "Aircraft texture load failed: " << id.texture_file << std::endl;
  }
  texture_.setSmooth(true);
  sf::Vector2u texture_size = texture_.getSize();
  sprite_.setTexture(texture_);
  sprite_.setOrigin(texture_size.x / 2, texture_size.y / 2);
  sprite_.setScale(length_ / texture_size.x, width_ / texture_size.y);
  sprite_.setPosition(sf::Vector2f(rand()%5000 + 5000, rand()%5000 + 5000));

  position_circle_.setRadius(5);
  position_circle_.setFillColor(sf::Color::Red);
  position_circle_.setOrigin(5,5);

  tag_.setFont(*font_);
  tag_.setString(name_);
  tag_.setCharacterSize(25);
  tag_.setOrigin(tag_.getLocalBounds().width/2, tag_.getLocalBounds().height/2);
  tag_.setFillColor(sf::Color::Yellow);

  stage_ = STAGE::TAXI_TO_GATE;
  state_ = STATE::MAINTAIN;

  if (route_) {
    target_speed_ = KnotsToMetersPerSecond(route_->GetTaxiSpeedLimit());
  }

  circle_indicator_.setFillColor(sf::Color::Transparent);
  circle_indicator_.setOutlineThickness(3);
  circle_indicator_.setOutlineColor(sf::Color::Transparent);
  float circle_indicator_radius = 15;
  circle_indicator_.setRadius(circle_indicator_radius);
  circle_indicator_.setOrigin(circle_indicator_radius, circle_indicator_radius);

  // determine size, for gate selection use
  if (length_ <= 25 && width_ <= 25) {
    size_ = 1;
    max_time_at_gate_ = 120;
  } else if (length_ <= 45 && width_ <= 45) {
    size_ = 2;
    max_time_at_gate_ = 240;
  } else {
    size_ = 3;
    max_time_at_gate_ = 360;
  }
}

void Aircraft::SetSpeed(float speed) { speed_ = speed; }
float Aircraft::GetSpeed() { return speed_; }
void Aircraft::SetAcceleration(float accel) { acceleration_ = accel; }
float Aircraft::GetMaxGroundDeacceleration() { return max_ground_deacceleration_; }
std::string Aircraft::GetName() { return name_; }
std::string Aircraft::GetModel() { return model_; }
void Aircraft::PushBackClearanceReceived() { clearance_of_push_back_received_ = true; }
bool Aircraft::IsPushBackRequestSent() { return request_of_push_back_sent_; }
std::string Aircraft::GetStageString() {
  switch (stage_) {
    case STAGE::TAXI_TO_GATE:
      return "TAXI_TO_GATE";
    case STAGE::AT_GATE:
      return "AT_GATE";
    case STAGE::REQUEST_PUSH_BACK:
      return "REQUEST_PUSH_BACK";
    case STAGE::PUSH_BACK:
      return "PUSH_BACK";
    case STAGE::TAXI_TO_RUNWAY:
      return "TAXI_TO_RUNWAY";
    case STAGE::TAKE_OFF:
      return "TAKE_OFF";
    default:
      std::cerr << "Stage string is not set" << std::endl;
      return "";
  }
}

void Aircraft::SetTaxiRoutes(std::list<std::string> routes) { taxi_routes_ = routes; }
std::string Aircraft::GetTaxiRoutesString() {
  std::string routes = "";
  for (auto& r : taxi_routes_) {
    routes += r;
    routes += ">";
  }
  if (routes.size() >= 1 && routes.substr(routes.size()-1, 1) == ">") {
    routes = routes.substr(0, routes.size() - 1);
  }
  return routes;
}

void Aircraft::SetGroundRoute(RouteBase* route, bool direction, float dist) {
  if (route_) {
    route_->ClearAircraft(shared_from_this());
  }
  route_ = route;
  route_->InsertAircraft(shared_from_this());
  target_speed_ = KnotsToMetersPerSecond(route_->GetTaxiSpeedLimit());
  direction_on_route_ = direction;
  distance_on_route_ = dist;
  name_to_route_map_.insert({route_->GetName(), route_});
}

float Aircraft::DetermineAcceleration(float target_speed, float dt,
                                      float max_acceleration_allowed,
                                      float max_deacceleration_allowed) {
  return (target_speed > speed_) ?
          std::min((target_speed - speed_) / dt, max_acceleration_allowed) :
          std::max((target_speed - speed_) / dt, max_deacceleration_allowed);
}

void Aircraft::ComputeDistanceToBreak() {
  // current speed
  // next speed
  // v_2^2 - v_1^2 = 2 * a * s;
  int taxi_routes_size = taxi_routes_.size();

  if (taxi_routes_size == 0) distance_to_break_ = -1;
  if (taxi_routes_size > 1) {
    float current_speed_limit = route_->GetTaxiSpeedLimit();
    float next_speed_limit = route_->GetConnectionInfo(*std::next(taxi_routes_.begin())).next_piece->GetTaxiSpeedLimit();
    if (next_speed_limit > current_speed_limit) {
      distance_to_break_ = -1;
    } else {
      float s = (next_speed_limit*next_speed_limit - current_speed_limit*current_speed_limit)/2/soft_ground_deacceleration_;
      if(direction_on_route_) {
        distance_to_break_ = route_->GetLength() - s;
      } else {
        distance_to_break_ = s;
      }
    }
  }
  if (taxi_routes_size == 1) {
    float current_speed_limit = route_->GetTaxiSpeedLimit();
    float s = current_speed_limit*current_speed_limit/2/soft_ground_deacceleration_;
    if(direction_on_route_) {
      distance_to_break_ = route_->GetLength() - s;
    } else {
      distance_to_break_ = s;
    }
  }
}

void Aircraft::Draw() {
  if (!active_) {
    return;
  }
  app_->draw(sprite_);
  tag_.setPosition(sprite_.getPosition() + sf::Vector2f(0, 30));
  app_->draw(tag_);
  position_circle_.setPosition(sprite_.getPosition());
  app_->draw(position_circle_);
  circle_indicator_.setPosition(sprite_.getPosition());
  app_->draw(circle_indicator_);
}

sf::FloatRect Aircraft::GetGlobalBounds() {
  return sprite_.getGlobalBounds();
}

bool Aircraft::Intersect(std::shared_ptr<Aircraft> another_aircraft) {
  sf::Rect<float> rect;
  float r1 = std::min(width_, length_) / 2;
  float r2 = std::min(another_aircraft->GetWidth(), another_aircraft->GetLength()) / 2;
  auto pos1 = GetPosition();
  auto pos2 = another_aircraft->GetPosition();
  auto res = sqrt((pos1.x-pos2.x)*(pos1.x-pos2.x) + (pos1.y-pos2.y)*(pos1.y-pos2.y)) <= r1 + r2;
  if (res) {
    std::cout << "Distance: " << sqrt((pos1.x-pos2.x)*(pos1.x-pos2.x) + (pos1.y-pos2.y)*(pos1.y-pos2.y)) << "r1+r2 = " << r1 + r2 << std::endl;
  }
  return res;
  // auto res = sprite_.getGlobalBounds().intersects(another_aircraft->GetGlobalBounds(), rect);
}

sf::Vector2f Aircraft::GetPosition() {
  return sprite_.getPosition();
}

float Aircraft::GetWidth() {
  return width_;
}

float Aircraft::GetLength() {
  return length_;
}

RouteBase* Aircraft::GetRoute() {
  return route_;
}

bool Aircraft::GetDirectionOnRoute() {
  return direction_on_route_;
}

float Aircraft::GetDistanceOnRoute() {
  return distance_on_route_;
}

std::list<std::string> Aircraft::GetTaxiRoutes() {
  return taxi_routes_;
}

void Aircraft::SetLandingRunwayInfo(std::shared_ptr<RunwayInfo> runway_info) {
  landing_runway_info_ = runway_info;
}

std::shared_ptr<RunwayInfo> Aircraft::GetLandingRunwayInfo() {
  return landing_runway_info_;
}

void Aircraft::SetLandingRunwayString(std::string landing_runway_string) {
  landing_runway_string_ = landing_runway_string;
}

void Aircraft::Activate() {
  active_ = true;
}

void Aircraft::Deactivate() {
  active_ = false;
}

bool Aircraft::IsActive() {
  return active_;
}

void Aircraft::Delete() {
  can_be_deleted_ = true;
}

bool Aircraft::CanBeDeleted() {
  return can_be_deleted_;
}

void Aircraft::SetCircleIndicatorColor(sf::Color color) {
  circle_indicator_.setOutlineColor(color);
}

int Aircraft::GetSize() {
  return size_;
}

Aircraft::~Aircraft() {
  //dtor
}

