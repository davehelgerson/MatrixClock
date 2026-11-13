#ifndef IClockface_h
#define IClockface_h

#include "CWDateTime.h"

class IClockface {
  public:
    virtual ~IClockface() = default;
    virtual void setup(CWDateTime *dateTime) = 0;
    virtual void update() = 0;
    virtual void teardown() {}
};

#endif
