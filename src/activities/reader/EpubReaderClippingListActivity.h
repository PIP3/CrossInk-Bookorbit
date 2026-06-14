#pragma once

#include <vector>

#include "ClippingStore.h"
#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class EpubReaderClippingListActivity final : public Activity {
 public:
  EpubReaderClippingListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                 std::vector<Clipping> clippings)
      : Activity("EpubClippingList", renderer, mappedInput), clippings(std::move(clippings)) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  std::vector<Clipping> clippings;
  ButtonNavigator buttonNavigator;
  int selectedIndex = 0;
  bool longPressConfirmHandled = false;

  int getPageItems() const;
  void deleteSelectedClipping();
  void showClippingActionMenu(bool ignoreInitialConfirmRelease);
};
