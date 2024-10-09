#ifndef AIRPORT_H
#define AIRPORT_H

#include <memory>
#include <SFML/Graphics.hpp>
#include "RouteBase.h"

struct LandingPositionInfo {
  RouteBase* runway;
  bool direction; // direction on runway
  float distance; // distance on runway
};

class Airport
{
  public:
  Airport(sf::RenderWindow* app, sf::Font* font, float global_ds,
           float runway_width, sf::Color runway_color, float taxiway_width,
           sf::Color taxiway_color, float gate_length, float gate_width,
           float gate_display_length, sf::Color gate_color, bool mode);
    ~Airport();

    // Add a route to the airport
    void AddRunway(LineParameter param, RunwayDetailsParam details_param, std::string airport_letter);
    void AddTaxiway(LineParameter param);
    void AddArcway(ArcParameter param);
    void AddGate(GateParameter param);
    void AddHoldPoint(HoldPoint hold_point);

    // Connect two routes, update break point
    // direction means: direction on the current route to enter the second
    // ConnectRoute() will be called twice inside this function's implementation.
    // If any route is a one-way route and not allowed to enter the other, the corresponding ConnectRoute() will not be called.
    void Connect(std::string route_name_1, bool direction_1, float dist_1,
                 std::string route_name_2, bool direction_2, float dist_2);

    // Get route shared pointer by route name
    RouteBase* GetRoutePtr(std::string route_name);

    // Get all available gates
    std::vector<RouteBase*> GetAvailableGates();

    // Get all gates, even include unavailable ones
    std::vector<RouteBase*> GetGates();

    // Get all runways
    std::vector<RouteBase*> GetRunways();
    // Get active runways strings, due to wind
    std::vector<std::string> GetActiveRunwayStrings();
    // Get active runway info, due to wind
    std::vector<std::shared_ptr<RunwayInfo>> GetActiveRunwayInfo();

    // Return a list of taxi commands
    std::list<std::string> ComputeRouteTo(RouteBase* from_route,
                                          float dist_on_from,
                                          RouteBase* to_route,
                                          float dist_on_to);

    std::string GetRunwayInternalName(std::string calling_name);

    // Draw the airport
    void Draw();

    // turn on/off road text display. No matter what, Gate text will still be displayed.
    void FlipRoadText();

    // Get gates vector with specific size
    std::vector<Gate*> GetGatesWithExactSize(int size);

    // Reset states of airport, to work with game restart
    void Reset();

    RouteBase* GetSegmentRoute(std::string segment_name);

    std::list<std::string> GetRoute(RouteBase* start_route, bool start_direction, float start_dist,
                                    RouteBase* end_route, bool end_direction, float end_dist);

    std::list<std::string> Dijkstra(std::vector<std::vector<float>> graph, int src, int dst);

    // Return the holdpoint that matches the input takeoff runway name, e.g., +R1, -R1
    HoldPoint GetHoldPoint(std::string take_off_runway);

    HoldPoint GetLineUpPoint(std::string take_off_runway);

    // Return landing info based on input runway, taking wind direction into account
    LandingPositionInfo GetLandingPositionInfo(RouteBase* landing_runway);

    // Set wind direction, [0, 360)
    void SetWindDirection(float wind_direction);
    float GetWindDirection();


  private:
    // After the routes are completed, build this matrix for route searching algorithm.
    // A route element, e.g., taxiway, can have multiple entries in the matrix if they
    // have break in/out points. Each segment has two entries in the matrix if they allow
    // traffic in both + and - directions.
    void BuildConnectionMatrix();

  private:
    sf::RenderWindow* app_;
    sf::Font* font_;

    float global_ds_; // meter

    float runway_width_;
    sf::Color runway_color_;

    float taxiway_width_;
    sf::Color taxiway_color_;

    float gate_length_;
    float gate_width_;
    float large_gate_width_;
    float gate_display_length_;
    float large_gate_length_;
    float large_gate_display_length_;
    sf::Color gate_color_;

    std::vector<Runway*> runways_;
    std::vector<Taxiway*> taxiways_;
    std::vector<Gate*> gates_;
    std::vector<Arcway*> arcways_;
    std::vector<HoldPoint> holdpoints_;

    std::unordered_map<std::string, RouteBase*> str_2_ptr_;

    bool display_road_text_ = false;  // even if false, Gate text will still be displayed

    std::vector<SegmentInfo> segments_;
    std::unordered_map<int, std::string> matrix_id_to_name_;
    std::unordered_map<std::string, int> name_to_matrix_id_;
    std::vector<std::vector<float>> connection_matrix_;

    bool mode_; // decided by the wind direction

    std::unordered_map<std::string, std::string> calling_name_to_internal_name_;

    float wind_direction_ = 0; // the direction where wind comes from. North wind is 0, east wind is 90, south 180, west 270;

};

#endif // AIRPORT_H
