#pragma once

#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "activities/Activity.h"
#include "models/CatalogModels.h"
#include "network/CatalogProvider.h"
#include "util/ButtonNavigator.h"

/**
 * Activity for browsing and managing smart scopes within a library.
 * Allows adding, editing, and deleting smart scopes.
 */
class SmartScopeBrowserActivity final : public Activity {
 public:
  explicit SmartScopeBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const std::string& libraryId);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  // FreeInkApp hosts the smart scope list (themed rows, touch routing)
  using UiApp = freeink::ui::FreeInkApp<20, 4>;

  ButtonNavigator buttonNavigator;
  std::string libraryId;
  std::vector<SmartScope> smartScopes;
  int selectedIndex = 0;

  freeink::ui::GfxRendererTarget uiTarget;  // must precede `app`: the app holds a reference to it
  UiApp app;
  std::atomic<bool> uiReady{false};
  int visibleRows = 1;
  int topIndex = 0;

  // Current state
  enum class BrowserState { LOADING, BROWSING, ERROR };
  BrowserState state = BrowserState::LOADING;
  std::string errorMessage;

  static void listScreen(UiApp::ScreenType& screen, void* user);
  static void onRowEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildListScreen(UiApp::ScreenType& screen);

  void loadSmartScopes();
  void handleSelection();
};
