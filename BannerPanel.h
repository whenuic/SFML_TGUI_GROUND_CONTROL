#ifndef BANNERPANEL_H
#define BANNERPANEL_H

#include <TGUI/TGUI.hpp>

#include <memory>
#include <map>

#include "Banner.h"
#include "Aircraft.h"
#include "Airport.h"

class Aircraft;
class Banner;

class BannerPanel
{
  public:
    BannerPanel(sf::Vector2f top_left_pos, float width, float height, sf::RenderWindow* app, sf::Font* font, tgui::Gui* gui, std::shared_ptr<Airport> airport);
    virtual ~BannerPanel();

    bool Contains(sf::Vector2f position);
    void CreateBanner(std::shared_ptr<Aircraft> aircraft);
    void RemoveBanner(std::shared_ptr<Aircraft> aircraft);
    void Organize();
    void RequestRunway(std::shared_ptr<Aircraft> aircraft);
    void RequestPushBack(std::shared_ptr<Aircraft> aircraft);
    void RequestTakeOff(std::shared_ptr<Aircraft> aircraft);
    void RequestGate(std::shared_ptr<Aircraft> aircraft);
    void TurnOnManualTaxiHold(std::shared_ptr<Aircraft> aircraft);
    void TurnOffManualTaxiHold(std::shared_ptr<Aircraft> aircraft);
    void TurnOnManualTaxiResume(std::shared_ptr<Aircraft> aircraft);
    std::shared_ptr<Banner> GetBanner(std::shared_ptr<Aircraft> aircraft);
    void Clear();

    // TGUI code
    tgui::ScrollablePanel::Ptr GetPanel();

  private:
    sf::RenderWindow* app_;
    sf::Font* font_;

    sf::Vector2f top_left_position_;
    float width_;
    float height_;
    int character_size_;

    std::map<std::shared_ptr<Aircraft>, std::shared_ptr<Banner>> banners_;
    std::list<std::shared_ptr<Aircraft>> panel_order_list_; // used to track the order of panel
    sf::RectangleShape panel_frame_;
    sf::Color default_outline_color_ = sf::Color::White;
    sf::Color default_text_color_ = sf::Color::White;

    float banner_height_ = 50;

    // -------------------------------------
    tgui::Gui* gui_;
    tgui::ScrollablePanel::Ptr panel_;

    std::shared_ptr<Airport> airport_;
};

#endif // BANNERPANEL_H
