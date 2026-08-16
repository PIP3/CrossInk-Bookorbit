#pragma once

#include <I18n.h>

#include <cstdint>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

// A compact 12-hour time editor for the frontlight schedule. It stores the
// same quarter-hour slots as the schedule engine, while presenting tap-target
// fields for hour, minutes, and AM/PM.
class FrontlightTimePickerActivity final : public Activity {
 public:
  FrontlightTimePickerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, StrId titleId,
                               uint8_t initialSlot);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class Field : uint8_t { Hour, Minute, Period, Count };

  StrId titleId;
  uint8_t initialSlot;
  uint8_t hour12 = 12;
  uint8_t minuteQuarter = 0;
  bool isPm = true;
  Field activeField = Field::Hour;
  ButtonNavigator buttonNavigator;

  void adjustActiveField(int delta);
  void selectNextField(int delta);
  void complete();
  bool selectFieldAt(int x, int y, bool togglePeriod);
};
