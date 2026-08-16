#include "FrontlightTimePickerActivity.h"

#include <FreeInkUICore.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <cstdio>
#include <utility>

#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/icons/listIcons.h"
#include "fontIds.h"
#include "util/FrontlightSchedule.h"

namespace {
constexpr int kFieldHeight = 60;
constexpr int kHourWidth = 76;
constexpr int kMinuteWidth = 88;
constexpr int kPeriodWidth = 96;
constexpr int kFieldGap = 14;
constexpr int kColonGap = 8;
constexpr int kTouchButtonSize = 60;
constexpr int kTouchButtonGap = 12;
constexpr int kTouchButtonOffset = 24;

struct PickerLayout {
  Rect hourRect;
  Rect minuteRect;
  Rect periodRect;
  Rect incrementRect;
  Rect decrementRect;
  int colonX;
  int textY;
};

bool contains(const Rect& rect, const int x, const int y) {
  return x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height;
}

PickerLayout getPickerLayout(const GfxRenderer& renderer, const bool showTouchControls) {
  const int colonWidth = renderer.getTextWidth(UI_12_FONT_ID, ":", EpdFontFamily::BOLD);
  const int fieldsWidth =
      kHourWidth + kFieldGap + kColonGap + colonWidth + kColonGap + kMinuteWidth + kFieldGap + kPeriodWidth;
  const int controlsWidth = showTouchControls ? kTouchButtonOffset + kTouchButtonSize : 0;
  const int totalWidth = fieldsWidth + controlsWidth;
  const int startX = (renderer.getScreenWidth() - totalWidth) / 2;
  const int controlsHeight = kTouchButtonSize * 2 + kTouchButtonGap;
  const int controlsY = renderer.getScreenHeight() / 2 - controlsHeight / 2;
  const int fieldY = controlsY + (controlsHeight - kFieldHeight) / 2;

  int x = startX;
  const Rect hourRect{x, fieldY, kHourWidth, kFieldHeight};
  x += kHourWidth + kFieldGap;
  const int colonX = x;
  x += kColonGap + colonWidth + kColonGap;
  const Rect minuteRect{x, fieldY, kMinuteWidth, kFieldHeight};
  x += kMinuteWidth + kFieldGap;
  const Rect periodRect{x, fieldY, kPeriodWidth, kFieldHeight};
  x += kPeriodWidth + kTouchButtonOffset;

  const Rect incrementRect{x, controlsY, kTouchButtonSize, kTouchButtonSize};
  const Rect decrementRect{x, controlsY + kTouchButtonSize + kTouchButtonGap, kTouchButtonSize, kTouchButtonSize};
  return {hourRect,
          minuteRect,
          periodRect,
          incrementRect,
          decrementRect,
          colonX,
          fieldY + (kFieldHeight - renderer.getLineHeight(UI_12_FONT_ID)) / 2};
}

void drawChevronButton(const GfxRenderer& renderer, const Rect& rect, const freeink::Icon& icon) {
  renderer.fillRectDither(rect.x, rect.y, rect.width, rect.height, Color::White);
  renderer.drawRect(rect.x, rect.y, rect.width, rect.height, true);
  const freeink::ui::BitmapRef bitmap{icon.bits, icon.w, icon.h, freeink::ui::BitmapFormat::Mask1, true};
  freeink::ui::forEachBitmapPixel(
      freeink::ui::Rect{static_cast<int16_t>(rect.x), static_cast<int16_t>(rect.y), static_cast<int16_t>(rect.width),
                        static_cast<int16_t>(rect.height)},
      bitmap, freeink::ui::BitmapMode::Center,
      [&renderer](const int16_t x, const int16_t y) { renderer.drawPixel(x, y, true); });
}
}  // namespace

FrontlightTimePickerActivity::FrontlightTimePickerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                           const StrId titleId, const uint8_t initialSlot)
    : Activity("FrontlightTimePicker", renderer, mappedInput), titleId(titleId), initialSlot(initialSlot) {}

void FrontlightTimePickerActivity::onEnter() {
  Activity::onEnter();
  const FrontlightSchedule::TimeOfDay time = FrontlightSchedule::timeOfDayFromSlot(initialSlot);
  hour12 = time.hour12;
  minuteQuarter = time.minuteQuarter;
  isPm = time.isPm;
  activeField = Field::Hour;
  requestUpdate();
}

void FrontlightTimePickerActivity::adjustActiveField(const int delta) {
  switch (activeField) {
    case Field::Hour:
      hour12 = static_cast<uint8_t>((static_cast<int>(hour12) - 1 + delta + 12) % 12 + 1);
      break;
    case Field::Minute:
      minuteQuarter = static_cast<uint8_t>((static_cast<int>(minuteQuarter) + delta + 4) % 4);
      break;
    case Field::Period:
      isPm = !isPm;
      break;
    case Field::Count:
      break;
  }
}

void FrontlightTimePickerActivity::selectNextField(const int delta) {
  constexpr int fieldCount = static_cast<int>(Field::Count);
  activeField = static_cast<Field>((static_cast<int>(activeField) + delta + fieldCount) % fieldCount);
}

void FrontlightTimePickerActivity::complete() {
  setResult(IntervalResult{FrontlightSchedule::slotFromTimeOfDay(hour12, minuteQuarter, isPm)});
  finish();
}

bool FrontlightTimePickerActivity::selectFieldAt(const int x, const int y, const bool togglePeriod) {
  const PickerLayout layout = getPickerLayout(renderer, mappedInput.hasTouch());
  if (contains(layout.hourRect, x, y)) {
    activeField = Field::Hour;
  } else if (contains(layout.minuteRect, x, y)) {
    activeField = Field::Minute;
  } else if (contains(layout.periodRect, x, y)) {
    activeField = Field::Period;
    if (togglePeriod) isPm = !isPm;
  } else {
    return false;
  }
  requestUpdate();
  return true;
}

void FrontlightTimePickerActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    complete();
    return;
  }

  if (mappedInput.hasTouch()) {
    int tx = 0;
    int ty = 0;
    const PickerLayout layout = getPickerLayout(renderer, true);
    if (mappedInput.wasScreenTouchDown(tx, ty)) {
      if (contains(layout.incrementRect, tx, ty) || contains(layout.decrementRect, tx, ty)) return;
      if (selectFieldAt(tx, ty, false)) return;
    }
    if (mappedInput.wasScreenTapped(tx, ty)) {
      if (contains(layout.incrementRect, tx, ty)) {
        adjustActiveField(+1);
        requestUpdate();
        return;
      }
      if (contains(layout.decrementRect, tx, ty)) {
        adjustActiveField(-1);
        requestUpdate();
        return;
      }
      if (selectFieldAt(tx, ty, true)) return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    selectNextField(-1);
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    selectNextField(+1);
    requestUpdate();
    return;
  }

  buttonNavigator.onNextRelease([this] {
    adjustActiveField(+1);
    requestUpdate();
  });
  buttonNavigator.onPreviousRelease([this] {
    adjustActiveField(-1);
    requestUpdate();
  });
  buttonNavigator.onNextContinuous([this] {
    adjustActiveField(+1);
    requestUpdate();
  });
  buttonNavigator.onPreviousContinuous([this] {
    adjustActiveField(-1);
    requestUpdate();
  });
}

void FrontlightTimePickerActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, header, I18N.get(titleId), false);
  } else {
    GUI.drawHeader(renderer, header, I18N.get(titleId));
  }

  const PickerLayout layout = getPickerLayout(renderer, mappedInput.hasTouch());
  char hourText[4];
  char minuteText[4];
  snprintf(hourText, sizeof(hourText), "%u", static_cast<unsigned>(hour12));
  snprintf(minuteText, sizeof(minuteText), "%02u", static_cast<unsigned>(minuteQuarter * 15));
  const char* periodText = I18N.get(isPm ? StrId::STR_PM : StrId::STR_AM);

  auto drawField = [&](const char* text, const Rect& rect, const Field field) {
    const bool selected = field == activeField;
    renderer.fillRectDither(rect.x, rect.y, rect.width, rect.height, selected ? Color::LightGray : Color::White);
    renderer.drawRect(rect.x, rect.y, rect.width, rect.height, true);
    if (selected) renderer.drawRect(rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2, true);
    const int textWidth = renderer.getTextWidth(UI_12_FONT_ID, text, EpdFontFamily::BOLD);
    renderer.drawText(UI_12_FONT_ID, rect.x + (rect.width - textWidth) / 2, layout.textY, text, true,
                      EpdFontFamily::BOLD);
  };

  drawField(hourText, layout.hourRect, Field::Hour);
  renderer.drawText(UI_12_FONT_ID, layout.colonX, layout.textY, ":", true, EpdFontFamily::BOLD);
  drawField(minuteText, layout.minuteRect, Field::Minute);
  drawField(periodText, layout.periodRect, Field::Period);

  if (mappedInput.hasTouch()) {
    drawChevronButton(renderer, layout.incrementRect, icon_chevron_up_32);
    drawChevronButton(renderer, layout.decrementRect, icon_chevron_down_32);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
