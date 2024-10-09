#ifndef BANNER_H
#define BANNER_H

#include <memory>
#include "Aircraft.h"
#include "Airport.h"
#include "Utils.h"

#include <TGUI/TGUI.hpp>

class Aircraft;

class Banner
{
  public:
    Banner(sf::Vector2f top_left_pos, float width, float height,
           int num_of_rows, int character_size, sf::RenderWindow* app,
           sf::Font* font, std::shared_ptr<Aircraft> aircraft,
           tgui::ScrollablePanel::Ptr panel, std::shared_ptr<Airport> airport);
    ~Banner();

    int GetNumOfRows();

    void SetText(std::string text, int line_number);

    bool Contains(sf::Vector2f position);
    sf::Vector2f GetPosition();
    void SetPosition(sf::Vector2f position);
    void RequestRunway();
    void RequestPushBack();
    void RequestTakeOff();
    void RequestGate();
    void TurnOnManualTaxiHold();
    void TurnOffManualTaxiHold();
    void TurnOnManualTaxiResume();
    void DisableGateSelector();
    std::shared_ptr<Airport> GetAirport();

  protected:

  private:
    sf::RenderWindow* app_;
    sf::Font* font_;

    sf::Vector2f top_left_position_;
    float width_;
    float height_;
    int num_of_rows_;
    int character_size_;

    std::vector<sf::Text> text_;
    sf::RectangleShape background_;
    sf::Color default_outline_color_ = sf::Color::White;
    sf::Color default_text_color_ = sf::Color::White;

    std::shared_ptr<Aircraft> aircraft_;

    // tgui code
    tgui::ScrollablePanel::Ptr parent_panel_;
    tgui::Panel::Ptr panel_;
    tgui::Button::Ptr pushback_;
    tgui::Button::Ptr lineup_;
    tgui::Button::Ptr takeoff_;
    std::vector<tgui::Label::Ptr> labels_;
    tgui::ComboBox::Ptr gate_selector_;
    tgui::ComboBox::Ptr runway_selector_;

    tgui::Button::Ptr taxi_hold_;

    std::shared_ptr<Airport> airport_;

};

#endif // BANNER_H
