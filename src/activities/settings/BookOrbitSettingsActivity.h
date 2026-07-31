#pragma once

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

/**
 * Submenu for BookOrbit Sync settings.
 * Shows username, password, server URL, and an authenticate option.
 *
 * Unlike KOReaderSettingsActivity there is no document-matching toggle: BookOrbit
 * always identifies documents by the binary partial-MD5 hash.
 */
class BookOrbitSettingsActivity final : public Activity {
 public:
  explicit BookOrbitSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("BookOrbitSettings", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  ButtonNavigator buttonNavigator;

  size_t selectedIndex = 0;

  void handleSelection();
};
