#include "QuickActionsActivity.h"

#include <HalGPIO.h>
#include <I18n.h>

#include <algorithm>

#include "CrossPointSettings.h"
#include "QuickActions.h"

namespace {
constexpr StrId actionLabels[] = {
    StrId::STR_IGNORE, StrId::STR_SLEEP, StrId::STR_PAGE_TURN, StrId::STR_FORCE_REFRESH, StrId::STR_CHANGE_FONT,
    StrId::STR_TOGGLE_GUIDE_DOTS, StrId::STR_TOGGLE_BIONIC_READING, StrId::STR_TOGGLE_BOOKMARK,
    StrId::STR_SYNC_PROGRESS, StrId::STR_MARK_FINISHED, StrId::STR_READING_STATS, StrId::STR_SCREENSHOT_BUTTON,
    StrId::STR_CYCLE_PAGE_TURN, StrId::STR_FILE_TRANSFER, StrId::STR_TILT_PAGE_TURN, StrId::STR_READER_DARK_MODE,
    StrId::STR_FOOTNOTES, StrId::STR_BROWSE_FILES, StrId::STR_CALIBRE_WIRELESS, StrId::STR_JOIN_NETWORK,
    StrId::STR_CREATE_HOTSPOT, StrId::STR_SAVE_CLIPPING, StrId::STR_LOOKUP};
constexpr StrId triggerLabels[] = {StrId::STR_NONE_OPT, StrId::STR_SHORT_PRESS_POWER, StrId::STR_LONG_PRESS_POWER,
                                   StrId::STR_LONG_PRESS_BACK, StrId::STR_LONG_PRESS_MENU_SHORTCUT};

std::vector<QuickActions::Trigger> availableTriggers() {
  std::vector<QuickActions::Trigger> triggers = {QuickActions::Trigger::None, QuickActions::Trigger::ShortPower,
                                                   QuickActions::Trigger::LongPower};
  if (!gpio.hasTouch()) {
    triggers.push_back(QuickActions::Trigger::LongBack);
    triggers.push_back(QuickActions::Trigger::LongMenu);
  }
  return triggers;
}
}  // namespace

void QuickActionsActivity::onEnter() {
  Activity::onEnter();
  QuickActions::synchronize(SETTINGS);
  showOverview();
}

void QuickActionsActivity::showOverview() {
  std::vector<std::string> rows;
  rows.reserve(6);
  auto trigger = static_cast<QuickActions::Trigger>(SETTINGS.quickActionsTrigger);
  const auto triggers = availableTriggers();
  if (std::find(triggers.begin(), triggers.end(), trigger) == triggers.end()) trigger = QuickActions::Trigger::None;
  rows.emplace_back(std::string(I18N.get(StrId::STR_SHORTCUT)) + ": " + I18N.get(triggerLabels[static_cast<uint8_t>(trigger)]));
  for (uint8_t i = 0; i < 5; ++i) {
    const uint8_t action = SETTINGS.quickActionSlots[i];
    const char* label = action < CrossPointSettings::QUICK_ACTION_SLOT_ACTION_COUNT ? I18N.get(actionLabels[action]) : "-";
    rows.emplace_back(std::to_string(i + 1) + ". " + label);
  }
  popup.show(StrId::STR_QUICK_ACTIONS, rows, 0, [this](int selected) {
    if (selected == 0) editShortcut();
    else if (selected > 0 && selected <= 5) editSlot(static_cast<uint8_t>(selected - 1));
  });
  requestUpdate();
}

void QuickActionsActivity::editShortcut() {
  const auto triggers = availableTriggers();
  std::vector<std::string> labels;
  labels.reserve(triggers.size());
  uint8_t current = 0;
  for (uint8_t i = 0; i < triggers.size(); ++i) {
    labels.emplace_back(I18N.get(triggerLabels[static_cast<uint8_t>(triggers[i])]));
    if (static_cast<uint8_t>(triggers[i]) == SETTINGS.quickActionsTrigger) current = i;
  }
  popup.show(StrId::STR_SHORTCUT, labels, current, [this, triggers](int selected) {
    if (selected < 0 || static_cast<size_t>(selected) >= triggers.size()) return;
    const auto trigger = triggers[selected];
    if (trigger == QuickActions::Trigger::ShortPower) SETTINGS.shortPwrBtn = CrossPointSettings::QUICK_ACTIONS;
    if (trigger == QuickActions::Trigger::LongPower) SETTINGS.longPwrBtn = CrossPointSettings::QUICK_ACTIONS;
    if (trigger == QuickActions::Trigger::LongBack) SETTINGS.longPressBackAction = CrossPointSettings::LONG_MENU_QUICK_ACTIONS;
    if (trigger == QuickActions::Trigger::LongMenu) SETTINGS.longPressMenuAction = CrossPointSettings::LONG_MENU_QUICK_ACTIONS;
    QuickActions::synchronize(SETTINGS, trigger);
    SETTINGS.saveToFile();
    showOverview();
  });
}

void QuickActionsActivity::editSlot(uint8_t slot) {
  std::vector<std::string> labels;
  labels.reserve(CrossPointSettings::QUICK_ACTION_SLOT_ACTION_COUNT);
  for (const auto label : actionLabels) labels.emplace_back(I18N.get(label));
  uint8_t current = SETTINGS.quickActionSlots[slot];
  if (current >= CrossPointSettings::QUICK_ACTION_SLOT_ACTION_COUNT) current = CrossPointSettings::IGNORE;
  popup.show(StrId::STR_QUICK_ACTIONS, labels, current, [this, slot](int selected) {
    SETTINGS.quickActionSlots[slot] = static_cast<uint8_t>(selected);
    SETTINGS.saveToFile();
    showOverview();
  });
}

void QuickActionsActivity::loop() {
  if (popup.handleInput(mappedInput, [this] { requestUpdate(); })) return;
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    SETTINGS.saveToFile();
    finish();
  }
}

void QuickActionsActivity::render(RenderLock&&) {
  renderer.clearScreen();
  popup.processRender(renderer, mappedInput);
}
