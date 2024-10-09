#ifndef ROUTEBASE_H
#define ROUTEBASE_H
#include <SFML/Graphics.hpp>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <memory>


#define PI 3.1415926536

#define debugging_enabled 0

#define DEBUG(x, my_switch) do { \
  if (debugging_enabled || my_switch) { std::cout << #x << ": " << x << std::endl; } \
} while (0)


class Aircraft;
class RouteBase;

enum RouteType {
  RUNWAY,
  TAXIWAY,
  ARCWAY,
  GATE,
};

enum HoldPointType {
  NOTSET, // Initial empty value
  TAKEOFF, // before enter the runway for takeoff
  TRAFFIC, // before enter the other route to avoid the traffic
  LINEUP, // On the runway, before take off
};

// Each runway has two runway infos if both directions are used.
struct RunwayInfo {
  // Pointer of the route
  RouteBase* route;
  // Internal direction, decided at the construction time in airport.cpp
  bool direction;
  // Calling name, name called, e.g., 28L
  std::string calling_name;
  // Internal name, e.g., "+R1", "-R1"
  std::string internal_name;
  // Airport letter, when two runways are in the same direction, airport will assign a letter to it, e.g., "R", "L", "C"
  std::string airport_letter;
  // heading degree, this is decided by the param when constructing the runway
  float degree;
  // runway number, e.g., 4, 28, 31
  int runway_number;
  // Ok to touch down distance. Contains three number, 0: close to the end of runway, 1:ideal, 2: close to the middle of runway
  std::vector<float> touch_down_distance_range;
};

// Params used to define a runway Route
struct RunwayDetailsParam {
  // the vertical lines at both ends of the runway
  int num_of_threshold_markings;
  float threshold_marking_width;
  float threshold_marking_length;
  float threshold_marking_out_interval;
  float threshold_marking_to_runway_end_distance;

  // center line settings
  float center_line_width;
  float center_line_length;
  float center_line_to_runway_end_distance; // where's the first center line start

  // touch down indicator
  int num_of_touch_down_indicator;
  float touch_down_indicator_width;
  float touch_down_indicator_length;
  float touch_down_indicator_to_runway_end_distance;
  float touch_down_indicator_interval; // from the first starting point to the second start point. not from first end point to second start point
  int touch_down_indicator_main_number; // main indicator is this number from the end of the runway
  int touch_down_indicator_main_to_normal_ratio; // main indicator's length is this times the normal length

  // runway number
  int runway_number_character_size;
  float runway_number_to_runway_end_distance;
  float runway_number_letter_to_runway_end_distance;
};

// Used to set aircraft positions.
struct Point {
  int index;
  sf::Vector2f position; // real world coordinates
  sf::CircleShape display_circle;
  float rotation; // curve direction in real world degree
};

// used to hold the aircraft when taxiing
struct HoldPoint {
  RouteBase* route;
  float distance_on_route;
  HoldPointType type;
  // if taxiing this direction, need to hold. If hold on both direction, two
  // hold points will be held by the RouteBase.
  bool direction;
  std::string hold_for_take_off_runway; // if type is ::TAKEOFF, this string sets which runway is hold for
};

// Using real world coordinate
struct LineParameter {
  float ds; // distance between two consecutive points
  float start_direction; // degrees, real world coordinates
  float length;
  float width;
  RouteType type;
  sf::Vector2f start_point; // real world coordinates
  std::string name;
  sf::Color ground_color;
  float taxi_speed_limit; // in knots
};

// Using real world coordinate
struct GateParameter {
  float ds; // distance between two consecutive points
  float start_direction; // degrees, real world coordinates
  float length; // dist between entering point and aircraft final stop position
  float width;
  float display_length; // must be larger than length, the actual size of parking spot
  RouteType type;
  sf::Vector2f start_point; // real world coordinates
  std::string name; // probably gate name, e.g., "D15"
  sf::Color ground_color;
  float taxi_speed_limit; // in knots
  int size; // 1-small, 2-medium, 3-large
};

// Center is determined by start_point, start_direction and radius
struct ArcParameter {
  float ds;
  float start_direction; // degrees, real world coordinates
  bool left_curve; // looking at the start direction, true iff curve goes left
  float radius;
  float center_angle; // degrees
  float width;
  RouteType type;
  sf::Vector2f start_point; // real world coordinates
  std::string name;
  sf::Color ground_color;
  float taxi_speed_limit; // in knots
};

// Stores all the connection info for segment
  // Ready for building the airport connection matrix
  struct SegmentInfo {
    // provide ability finding back
    RouteBase* route;
    // unique name: route_name + on_route_id + "+/-", e.g., R1s1, R1 is route name, s1 is segment name on the route, "+/-" is segment travel direction
    std::string name;
    // unique id in matrix, will be signed by the Airport class
    int matrix_id;
    // Length of this segment
    float length;
    // direction on route
    bool direction;
    // start distance
    float start_distance;
    // end distance
    float end_distance;
    // In vector
    std::vector<std::string> in_segment;
    // Out vector
    std::vector<std::string> out_segment;
  };

class RouteBase {
  struct ConnectionInfo {
    // Direction on the current route that is allowed to enter the next route
    bool direction_allowed_to_enter_next_route;
    // T breaks out at 500, when entering B, at B 60.
    float distance_to_break_out;
    // Pointer of next piece.
    RouteBase* next_piece;
    // When entering the next piece, the distance at next piece.
    float distance_to_break_in;
    // True-increase distance when entering next piece,
    // False-decrease distance when entering next piece.
    bool positive_entering_next_piece;
  };

  enum BREAKPOINT_TYPE {
    IN,
    OUT
  };

  // Stores all the connection info on the route
  // Ready for building the SegmentInfo
  struct BreakpointInfo {
    // Type. in or out
    BREAKPOINT_TYPE type;
    // Upstream or downstream RouteBase*
    RouteBase* peer;
    // Distance on current route
    float distance_on_current_route;
  };

  public:
    RouteBase(sf::RenderWindow* app, sf::Font* font);

    // Return the position of distance 0
    sf::Vector2f GetStartPosition();

    // Return the total length that an aircraft can travel of this route piece
    float GetLength() { return length_; }

    // Update aircraft position
    void ComputePosition(std::shared_ptr<Aircraft> aircraft,
                         float& dist_at_current,
                         bool& direction,
                         const float delta_distance,
                         RouteBase** current,
                         std::list<std::string>& taxi_routes);

    std::string GetName() { return name_; }
    RouteType GetRouteType() { return route_type_; }

    float GetRotation(float dis, bool direction);

    sf::Vector2f GetEndPosition() { return center_points_.back().position; }

    sf::Vector2f GetBreakOutPosition(float dis);

    ConnectionInfo GetConnectionInfo(std::string route) {
      return connections_[route];
    }

    float GetTaxiSpeedLimit() { return taxi_speed_limit_; }

    virtual void Draw(bool display_text) = 0;

    void ConnectRoute(float my_break_out_distance,
                      bool direction_allowed_to_enter_next_route,
                      RouteBase * next_route,
                      float next_break_in_distance,
                      bool positive_entering_next_route);

    float GetDistanceToNextHold(const std::list<std::string>& taxi_routes,
                                const float& distance_on_route,
                                const bool& direction_on_route,
                                /*output*/HoldPointType& next_hold_type);

    void AddHoldPoint(HoldPoint hold_point);
    std::vector<HoldPoint> GetHoldPoints(bool direction);

    void InsertAircraft(std::shared_ptr<Aircraft> aircraft);
    void ClearAircraft(std::shared_ptr<Aircraft> aircraft);

    std::shared_ptr<Aircraft> ClosestAircraftInWay(
                            std::shared_ptr<Aircraft> aircraft,
                            float& search_dist,
                            bool direction,
                            float dist);

    std::vector<std::shared_ptr<Aircraft>> GetAircraftOnRoute();

    virtual void Reset();

    bool AllowTravelInDirection(bool direction);

    void SetOneWayDirection(bool direction);

    void InsertBreakpoint(BreakpointInfo breakpoint_info);

    void CreateSegments();

    std::string GetBreakoutSegmentName(bool direction, float distance_to_break_out);

    std::string GetBreakinSegmentName(bool direction, float distance_to_break_in);

    // Based on ConnectionInfo and BREAKPOINT::IN, set intra-route
    // in, out info on SegmentInfo
    void PopulateIntraRouteConnection();

    // If the point is on joint of two consecutive segments,
    // return the upstream one.
    std::string GetSegmentName(bool direction, float distance_on_route);

    std::vector<SegmentInfo> GetSegments();

    // get the next route ptr breaking out at input distance. used to find the next route after push back done.
    std::vector<RouteBase*> GetConnectedRouteBreakOutAt(float distance);


  protected:
    // Setup route element text for display
    virtual void SetupText() = 0;

  protected:
    sf::RenderWindow* app_;
    sf::Font* font_;

    // route params
    RouteType route_type_;
    float ds_;
    float length_;
    float width_;

    float taxi_speed_limit_; // in knots

    // route text
    sf::Text text_;
    std::string name_;
    sf::Color ground_color_;

    // route positioning points, for computing aircraft position purpose, all
    // display points, such as outer line, lights and notes are stored in
    // dedicated containers.
    std::vector<Point> center_points_;

    // route lights

    // ConnectionInfo
    std::unordered_map<std::string, ConnectionInfo> connections_;

    // hold points, separated by direction, if +, sorted increasingly, if -, sorted decreasingly
    std::unordered_map<bool, std::vector<HoldPoint>> direction_to_hold_point_;
    std::vector<sf::RectangleShape> hold_point_rects_;

    // store all the aircrafts currently on this route.
    std::unordered_set<std::shared_ptr<Aircraft>> aircraft_on_route_;

    // one way indicator. 0:both ways, 1:+, -1:-
    int one_way_indicator_ = 0;

    // Breakpoints in sorted order, the one closes to 0 will appear first.
    std::vector<BreakpointInfo> breakpoints_;

    // SegmentInfo, seg1+, seg1-, seg2+, seg2-, ...
    // for one-way route, still in this order seg1+, seg2+, ... or seg1-, seg2-, ...
    std::vector<SegmentInfo> segments_;
};

class Arcway : public RouteBase {
  public:
    Arcway(ArcParameter param, sf::RenderWindow* app, sf::Font* font);

    void Draw(bool display_text) override;

  private:
    void SetupText() override;

    sf::Vector2f center_position_;

    std::vector<sf::Vertex> vertices_;

};

class Straightway : public RouteBase {
  public:
    Straightway(LineParameter param, sf::RenderWindow* app, sf::Font* font);

    virtual void Draw(bool display_text) = 0;

  protected:
    virtual void SetupText() = 0;

  protected:
    // Runway display params
    sf::RectangleShape straightway_rect_;
};

class Runway : public Straightway {
  public:
    // The airport letter applies to the direction of the LineParameter, the other end's letter is the opposite of the input letter.
    Runway(LineParameter param, RunwayDetailsParam details_param,
           sf::RenderWindow* app, sf::Font* font, std::string airport_letter);
    ~Runway();

    void Draw(bool display_text) override;
    std::vector<std::string> GetRunwayNames();
    std::vector<std::string> GetRunwayCallingNames();
    std::vector<std::shared_ptr<RunwayInfo>> GetRunwayInfo();


  private:
    void SetupText() override;
    void InitializeDisplayDetails(LineParameter param, RunwayDetailsParam details_param);
    void InitializeRunwayInfo(LineParameter param, RunwayDetailsParam details_param, std::string airport_letter);

  private:
    // For Draw purpose
    std::vector<sf::RectangleShape> threshold_markings_;
    std::vector<sf::Text> runway_numbers_; // stores 04 28
    std::vector<sf::Text> runway_letters_; // stores L R C
    std::vector<sf::RectangleShape> touch_down_indicators_;
    std::vector<sf::RectangleShape> center_lines_;

    std::vector<std::string> runway_names_; // stores +R1, -R1
    std::vector<std::string> calling_names_; // stores 18L, 27C, etc.

    // RunwayInfo
    std::vector<std::shared_ptr<RunwayInfo>> runway_info_;
};

class Taxiway : public Straightway {
  public:
    Taxiway(LineParameter param, sf::RenderWindow* app, sf::Font* font);
    ~Taxiway();

    void Draw(bool display_text) override;

  private:
    void SetupText() override;

  private:
    // Taxiway display params
    sf::RectangleShape taxiway_rect_;

    // Taxiway lights params
};

class Gate : public Straightway {
  public:
    Gate(GateParameter param, sf::RenderWindow* app, sf::Font* font);

    void Draw(bool display_text) override;
    void AddPushBackRoute(std::string runway, RouteBase* route);
    RouteBase* GetPushBackRoute(std::string runway);
    void AddTaxiToRunwayList(std::string runway, std::list<std::string> taxi_to_runway_list);
    std::list<std::string> GetTaxiToRunwayList(std::string runway);
    void AssignAircraft(std::shared_ptr<Aircraft> aircraft);
    bool IsAvailable();
    void Free(std::shared_ptr<Aircraft> aircraft);
    int GetSize();
    int GetAssignedAircraftNumber();
    void Reset() override;

  private:
    void SetupText() override;

  private:
    float display_length_;

    std::unordered_map<std::string, RouteBase*> push_back_route_;
    std::unordered_map<std::string, std::list<std::string>> taxi_to_runway_route_;

    std::list<std::shared_ptr<Aircraft>> assigned_aircraft_;

    int size_; // see definition in RouteBase.h
};


#endif // ROUTEBASE_H
