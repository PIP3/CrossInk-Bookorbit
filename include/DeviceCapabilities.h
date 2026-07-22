#pragma once

#include <HalGPIO.h>

inline bool deviceHasEdgeSideButtons(const HalGPIO& gpio) {
#ifdef SIMULATOR
  return gpio.deviceIsX3();
#else
  return gpio.hasEdgeSideButtons();
#endif
}
