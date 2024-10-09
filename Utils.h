#ifndef UTILS_H
#define UTILS_H

#include <SFML/Graphics.hpp>
#include <random>

inline sf::Vector2f ToSfmlPosition(sf::Vector2f world_coordinate) {
  return sf::Vector2f(world_coordinate.x, -1 * world_coordinate.y);
}

inline float ToSfmlRotation(float world_degree) {
  return -1.0 * world_degree;
}

inline float KnotsToMetersPerSecond(float knot) { return knot * 0.5144; }

inline std::string ConvertSecondsToHHMMSS(float seconds) {
  int second = int(seconds);
  int minute = second / 60;
  second -= (minute * 60);
  int hour = minute / 60;
  minute -= (hour * 60);
  int day = hour / 24;
  hour -= (day * 24);
  int year = day / 365;
  day -= (year * 365);

  return std::to_string(year) + "y" + std::to_string(day) + "d" + "\n" +
         (hour < 10 ? "0" : "") + std::to_string(hour) + ":" +
         (minute < 10 ? "0" : "") + std::to_string(minute) + ":" +
         (second < 10 ? "0" : "") + std::to_string(second);
}

enum ClickResponse {
  NOTHING = 0,
  ASSIGN_GATE = 1,
};

template <typename Iter>
Iter select_randomly(Iter start, Iter end) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  return select_randomly(start, end, gen);
}

#endif // UTILS_H
