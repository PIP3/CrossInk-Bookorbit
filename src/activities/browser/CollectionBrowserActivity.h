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
 * Activity for browsing collections within a library.
 * Supports nested collections and smart scopes.
 */
class CollectionBrowserActivity final : public Activity {
 public:
  explicit CollectionBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const std::string& libraryId);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  // FreeInkApp hosts the collection list (themed rows, touch routing)
  using UiApp = freeink::ui::FreeInkApp<24, 4>;

  ButtonNavigator buttonNavigator;
  std::string libraryId;
  std::vector<Collection> collections;
  std::vector<SmartScope> smartScopes;
  int selectedIndex = 0;
  bool showSmartScopes = false;
  
  // Navigation stack for nested collections
  std::vector<std::string> navigationStack;

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

  void loadCollections();
  void loadSmartScopes();
  void navigateToCollection(const Collection& collection);
  void navigateToSmartScope(const SmartScope& smartScope);
  void navigateBack();
  void handleSelection();
};
