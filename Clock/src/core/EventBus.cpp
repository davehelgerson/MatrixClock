#include "EventBus.h"

void EventBus::broadcast(EventType event, Sprite* sender) {
  for (uint8_t i = 0; i < _subNum; i++) {
    _subscriptions[i]->execute(event, sender);
  }
}

void EventBus::subscribe(EventTask* task) {
  // Sprites call subscribe() from init(), which runs on every clockface setup
  // (and every minute for the Mario minute block), so ignore repeats.
  for (uint8_t i = 0; i < _subNum; i++) {
    if (_subscriptions[i] == task) {
      return;
    }
  }

  if (_subNum < 10) {
    _subscriptions[_subNum] = task;
    _subNum++;
  } else {
    Serial.println("Out of space");
  }

}
