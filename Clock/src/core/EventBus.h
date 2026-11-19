#ifndef EventBus_h
#define EventBus_h

#include <Arduino.h>
#include "EventTask.h"
#include "Sprite.h"

class EventBus {
  private:
    EventTask* _subscriptions[10];
    uint8_t _subNum = 0;

  public:
    void broadcast(EventType event, Sprite* sender);
    void subscribe(EventTask* task);
    void unsubscribe(EventTask* task);
    
};


#endif
