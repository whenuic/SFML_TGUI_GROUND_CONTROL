#include "Banner.h"
#include <TGUI/TGUI.hpp>

Banner::Banner(sf::Vector2f top_left_pos, float width, float height,
               int num_of_rows, int character_size,
               sf::RenderWindow* app, sf::Font* font,
               std::shared_ptr<Aircraft> aircraft,
               tgui::ScrollablePanel::Ptr panel,
               std::shared_ptr<Airport> airport)
  : app_(app),
    font_(font),
    top_left_position_(top_left_pos),
    width_(width),
    height_(height),
    num_of_rows_(num_of_rows),
    character_size_(character_size),
    text_(std::vector<sf::Text>(num_of_rows)),
    aircraft_(aircraft),
    airport_(airport) {
    for (int i=0; i<num_of_rows_; i++) {
      text_[i].setFont(*font_);
      text_[i].setPosition(top_left_position_.x + 3, top_left_position_.y + i * height/num_of_rows_);
      text_[i].setCharacterSize(character_size_);
    }
    background_.setPosition(top_left_position_);
    background_.setSize(sf::Vector2f(width, height));
    background_.setFillColor(sf::Color::Transparent);
    background_.setOutlineThickness(-1);
    background_.setOutlineColor(default_outline_color_);

    // tgui code
    parent_panel_ = panel;
    panel_ = tgui::Panel::create();
    parent_panel_->add(panel_);
    panel_->setSize(width_, height);
    panel_->getRenderer()->setBackgroundColor(sf::Color::White);
    panel_->getRenderer()->setBorders(2);
    panel_->getRenderer()->setBorderColor(sf::Color::Black);
    panel_->setPosition(top_left_position_);

    for (int i=0; i<3; i++) {
      labels_.push_back(tgui::Label::create());
      panel_->add(labels_.back());
      labels_.back()->setSize(width_/2, height_/3);
      labels_.back()->setPosition("0%", std::to_string(int(100/3*i)) + "%");
      labels_.back()->getRenderer()->setBackgroundColor(sf::Color::Green);
    }

    pushback_ = tgui::Button::create();
    panel_->add(pushback_);
    pushback_->getRenderer()->setBackgroundColor(sf::Color::Yellow);
    pushback_->setSize(width/2/3, height_/2);
    pushback_->setPosition("66.66%", "25%");
    pushback_->setTextSize(10);
    pushback_->setEnabled(false);
    pushback_->setVisible(false);

    lineup_ = tgui::Button::create();
    panel_->add(lineup_);
    lineup_->getRenderer()->setBackgroundColor(sf::Color::Yellow);
    lineup_->setSize(width/2/3, height_/2);
    lineup_->setPosition("66.66%", "25%");
    lineup_->setTextSize(10);
    lineup_->setEnabled(false);
    lineup_->setVisible(false);

    takeoff_ = tgui::Button::create();
    panel_->add(takeoff_);
    takeoff_->getRenderer()->setBackgroundColor(sf::Color::Green);
    takeoff_->setSize(width/2/3, height_/2);
    takeoff_->setPosition("83.33%", "25%");
    takeoff_->setTextSize(10);
    takeoff_->setEnabled(false);
    takeoff_->setVisible(false);

    taxi_hold_ = tgui::Button::create();
    panel_->add(taxi_hold_);
    taxi_hold_->getRenderer()->setBackgroundColor(sf::Color::Red);
    taxi_hold_->setSize(width/2/3, height_/2);
    taxi_hold_->setPosition("66.66%", "25%");
    taxi_hold_->setTextSize(10);
    taxi_hold_->setText("HOLD");
    taxi_hold_->connect("pressed", [&](){ // std::cout << "Taxi hold pressed." << std::endl;
                                        if (taxi_hold_->getText() == "RESUME") {
                                          taxi_hold_->setText("HOLD");
                                          taxi_hold_->getRenderer()
                                              ->setBackgroundColor(
                                                  sf::Color::Red);
                                        } else {
                                          taxi_hold_->setText("RESUME");
                                          taxi_hold_->getRenderer()
                                              ->setBackgroundColor(
                                                  sf::Color::Green);
                                        }
                                        aircraft_->manual_taxi_hold_ = !aircraft_->manual_taxi_hold_;
                                      });
    taxi_hold_->setEnabled(false);
    taxi_hold_->setVisible(false);

    gate_selector_ = tgui::ComboBox::create();
    panel_->add(gate_selector_);
    for (auto& g : airport_->GetGates()) {
      // match size
      if (static_cast<Gate*>(g)->GetSize() >= aircraft_->GetSize()) {
        gate_selector_->addItem(g->GetName());
      }
    }
    gate_selector_->setSize(width_/2, height_/3);
    gate_selector_->setPosition("0%", std::to_string(int(100/3*2)) + "%");
    gate_selector_->setEnabled(false);
    gate_selector_->setVisible(false);

    runway_selector_ = tgui::ComboBox::create();
    panel_->add(runway_selector_);
    for (auto& str : airport_->GetActiveRunwayStrings()) {
      runway_selector_->addItem(str);
    }
    runway_selector_->setSize(width_/2, height_/3);
    runway_selector_->setPosition("0%", std::to_string(int(100/3*2)) + "%");
    runway_selector_->setEnabled(false);
    runway_selector_->setVisible(false);
}

int Banner::GetNumOfRows() { return num_of_rows_; }

void Banner::SetPosition(sf::Vector2f position) {
  top_left_position_ = position;
  // background, text, textbox
  for (int i=0; i<num_of_rows_; i++) {
    text_[i].setPosition(top_left_position_.x + 3, top_left_position_.y + i * height_/num_of_rows_);
  }
  background_.setPosition(top_left_position_);
  panel_->setPosition(top_left_position_);
}

sf::Vector2f Banner::GetPosition() {
  return top_left_position_;
}

// input line_number is 1 based. The first line from the user's perspective is line 1.
void Banner::SetText(std::string text, int line_number) {
  if (line_number <= num_of_rows_) {
    text_[line_number-1].setString(text);
    labels_[line_number - 1]->setText(text);
  }
}

bool Banner::Contains(sf::Vector2f position) {
  auto pos = background_.getPosition();
  return position.x >= pos.x && position.x <= pos.x + width_ &&
      position.y >= pos.y && position.y <= pos.y + height_;
}

void Banner::RequestRunway() {
  SetText("REQ_RWY | " + aircraft_->gate_->GetName(), 2);

  runway_selector_->setVisible(true);
  runway_selector_->setEnabled(true);
  runway_selector_->connect("ItemSelected", [&](){ aircraft_->take_off_runway_assigned_ = true;
                                                   auto display_name = runway_selector_->getSelectedItem();
                                                   aircraft_->take_off_runway_ = airport_->GetRunwayInternalName(display_name);
                                                   runway_selector_->setVisible(false);
                                                   runway_selector_->setEnabled(false);
                                                 });
}

void Banner::RequestPushBack() {
  SetText("REQUEST_PUSH_BACK", 2);
  //tgui code
  pushback_->setVisible(true);
  pushback_->setEnabled(true);
  pushback_->setText("PUSH\nBACK");
  pushback_->connect("pressed", [&](){ if (aircraft_->IsPushBackRequestSent()) {
                                           aircraft_->PushBackClearanceReceived();
                                       }
                                       pushback_->setEnabled(false);
                                       pushback_->setVisible(false);
                                       // std::cout<<"Pushback pressed." << std::endl;
                                     });
}

void Banner::RequestTakeOff() {
  SetText("REQUEST_TAKE_OFF", 2);

  //tgui code
  lineup_->setVisible(true);
  lineup_->setEnabled(true);
  lineup_->setText("LINEUP");
  lineup_->connect("pressed", [&](){ if (aircraft_->request_of_take_off_sent_) {
                                         if (!aircraft_->clearance_of_line_up_received_) {
                                           aircraft_->clearance_of_line_up_received_ = true;
                                         }
                                       }
                                       lineup_->setEnabled(false);
                                       lineup_->setVisible(false);
                                       // std::cout << "lineup pressed." << std::endl;
                                     });

  takeoff_->setVisible(true);
  takeoff_->setEnabled(true);
  takeoff_->setText("TAKE\nOFF");
  takeoff_->connect("pressed", [&](){ if (aircraft_->request_of_take_off_sent_) {
                                         aircraft_->clearance_of_line_up_received_ = true;
                                         aircraft_->clearance_of_take_off_received_ = true;
                                       }
                                       // Need to disable the lineup button
                                       lineup_->setEnabled(false);
                                       lineup_->setVisible(false);
                                       takeoff_->setEnabled(false);
                                       takeoff_->setVisible(false);
                                       // std::cout << "takeoff pressed." << std::endl;
                                     });
}

void Banner::RequestGate() {
  // std::cout<<"Banner Request gate called." << std::endl;
  SetText("REQ GATE | Rwy: " + aircraft_->landing_runway_info_->calling_name, 2);
  gate_selector_->setEnabled(true);
  gate_selector_->setVisible(true);
  gate_selector_->connect("ItemSelected", [&](){
    // Without this if check, if the gate select time expires and automatic assignment is made, and the selector is clicked but selection click is after the automatic assignment, then the selected gate will be marked NOT available. But the aircraft will not use the selected gate. Hence, this gate is locked forever.
    if (aircraft_->gate_assigned_ == "") {
      std::string selected_gate = gate_selector_->getSelectedItem();
      static_cast<Gate*>(airport_->GetRoutePtr(selected_gate))->AssignAircraft(aircraft_);
      aircraft_->gate_assigned_ = selected_gate;
      gate_selector_->setEnabled(false);
      gate_selector_->setVisible(false);
      SetText("GATE ASSIGNED: " + selected_gate, 2);
    }
  });
}

void Banner::DisableGateSelector() {
  gate_selector_->setEnabled(false);
  gate_selector_->setVisible(false);
}

void Banner::TurnOnManualTaxiHold() {
  taxi_hold_->setEnabled(true);
  taxi_hold_->setVisible(true);
}

void Banner::TurnOffManualTaxiHold() {
  taxi_hold_->setEnabled(false);
  taxi_hold_->setVisible(false);
}

void Banner::TurnOnManualTaxiResume() {
  taxi_hold_->setEnabled(true);
  taxi_hold_->setVisible(true);
  aircraft_->manual_taxi_hold_ = true;
  taxi_hold_->setText("RESUME");
  taxi_hold_->getRenderer()->setBackgroundColor(sf::Color::Green);
}

std::shared_ptr<Airport> Banner::GetAirport() {
  return airport_;
}

Banner::~Banner() {
  // std::cout << "Banner destructor called.";
  parent_panel_->remove(panel_);
}
