#pragma once

#include <Arduino.h>
#include "../core/Game.h"
#include "../core/Locator.h"
#include "../core/EventBus.h"
#include "../assets/assets.h"


class Pacman: public Sprite, public EventTask {
  private:

    uint16_t _PACMAN [2][25] = {
      {
        // 'pacman1, 5x5px
        0x0000, 0xFE40, 0xFE40, 0xFE40, 0x0000, 
        0xFE40, 0xFE40, 0x0000, 0xFE40, 0xFE40, 
        0xFE40, 0xFE40, 0xFE40, 0xFE40, 0xFE40, 
        0xFE40, 0xFE40, 0xFE40, 0xFE40, 0xFE40, 
        0x0000, 0xFE40, 0xFE40, 0xFE40, 0x0000
      }, 
      {
        // 'pacman2, 5x5px
        0x0000, 0xFE40, 0xFE40, 0xFE40, 0xFE40, 
        0xFE40, 0xFE40, 0x0000, 0xFE40, 0x0000, 
        0xFE40, 0xFE40, 0xFE40, 0x0000, 0x0000, 
        0xFE40, 0xFE40, 0xFE40, 0xFE40, 0x0000, 
        0x0000, 0xFE40, 0xFE40, 0xFE40, 0xFE40
      }
    };

    byte _iteration = 0;

    int _lastX;
    int _lastY;
    long current_color = 0xFE40;

    const unsigned short* _sprite;
    unsigned long invencibleTimeout = 0;
    

    bool _pacman_anim = true;

    void flip();
    void rotate();
    void changePacmanColor(uint16_t newcolor);

    
  public:
    enum State {
      MOVING,
      STOPPED,
      TURNING,
      INVENCIBLE
    };
    Pacman(int x, int y);
    void init();
    void move(Direction dir);
    void turn(Direction dir);
    void setState(State state);
    int nextBlock();
    void setPosition(int x, int y);
    int getX();
    int getY();
    void update();
    const char* name();
    void execute(EventType event, Sprite* caller);
    Direction _direction = Direction::RIGHT;
    State _state = MOVING;
    const int SPRITE_SIZE = 5;

    
};
