#include <iostream>
#include <math.h>
#include "BannerPanel.h"
#include "Utils.h"



BannerPanel::BannerPanel(sf::Vector2f top_left_pos, float width,
                         float height, sf::RenderWindow* app,
                         sf::Font* font, tgui::Gui* gui,
                         std::shared_ptr<Airport> airport)
  : app_(app),
    font_(font),
    top_left_position_(top_left_pos),
    width_(width),
    height_(height),
    gui_(gui),
    airport_(airport) {
  // tgui code
  panel_ = tgui::ScrollablePanel::create();
  panel_->getRenderer()->setBackgroundColor(sf::Color::Black);
  gui->add(panel_);
  panel_->setSize(width_, height_);
  panel_->setPosition(top_left_position_);
}

std::shared_ptr<Banner> BannerPanel::GetBanner(std::shared_ptr<Aircraft>
    aircraft) {
  return banners_.count(aircraft) > 0 ? banners_[aircraft] : nullptr;
}

void BannerPanel::CreateBanner(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 0) {
    // std::cout << "Banner created for " << aircraft->GetName() << std::endl;
    // existing number, doesn't count the one is being added now.
    int num_of_banners = banners_.size();
    banners_[aircraft] = std::make_shared<Banner>(sf::Vector2f(
                           0, top_left_position_.y +
                           (num_of_banners) * banner_height_), width_,
                           banner_height_, 3, 14, app_, font_, aircraft, panel_, airport_);
    panel_order_list_.push_back(aircraft);
  } else {
    std::cout << "Aircraft: " << aircraft->GetName()
              << " already had a banner." << std::endl;
  }
}

void BannerPanel::RemoveBanner(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 0) return;
  // find iterator, update the rest position, erase this.
  auto iter = std::find(std::begin(panel_order_list_), std::end(panel_order_list_), aircraft);
  iter = panel_order_list_.erase(iter);
  sf::Vector2f old_position = banners_[aircraft]->GetPosition();
  // std::cout << "Old position:" << old_position.x << " " << old_position.y
  //          << std::endl;
  while (iter != panel_order_list_.end()) {
    sf::Vector2f new_position = banners_[*iter]->GetPosition();
    // std::cout << "New position:" << new_position.x << " "
    //           << new_position.y << std::endl;
    banners_[*iter]->SetPosition(old_position);
    old_position = new_position;
    iter++;
  }
  banners_.erase(banners_.find(aircraft));
}

void BannerPanel::RequestRunway(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 1) {
    banners_[aircraft]->RequestRunway();
  }
}

void BannerPanel::RequestPushBack(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 1) {
    banners_[aircraft]->RequestPushBack();
  }
}

void BannerPanel::RequestTakeOff(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 1) {
    banners_[aircraft]->RequestTakeOff();
  }
}

void BannerPanel::RequestGate(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 1) {
    banners_[aircraft]->RequestGate();
  }
}

void BannerPanel::TurnOnManualTaxiHold(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 1) {
    banners_[aircraft]->TurnOnManualTaxiHold();
  }
}

void BannerPanel::TurnOffManualTaxiHold(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 1) {
    banners_[aircraft]->TurnOffManualTaxiHold();
  }
}

void BannerPanel::TurnOnManualTaxiResume(std::shared_ptr<Aircraft> aircraft) {
  if (banners_.count(aircraft) == 1) {
    banners_[aircraft]->TurnOnManualTaxiResume();
  }
}

BannerPanel::~BannerPanel()
{

}

bool BannerPanel::Contains(sf::Vector2f position) {
  auto pos = panel_frame_.getPosition();
  return position.x >= pos.x && position.x <= pos.x + width_ &&
      position.y >= pos.y && position.y <= pos.y + height_;
}

void BannerPanel::Clear() {
  panel_order_list_.clear();
  banners_.clear();
}

tgui::ScrollablePanel::Ptr BannerPanel::GetPanel() {
  return panel_;
}
