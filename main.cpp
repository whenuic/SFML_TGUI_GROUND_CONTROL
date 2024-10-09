#include <TGUI/TGUI.hpp>
#include <math.h>
#include <iostream>
#include <unordered_map>
#include <list>
#include <stack>
#include <memory>
#include <iomanip>
#include <sstream>

#include "RouteBase.h"
#include "Airport.h"
#include "Aircraft.h"
#include "Utils.h"
#include "StateMachine.h"
#include "BannerPanel.h"
#include "Banner.h"

#define PI 3.1415926536

// 1. Self: ends, directions, meshed points(position, distance to the end), display shapes
// 2. Connection info, [breakout point, next element]
// 3. Computation, next position.

int main() {
  /* initialize random seed: */
  srand(time(NULL));

  // Create the main window
  int screen_width = sf::VideoMode::getDesktopMode().width;
  int screen_height = sf::VideoMode::getDesktopMode().height;

  int ground_view_width = 1500;
  int panel_view_width = 300;
  int window_width = ground_view_width + panel_view_width;
  int window_height = 1000;
  sf::RenderWindow app(sf::VideoMode(window_width, window_height),
                       "GROUND CONTROL");
  //TGUI code
  tgui::Gui gui{app};

  sf::Color ground_background_color(0, 78, 56, 255);
  app.clear(ground_background_color);

  sf::View view;
  float view_zoom = 1;
  view.setCenter(50, 250);
  float real_world_width = 3200;
  float real_world_height = window_height * real_world_width / ground_view_width;

  view.setSize(real_world_width, real_world_height);
  view.setViewport(sf::FloatRect(0.0, 0.0, float(ground_view_width) / float(window_width), 1));
  app.setView(view);

  sf::View panel_view;
  panel_view.setViewport(sf::FloatRect(float(ground_view_width) / float(window_width), 0.0, float(panel_view_width) / float(window_width), 1.0));
  panel_view.setSize(panel_view_width, window_height);
  panel_view.setCenter(panel_view_width / 2.0, window_height/2.0);

  sf::Font font;
  if (!font.loadFromFile("DroidSansMono.ttf")) {
    std::cout << "Font load failed: "
              << "DroidSansMono.ttf" << std::endl;
  }

  sf::Text game_over_text;
  game_over_text.setFont(font);
  game_over_text.setCharacterSize(100);
  game_over_text.setPosition(sf::Vector2f(0, 0));
  game_over_text.setFillColor(sf::Color::Red);
  game_over_text.setString("GAME OVER");


  uint32_t frame_count = 0;

  // Airport params
  float global_ds = 0.5;

  float runway_width = 46;
  sf::Color runway_color = sf::Color(37, 40, 45, 255);

  float taxiway_width = 22;
  sf::Color taxiway_color = sf::Color(150, 150, 150, 255);
  sf::Color transparent_color = sf::Color::Transparent;

  float gate_width = 60;
  float gate_display_length = 80;
  float gate_length = 45;
  // sf::Color gate_color = sf::Color(70, 70, 70, 255);
  sf::Color gate_color = transparent_color;

  bool mode = true;
  std::shared_ptr<Airport> airport = std::make_shared<Airport>(&app, &font, global_ds, runway_width, runway_color,
                  taxiway_width, taxiway_color, gate_length, gate_width,
                  gate_display_length, gate_color, mode);
  airport->SetWindDirection(220);

  // Add an aircraft calling name pool
  std::vector<std::string> calling_name_pool = {
      /*United*/ "UAL",  /*American*/ "AAL", /*Southwest*/ "SWA",
      /*JetBlue*/ "JBU", /*Alaska*/ "ASA",   /*Delta*/ "DAL"};

  // Add aircraft identification pool
  std::vector<AircraftIdentification> aircraft_identification_pool = {
      {"", "A320", "A320neo_CFM_AIB_VT.png", 37.57, 35.8, -3.0},
      {"", "A350", "A350-1000_T.png", 73.79, 64.75, -3.0},
      {"", "A380", "A380.png", 72.85, 79.88, -3.0}};


  std::shared_ptr<BannerPanel> panel = std::make_shared<BannerPanel>(
          sf::Vector2f(ground_view_width, 0), panel_view_width, window_height, &app, &font, &gui, airport);

  std::vector<std::shared_ptr<Aircraft>> aircrafts;
  std::vector<std::unique_ptr<StateMachine>> state_machines;

  aircrafts.push_back(std::make_shared<Aircraft>(AircraftIdentification({"CZ3525", "A320", "A320neo_CFM_AIB_VT.png", 37.57, 35.8, -3.0}), &app, &font, airport));
  aircrafts.back()->SetLandingRunwayInfo(airport->GetActiveRunwayInfo()[0]);
  state_machines.push_back(std::make_unique<StateMachine>(aircrafts.back(), panel));

  // Game params
  float dt = 0.0;
  float dt_scaled = 0.0;
  float time_accumulator = 0.0; // elapsed time in real world, regardless of speed_coeff
  float time_accumulator_scaled = 0.0;
  float last_generate_traffic_time = 0;
  sf::Clock clock;
  float speed_coeff = 1;

  bool is_game_over = false;
  bool is_auto_generate_traffic = true;
  int auto_generate_traffic_interval = 600; // 10 min
  int total_take_off = 0;

  // TGUI code
  tgui::Label::Ptr airport_name_label = tgui::Label::create();
  gui.add(airport_name_label);
  airport_name_label->setSize(490, 60);
  airport_name_label->setPosition(490, window_height-60);
  airport_name_label->setText("Kingston-Norman Manley Airport, Jamaica");
  airport_name_label->setTextSize(20);
  airport_name_label->getRenderer()->setTextColor(sf::Color::White);

  tgui::Button::Ptr zoom_in = tgui::Button::create();
  gui.add(zoom_in);
  zoom_in->setSize(70, 30);
  zoom_in->setPosition(0, 0);
  zoom_in->connect("pressed", [&](){view_zoom*=0.5; view.zoom(0.5);});
  zoom_in->setText("zoom in");

  tgui::Button::Ptr zoom_out = tgui::Button::create();
  gui.add(zoom_out);
  zoom_out->setSize(70, 30);
  zoom_out->setPosition(70, 0);
  zoom_out->connect("pressed", [&](){view_zoom/=0.5; view.zoom(2);});
  zoom_out->setText("zoom out");

  tgui::Button::Ptr flip_text = tgui::Button::create();
  gui.add(flip_text);
  flip_text->setSize(70, 30);
  flip_text->setPosition(140, 0);
  flip_text->connect("pressed", [&]() {
    airport->FlipRoadText();
  });
  flip_text->setText("Flip Text");

  tgui::Label::Ptr simulation_speed_label = tgui::Label::create();
  gui.add(simulation_speed_label);
  simulation_speed_label->setSize(70, 30);
  simulation_speed_label->setPosition(ground_view_width - 55, 0);
  simulation_speed_label->setText(
      speed_coeff >= 1 ? std::to_string(int(speed_coeff)) + " x" : "0.5 x");
  simulation_speed_label->setTextSize(20);
  simulation_speed_label->getRenderer()->setTextColor(sf::Color::Red);

  auto GenerateTraffic = [&]() {
    int random_number = rand() % 6 + 1;
    int aircraft_id_pool_index = random_number <= 4 ? 0 : (random_number == 5 ? 1 : 2);
    AircraftIdentification id =
        aircraft_identification_pool[aircraft_id_pool_index];
    id.name = calling_name_pool[rand() % calling_name_pool.size()] +
              std::to_string(rand() % 899 + 101);
    aircrafts.push_back(std::make_shared<Aircraft>(id, &app, &font, airport));
    auto runway_infos = airport->GetActiveRunwayInfo();
    int random_runway_number = rand() % int(runway_infos.size());
    aircrafts.back()->SetLandingRunwayInfo(runway_infos[random_runway_number]);
    state_machines.push_back(
        std::make_unique<StateMachine>(aircrafts.back(), panel));
  };

  tgui::Button::Ptr land_one = tgui::Button::create();
  gui.add(land_one);
  land_one->setSize(70, 30);
  land_one->setPosition(ground_view_width - 70, 30);
  land_one->setText("+1");
  land_one->getRenderer()->setBorders(3);
  land_one->getRenderer()->setBorderColor(sf::Color::Blue);
  land_one->getRenderer()->setBorderColorHover(sf::Color::Yellow);
  land_one->connect("pressed", GenerateTraffic);

  tgui::Slider::Ptr simulation_speed_slider = tgui::Slider::create();
  gui.add(simulation_speed_slider);
  simulation_speed_slider->setSize(140, 30);
  simulation_speed_slider->setPosition(ground_view_width - 70 - 140, 0);
  simulation_speed_slider->setMinimum(-1);
  simulation_speed_slider->setMaximum(6);
  simulation_speed_slider->setStep(1);
  simulation_speed_slider->setValue(0);
  simulation_speed_slider->connect("valuechanged", [&]() {
    speed_coeff = std::pow(2, simulation_speed_slider->getValue());
    simulation_speed_label->setText(speed_coeff >= 1 ? std::to_string(int(speed_coeff)) + " x" : "0.5 x");
  });

  tgui::Label::Ptr fps_label = tgui::Label::create();
  gui.add(fps_label);
  fps_label->setSize(70, 30);
  fps_label->setPosition(210, 0);
  fps_label->setTextSize(12);
  fps_label->getRenderer()->setTextColor(sf::Color::White);

  tgui::Label::Ptr time_label = tgui::Label::create();
  gui.add(time_label);
  time_label->setSize(70, 30);
  time_label->setPosition(280, 0);
  time_label->setTextSize(12);
  time_label->getRenderer()->setTextColor(sf::Color::White);

  tgui::Label::Ptr total_take_off_label = tgui::Label::create();
  gui.add(total_take_off_label);
  total_take_off_label->setSize(70, 30);
  total_take_off_label->setPosition(350, 0);
  total_take_off_label->setTextSize(12);
  total_take_off_label->getRenderer()->setTextColor(sf::Color::White);

  tgui::Button::Ptr exit_button = tgui::Button::create();
  gui.add(exit_button);
  exit_button->setSize(70, 30);
  exit_button->setPosition(ground_view_width / 2, window_height / 2 + 30);
  exit_button->setText("Exit");
  exit_button->connect("pressed", [&]() { app.close(); });
  exit_button->setVisible(false);

  tgui::Button::Ptr restart_game_button = tgui::Button::create();
  gui.add(restart_game_button);
  restart_game_button->setSize(70, 30);
  restart_game_button->setPosition(ground_view_width / 2, window_height / 2);
  restart_game_button->setText("Restart");
  restart_game_button->connect("pressed", [&]() {
    aircrafts.clear();
    state_machines.clear();
    panel->Clear();
    airport->Reset();
    is_game_over = false;
    time_accumulator = 0;
    time_accumulator_scaled = 0;
    last_generate_traffic_time = 0;
    total_take_off = 0;
    restart_game_button->setVisible(false);
    exit_button->setVisible(false);
    GenerateTraffic();
  });
  restart_game_button->setVisible(false);

  sf::Texture wind_triangle_texture;
  wind_triangle_texture.loadFromFile("wind.png");
  sf::Vector2u wind_triangle_texture_size = wind_triangle_texture.getSize();
  sf::Sprite wind_triangle;
  wind_triangle.setTexture(wind_triangle_texture);
  wind_triangle.setOrigin(wind_triangle_texture_size.x / 2, wind_triangle_texture_size.y / 2);
  wind_triangle.setScale(0.075, 0.075);
  wind_triangle.setPosition(30, 30);
  wind_triangle.rotate(90);
  // wind_triangle.rotate(180);
  tgui::Canvas::Ptr wind_indicator = tgui::Canvas::create();
  gui.add(wind_indicator);
  wind_indicator->setSize(70, 70);
  wind_indicator->setPosition(ground_view_width - 70, 60);
  wind_indicator->clear(ground_background_color);
  wind_indicator->draw(wind_triangle);

  // Start the game loop
  while (app.isOpen()) {
    frame_count++;
    dt = clock.restart().asSeconds();
    time_accumulator += dt;
    dt_scaled = dt * speed_coeff;
    time_accumulator_scaled += dt_scaled;
    if (!is_game_over) {
      time_label->setText(ConvertSecondsToHHMMSS(time_accumulator_scaled));
      fps_label->setText("fps: " + std::to_string(int(round(1 / dt))));
      total_take_off_label->setText("TAKE OFF:\n" + std::to_string(total_take_off));
      if (is_auto_generate_traffic && time_accumulator_scaled - last_generate_traffic_time > auto_generate_traffic_interval) {
        GenerateTraffic();
        last_generate_traffic_time = time_accumulator_scaled;
        auto_generate_traffic_interval = auto_generate_traffic_interval + rand() % 60;
      }
    }

    // 1. Clear screen
    app.clear(ground_background_color);

    app.setView(view);

    airport->Draw();

    for (auto& aircraft : aircrafts) {
      aircraft->Draw();
    }

    wind_triangle.setRotation(airport->GetWindDirection());
    wind_indicator->clear(ground_background_color);
    wind_indicator->draw(wind_triangle);
    gui.draw();

    if (is_game_over) {
      app.draw(game_over_text);
    }

    app.setView(panel_view);

    app.display();

    // 2. Process events
    sf::Event event;
    while (app.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        app.close();
      }

      bool bypass_gui_event_handling = false;

      if (event.type == sf::Event::MouseWheelScrolled) {
        // get panel position
        auto panel_pos = panel->GetPanel()->getAbsolutePosition();
        std::cout << panel_pos.x << " " << panel_pos.y << std::endl;
        auto panel_size = panel->GetPanel()->getSize();
        std::cout << panel_size.x << " " << panel_size.y << std::endl;

        sf::Vector2i mouse_pos = sf::Mouse::getPosition(app);
        std::cout << mouse_pos.x << " " << mouse_pos.y << std::endl;
        if ((mouse_pos.x >= panel_pos.x && mouse_pos.x <= panel_pos.x + panel_size.x) && (mouse_pos.y >= panel_pos.y && mouse_pos.y <= panel_pos.y + panel_size.y)) {
          auto old_bar_value = panel->GetPanel()->getVerticalScrollbarValue();
          auto new_value = old_bar_value - event.mouseWheelScroll.delta * 25;
          // unsigned int may cause problem.
          if (float(old_bar_value) - event.mouseWheelScroll.delta * 25 < 0) {
            new_value = 0;
          }
          panel->GetPanel()->setVerticalScrollbarValue(new_value);
          bypass_gui_event_handling = true;
        }
      }

      if (!bypass_gui_event_handling) {
        gui.handleEvent(event);
      }

      if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          sf::Vector2i mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
          sf::Vector2f translated_pos = app.mapPixelToCoords(mouse_pos);
        }
      }

      if (event.type == sf::Event::KeyPressed) {
        if(event.key.code == sf::Keyboard::Left) {
          view.move(-50*view_zoom, 0);
        } else if (event.key.code == sf::Keyboard::Right) {
          view.move(50*view_zoom, 0);
        } else if (event.key.code == sf::Keyboard::Up) {
          view.move(0, -50*view_zoom);
        } else if (event.key.code == sf::Keyboard::Down) {
          view.move(0, 50*view_zoom);
        }
      }
    }

    if (is_game_over) {
      continue;
    }
    // 3. Update aircraft dynamics
    for (auto& sm : state_machines) {
      sm->Update(dt_scaled);
    }
    std::stack<int> aircraft_deletion_index;
    for (int i=0; i<aircrafts.size(); i++) {
      if (aircrafts[i]->CanBeDeleted()) {
        aircraft_deletion_index.push(i);
      }
    }
    while (!aircraft_deletion_index.empty()) {
      aircrafts.erase(aircrafts.begin() + aircraft_deletion_index.top());
      state_machines.erase(state_machines.begin() + aircraft_deletion_index.top());
      aircraft_deletion_index.pop();
      total_take_off++;
    }

    // 4. Check game over
    for(int i=0; i<aircrafts.size(); i++) {
      if (!aircrafts[i]->IsActive()) {
        continue;
      }
      auto pos1 = aircrafts[i]->GetPosition();
      for (int j=i+1; j<aircrafts.size(); j++) {
        if (!aircrafts[j]->IsActive()) {
          continue;
        }
        auto pos2 = aircrafts[j]->GetPosition();
        // sf::Rect intersects doesn't work well since if rotated, the global bound box doesn't rotate, just expands.
        if (aircrafts[i]->Intersect(aircrafts[j])) {
          std::cout << aircrafts[i]->GetName() << " at: x " << pos1.x << " y " << pos1.y << std::endl;
          std::cout << aircrafts[j]->GetName() << " at: x " << pos2.x << " y " << pos2.y << std::endl;
          is_game_over = true;
          aircrafts[i]->SetCircleIndicatorColor(sf::Color::Red);
          aircrafts[j]->SetCircleIndicatorColor(sf::Color::Red);
          break;
        }
      }
      if (is_game_over) {
        restart_game_button->setVisible(true);
        exit_button->setVisible(true);
        std::cout << "Game over" << std::endl;
        break;
      }

    }
  }

  return EXIT_SUCCESS;
}
