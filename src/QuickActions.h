#pragma once

#include <HalTiltSensor.h>

#include "CrossPointSettings.h"

// One source of truth for the shortcut that opens Quick Actions.  UI and web
// settings call this after changing an action, so the persisted state cannot
// end up with two physical gestures claiming the same menu.
namespace QuickActions {
enum class Trigger : uint8_t { None = 0, ShortPower, LongPower, LongBack, LongMenu };

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
    if (shortPower) owner = Trigger::ShortPower;
    else if (longPower) owner = Trigger::LongPower;
    else if (longBack) owner = Trigger::LongBack;
    else if (longMenu) owner = Trigger::LongMenu;
  }

  if (owner != Trigger::ShortPower && shortPower) settings.shortPwrBtn = CrossPointSettings::IGNORE;
  if (owner != Trigger::LongPower && longPower) settings.longPwrBtn = CrossPointSettings::IGNORE;
  if (owner != Trigger::LongBack && longBack) settings.longPressBackAction = CrossPointSettings::LONG_MENU_OFF;
  if (owner != Trigger::LongMenu && longMenu) settings.longPressMenuAction = CrossPointSettings::LONG_MENU_OFF;
  settings.quickActionsTrigger = static_cast<uint8_t>(owner);
}

inline Trigger triggerForSetting(uint8_t CrossPointSettings::*member) {
  if (member == &CrossPointSettings::shortPwrBtn) return Trigger::ShortPower;
  if (member == &CrossPointSettings::longPwrBtn) return Trigger::LongPower;
  if (member == &CrossPointSettings::longPressBackAction) return Trigger::LongBack;
  if (member == &CrossPointSettings::longPressMenuAction) return Trigger::LongMenu;
  return Trigger::None;
}

inline void settingChanged(CrossPointSettings& settings, uint8_t CrossPointSettings::*member) {
  const Trigger trigger = triggerForSetting(member);
  if (trigger == Trigger::None) return;
  const bool selected = (trigger == Trigger::ShortPower || trigger == Trigger::LongPower)
                            ? settings.*member == CrossPointSettings::QUICK_ACTIONS
                            : settings.*member == CrossPointSettings::LONG_MENU_QUICK_ACTIONS;
  synchronize(settings, selected ? trigger : Trigger::None);
}

inline CrossPointSettings::LONG_PRESS_MENU_ACTION toReaderAction(uint8_t powerAction) {
  switch (powerAction) {
    case CrossPointSettings::SLEEP: return CrossPointSettings::LONG_MENU_SLEEP;
    case CrossPointSettings::FORCE_REFRESH: return CrossPointSettings::LONG_MENU_REFRESH_SCREEN;
    case CrossPointSettings::TOGGLE_FONT: return CrossPointSettings::LONG_MENU_CHANGE_FONT;
    case CrossPointSettings::TOGGLE_GUIDE_DOTS: return CrossPointSettings::LONG_MENU_TOGGLE_GUIDE_DOTS;
    case CrossPointSettings::TOGGLE_BIONIC_READING: return CrossPointSettings::LONG_MENU_TOGGLE_BIONIC;
    case CrossPointSettings::TOGGLE_BOOKMARK: return CrossPointSettings::LONG_MENU_TOGGLE_BOOKMARK;
    case CrossPointSettings::SYNC_PROGRESS: return CrossPointSettings::LONG_MENU_SYNC_PROGRESS;
    case CrossPointSettings::MARK_FINISHED: return CrossPointSettings::LONG_MENU_MARK_FINISHED;
    case CrossPointSettings::READING_STATS: return CrossPointSettings::LONG_MENU_READING_STATS;
    case CrossPointSettings::SCREENSHOT: return CrossPointSettings::LONG_MENU_SCREENSHOT;
    case CrossPointSettings::CYCLE_PAGE_TURN: return CrossPointSettings::LONG_MENU_CYCLE_PAGE_TURN;
    case CrossPointSettings::FILE_TRANSFER: return CrossPointSettings::LONG_MENU_FILE_TRANSFER;
    case CrossPointSettings::TOGGLE_TILT_PAGE_TURN:
      return isActionAvailable(powerAction) ? CrossPointSettings::LONG_MENU_TOGGLE_TILT_PAGE_TURN
                                            : CrossPointSettings::LONG_MENU_OFF;
    case CrossPointSettings::TOGGLE_DARK_MODE: return CrossPointSettings::LONG_MENU_TOGGLE_DARK_MODE;
    case CrossPointSettings::FOOTNOTES: return CrossPointSettings::LONG_MENU_FOOTNOTES;
    case CrossPointSettings::FILE_BROWSER: return CrossPointSettings::LONG_MENU_FILE_BROWSER;
    case CrossPointSettings::CALIBRE_WIRELESS: return CrossPointSettings::LONG_MENU_CALIBRE_WIRELESS;
    case CrossPointSettings::JOIN_NETWORK: return CrossPointSettings::LONG_MENU_JOIN_NETWORK;
    case CrossPointSettings::CREATE_HOTSPOT: return CrossPointSettings::LONG_MENU_CREATE_HOTSPOT;
    case CrossPointSettings::CREATE_CLIPPING: return CrossPointSettings::LONG_MENU_CREATE_CLIPPING;
    case CrossPointSettings::LOOKUP_WORD: return CrossPointSettings::LONG_MENU_LOOKUP_WORD;
    default: return CrossPointSettings::LONG_MENU_OFF;
  }
}
}  // namespace QuickActions
