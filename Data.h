#ifndef DATA_H
#define DATA_H

#include "Aircraft.h"
struct AircraftIdentification;
class Aircraft;

class Data
{
  public:
    Data();
    virtual ~Data();

    void SetRenderWindow(sf::RenderWindow* app) {app_ = app;}

  protected:
    sf::RenderWindow* app_;

    std::unique_ptr<Aircraft> aircraft_;


  private:
};

#endif // DATA_H
