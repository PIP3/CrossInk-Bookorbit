#pragma once
#include <string>
#include <vector>

#include "../Activity.h"
#include "util/ButtonNavigator.h"
#include "util/DictionaryLookupController.h"
#include "util/LookupHistory.h"

class LookedUpWordsActivity final : public Activity {
 public:
  explicit LookedUpWordsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookCachePath)
      : Activity("LookedUpWords", renderer, mappedInput),
        cachePath(std::move(bookCachePath)),
        controller(renderer, mappedInput, *this, cachePath) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  std::string cachePath;
  std::vector<LookupHistory::Entry> entries;
  int selectedIndex = 0;

  DictionaryLookupController controller;
  ButtonNavigator buttonNavigator;

  bool skipLoopDelay() override { return controller.skipLoopDelay(); }

  void showDeleteConfirmation(bool ignoreInitialConfirmRelease);
  static const char* glyphFor(LookupHistory::Status s);
};
