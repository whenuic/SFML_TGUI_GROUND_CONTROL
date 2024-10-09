#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <iostream>

#include <SFML/Graphics.hpp>
#include "Airport.h"
#include "RouteBase.h"
#include "StateMachine.h"
#include "Utils.h"

struct AircraftFlyingStates {
  float speed;
  float elevation;
  float heading;
  float speed_derivative;
  float elevation_derivative;
  float heading_derivative;
  sf::Vector2f position;
};

struct AircraftFlyingProperties {
  float max_accel;
  float max_daccel;
};

struct AircraftGroundStates {
  float speed;
  RouteBase* route;
  bool direction_on_route;
  float distance_to_route_head;
  float heading; // can be derived by route + direction
  sf::Vector2f position; // Real world coordinates, can be derived by route + distance
};

struct AircraftIdentification {
  std::string name;
  std::string model;
  std::string texture_file;
  float length;
  float width;
  float max_ground_deacceleration;
};

struct AircraftFlyingDisplay {

};

struct AircraftGroundDisplay {

};

class Aircraft : public std::enable_shared_from_this<Aircraft>
{
  enum STAGE {
    TAXI_TO_GATE = 0,
    AT_GATE = 1,
    REQUEST_PUSH_BACK = 2,
    PUSH_BACK = 3,
    TAXI_TO_RUNWAY = 4,
    TAKE_OFF = 5,
  };

  enum STATE {
    IDLE = 0,
    START = 1,
    STOP = 2,
    MAINTAIN = 3,
  };

  friend class StateMachine;
  friend class State;
  friend class Banner;
  friend class InitialState;
  friend class TouchDownState;
  friend class MaintainSpeedState;
  friend class StopState;
  friend class IdleState;
  friend class RequestRunwayState;
  friend class RequestPushBackState;
  friend class PushBackState;
  friend class TakeOffState;
  friend class HoldState;
  friend class LeavingState;

  public:
    Aircraft(AircraftIdentification id, sf::RenderWindow* app, sf::Font* font, std::shared_ptr<Airport> airport);
    virtual ~Aircraft();

    // Get set properties
    std::string GetName();
    std::string GetModel();
    float GetWidth();
    float GetLength();
    int GetSize(); // 1-small, 2-medium and 3-large

    void SetSpeed(float speed);
    float GetSpeed();
    sf::Vector2f GetPosition();

    void SetTaxiRoutes(std::list<std::string> routes);
    std::list<std::string> GetTaxiRoutes();
    std::string GetTaxiRoutesString();

    RouteBase* GetRoute();
    bool GetDirectionOnRoute();
    float GetDistanceOnRoute();

    void SetLandingRunwayInfo(std::shared_ptr<RunwayInfo>);
    std::shared_ptr<RunwayInfo> GetLandingRunwayInfo();
    void SetLandingRunwayString(std::string landing_runway_string);

    void SetGroundRoute(RouteBase* route, bool direction, float distance);

    void SetAcceleration(float accel);
    float GetMaxGroundDeacceleration();

    std::string GetStageString();

    void ComputeDistanceToBreak();
    void Draw();


    void PushBackClearanceReceived();
    bool IsPushBackRequestSent();


    sf::FloatRect GetGlobalBounds();
    bool Intersect(std::shared_ptr<Aircraft> another_aircraft);


    void Activate();
    void Deactivate();
    bool IsActive();
    void Delete();
    bool CanBeDeleted();

    void SetCircleIndicatorColor(sf::Color color);


  protected:

  private:
    float DetermineMaxTaxiSpeed(RouteType type);
    float DetermineAcceleration(float target_speed, float dt,
                                float max_acceleration_allowed,
                                float max_deacceleration_allowed);

  private:
    sf::RenderWindow* app_;
    sf::Font* font_;

    std::string name_;
    std::string model_;
    float length_;
    float width_;
    int size_;  // 1-small, 2-medium, 3-large

    float max_ground_deacceleration_; // negative number
    float soft_ground_deacceleration_ = -0.5;
    float max_ground_acceleration_ = 2.0;
    float soft_ground_acceleration_ = 0.5;
    float speed_; // meter per second
    float acceleration_ = 0.0;

    sf::Texture texture_;

    // Ground display
    sf::Sprite sprite_;

    sf::CircleShape position_circle_;
    sf::CircleShape circle_indicator_; // used to indicate when game over

    sf::Text tag_;

    std::unordered_set<std::shared_ptr<Aircraft>> aircrafts_nearby_;

    std::list<std::string> taxi_routes_;

    RouteBase* route_ = nullptr;
    bool direction_on_route_;
    float distance_on_route_;
    float distance_to_break_ = -1; // this is the critical position to break before enter the corner or stop.

    float max_taxi_speed_straight_ = KnotsToMetersPerSecond(30.0);
    float max_taxi_speed_arc_ = KnotsToMetersPerSecond(10.0);
    float max_gate_speed_ = KnotsToMetersPerSecond(1.2);

    bool active_ = false;
    bool can_be_deleted_ = false;

    STAGE stage_;
    STATE state_;
    bool request_sent_ = false;
    float target_speed_;

    float max_time_at_gate_ = 60; // seconds
    float time_at_gate_ = 0.0;

    RouteBase* gate_ = nullptr;
    std::shared_ptr<RunwayInfo> landing_runway_info_ = nullptr;
    std::string landing_runway_string_ = "";

    std::unordered_map<std::string, RouteBase*> name_to_route_map_;

    bool request_of_push_back_sent_ = false;
    bool clearance_of_push_back_received_ = false;

    bool request_of_runway_sent_ = false;
    bool take_off_runway_assigned_ = false;
    std::string take_off_runway_ = ""; // in the form of "+R1" or "-R1"


    bool request_of_take_off_sent_ = false;
    bool clearance_of_line_up_received_ = false;
    bool clearance_of_take_off_received_ = false;

    bool manual_taxi_hold_ = false;

    bool request_of_gate_sent_ = false;
    std::string gate_assigned_ = "";

    HoldPointType next_hold_type_ = HoldPointType::NOTSET;
    float distance_to_next_hold_ = -1; // negative number indicating needs to compute again

    std::shared_ptr<Airport> airport_ = nullptr;
};

#endif // AIRCRAFT_H
