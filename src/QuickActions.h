#pragma once

#include <HalTiltSensor.h>
#include <I18n.h>

#include <array>
#include <functional>

#include "CrossPointSettings.h"

class OptionPopup;

// One source of truth for the shortcut that opens Quick Actions.  UI and web
// settings call this after changing an action, so the persisted state cannot
// end up with two physical gestures claiming the same menu.
namespace QuickActions {
enum class Trigger : uint8_t { None = 0, ShortPower, LongPower, LongBack, LongMenu };

inline constexpr std::array<StrId, CrossPointSettings::QUICK_ACTION_SLOT_ACTION_COUNT> actionLabels = {
    StrId::STR_IGNORE,
    StrId::STR_SLEEP,
    StrId::STR_PAGE_TURN,
    StrId::STR_FORCE_REFRESH,
    StrId::STR_CHANGE_FONT,
    StrId::STR_TOGGLE_GUIDE_DOTS,
    StrId::STR_TOGGLE_BIONIC_READING,
    StrId::STR_TOGGLE_BOOKMARK,
    StrId::STR_SYNC_PROGRESS,
    StrId::STR_MARK_FINISHED,
    StrId::STR_READING_STATS,
    StrId::STR_SCREENSHOT_BUTTON,
    StrId::STR_CYCLE_PAGE_TURN,
    StrId::STR_FILE_TRANSFER,
    StrId::STR_TILT_PAGE_TURN,
    StrId::STR_READER_DARK_MODE,
    StrId::STR_FOOTNOTES,
    StrId::STR_BROWSE_FILES,
    StrId::STR_CALIBRE_WIRELESS,
    StrId::STR_JOIN_NETWORK,
    StrId::STR_CREATE_HOTSPOT,
    StrId::STR_SAVE_CLIPPING,
    StrId::STR_LOOKUP};

inline bool supportsTiltPageTurn() { return halTiltSensor.isAvailable(); }

inline bool isActionAvailable(const uint8_t action) {
  return action != CrossPointSettings::TOGGLE_TILT_PAGE_TURN || supportsTiltPageTurn();
}

inline void synchronize(CrossPointSettings& settings, Trigger preferred = Trigger::None) {
  const bool shortPower = settings.shortPwrBtn == CrossPointSettings::QUICK_ACTIONS;
  const bool longPower = settings.longPwrBtn == CrossPointSettings::QUICK_ACTIONS;
  const bool longBack = settings.longPressBackAction == CrossPointSettings::LONG_MENU_QUICK_ACTIONS;
  const bool longMenu = settings.longPressMenuAction == CrossPointSettings::LONG_MENU_QUICK_ACTIONS;

  Trigger owner = preferred;
  if (owner == Trigger::None) {
    if (shortPower)
      owner = Trigger::ShortPower;
    else if (longPower)
      owner = Trigger::LongPower;
    else if (longBack)
      owner = Trigger::LongBack;
    else if (longMenu)
      owner = Trigger::LongMenu;
  }

  if (owner != Trigger::ShortPower && shortPower) settings.shortPwrBtn = CrossPointSettings::IGNORE;
  if (owner != Trigger::LongPower && longPower) settings.longPwrBtn = CrossPointSettings::IGNORE;
  if (owner != Trigger::LongBack && longBack) settings.longPressBackAction = CrossPointSettings::LONG_MENU_OFF;
  if (owner != Trigger::LongMenu && longMenu) settings.longPressMenuAction = CrossPointSettings::LONG_MENU_OFF;
  settings.quickActionsTrigger = static_cast<uint8_t>(owner);
}

inline Trigger triggerForSetting(uint8_t CrossPointSettings::* member) {
  if (member == &CrossPointSettings::shortPwrBtn) return Trigger::ShortPower;
  if (member == &CrossPointSettings::longPwrBtn) return Trigger::LongPower;
  if (member == &CrossPointSettings::longPressBackAction) return Trigger::LongBack;
  if (member == &CrossPointSettings::longPressMenuAction) return Trigger::LongMenu;
  return Trigger::None;
}

inline void settingChanged(CrossPointSettings& settings, uint8_t CrossPointSettings::* member) {
  const Trigger trigger = triggerForSetting(member);
  if (trigger == Trigger::None) return;
  const bool selected = (trigger == Trigger::ShortPower || trigger == Trigger::LongPower)
                            ? settings.*member == CrossPointSettings::QUICK_ACTIONS
                            : settings.*member == CrossPointSettings::LONG_MENU_QUICK_ACTIONS;
  synchronize(settings, selected ? trigger : Trigger::None);
}

void showConfiguredPopup(OptionPopup& popup, const std::function<void()>& requestUpdate);
}  // namespace QuickActions
