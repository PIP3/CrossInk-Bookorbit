#pragma once

#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>
#include <I18n.h>

#include <string>
#include <vector>

#include "../Activity.h"
#include "util/ButtonNavigator.h"

enum class FileBrowserAction : int {
  Delete = 0,
  PinFavorite = 1,
  UnpinFavorite = 2,
  SetSleepFolder = 3,
  ClearSleepFolder = 4,
  DeleteCache = 5,
  ToggleCompleted = 6,
  RemoveFromRecents = 7,
  DeleteStats = 8,
  ViewBookmarks = 9,
  ViewClippings = 10,
  DeleteBookmarks = 11,
  DeleteClippings = 12,
  EpubRenderMode = 13,
  ResetReaderSettings = 14,
  SendNearby = 15,
};

class FileBrowserActionActivity final : public Activity {
 public:
  struct MenuItem {
    FileBrowserAction action;
    StrId labelId;
  };

  FileBrowserActionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string title,
                            std::vector<MenuItem> items, bool ignoreInitialConfirmRelease = false);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  using UiApp = freeink::ui::FreeInkApp<20, 4>;
  static constexpr freeink::ui::ActionId ACTION_ROW = 1;

  static void actionMenuScreen(UiApp::ScreenType& screen, void* user);
  static void onRowEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildActionMenuScreen(UiApp::ScreenType& screen);

  ButtonNavigator buttonNavigator;
  std::string title;
  std::vector<MenuItem> items;
  std::vector<freeink::ui::ListItem> uiItems;
  int selectedIndex = 0;
  bool ignoreConfirmRelease = false;
  bool ignoreTouchRelease = false;
  freeink::ui::GfxRendererTarget uiTarget;
  UiApp app;
  bool uiReady = false;
  int contentTop = 0;
  int contentBottom = 0;
};
