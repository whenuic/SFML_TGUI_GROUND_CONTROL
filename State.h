#ifndef STATE_H
#define STATE_H

#include "SFML/Graphics.hpp"
#include <memory>

class Aircraft;
class RouteBase;
class BannerPanel;

class State {
  public:
    State(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel)
     : aircraft_(aircraft), panel_(panel) {};

    virtual std::string Entry() { return state_name_; };
    virtual std::string Update(float dt) { return state_name_; };
    virtual void Exit() {};
    virtual std::string GetStateName() { return state_name_; }

  protected:
    std::shared_ptr<Aircraft> aircraft_;
    std::shared_ptr<BannerPanel> panel_;
    std::string state_name_ = "";
};

class InitialState : public State {
  public:
    InitialState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Entry() override;
    std::string Update(float dt) override;
  private:
    float timer_ = 0;
    float before_landing_interval = 120; // 2 mins before landing
};

class TouchDownState : public State {
  public:
    TouchDownState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Entry() override;
  private:

};

class MaintainSpeedState : public State {
  public:
    MaintainSpeedState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Update(float dt) override;
    std::string Entry() override;
    void Exit() override;

  private:
    RouteBase* current_route_ = nullptr;
    RouteBase* next_route_ = nullptr;
    float current_route_speed_limit_ = 0;
    float next_route_speed_limit_ = 0;

};

class StopState : public State {
  public:
    StopState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Entry() override;
    std::string Update(float dt) override;
    void Exit() override;
};

class IdleState : public State {
  public:
    IdleState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Update(float dt) override;
    std::string Entry() override;
    void Exit() override;
  private:
    float timer_ = 0;
};

class RequestRunwayState : public State {
  public:
    RequestRunwayState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Update(float dt) override;
    std::string Entry() override;
};

class RequestPushBackState : public State {
  public:
    RequestPushBackState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Update(float dt) override;
    std::string Entry() override;
};

class PushBackState : public State {
  public:
    PushBackState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Update(float dt) override;
    std::string Entry() override;
    void Exit() override;

  private:
    float push_back_speed_ = 0;
};

class TakeOffState : public State {
  public:
    TakeOffState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Update(float dt) override;
    std::string Entry() override;
};

class HoldState : public State {
  public:
    HoldState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Update(float dt) override;
    std::string Entry() override;
};

class LeavingState : public State {
  public:
    LeavingState(std::shared_ptr<Aircraft> aircraft, std::shared_ptr<BannerPanel> panel);
    std::string Update(float dt) override;
    std::string Entry() override;
};
#endif // STATE_H
