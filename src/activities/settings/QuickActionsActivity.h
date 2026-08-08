#pragma once

#include "activities/Activity.h"
#include "components/OptionPopup.h"

class QuickActionsActivity final : public Activity {
  OptionPopup popup;
  void showOverview();
  void editShortcut();
  void editSlot(uint8_t slot);

 public:
  QuickActionsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("QuickActions", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
