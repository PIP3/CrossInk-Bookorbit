#include "SmartScopeBrowserActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "components/OptionPopup.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"
#include "managers/LibraryManager.h"

namespace fui = freeink::ui;

namespace {
constexpr fui::ActionId ACTION_ROW = 1;
}  // namespace

SmartScopeBrowserActivity::SmartScopeBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
    const std::string& libraryId)
    : Activity("SmartScopeBrowser", renderer, mappedInput),
      libraryId(libraryId),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void SmartScopeBrowserActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<SmartScopeBrowserActivity*>(user);
  if (event.value < 0 || event.value >= static_cast<int16_t>(self->smartScopes.size())) return;
  self->selectedIndex = event.value;
  self->app.clearTapFlash();
  self->handleSelection();
  self->requestUpdate();
}

void SmartScopeBrowserActivity::onEnter() {
  Activity::onEnter();

  state = BrowserState::LOADING;
  selectedIndex = 0;
  uiReady = false;
  visibleRows = 1;
  topIndex = 0;
  
  applySharedUiTheme(app, uiTarget);
  app.on(ACTION_ROW, &SmartScopeBrowserActivity::onRowEvent, this);
  app.setScreen(&SmartScopeBrowserActivity::listScreen, this);
  
  // Load smart scopes for the current library
  loadSmartScopes();
  
  requestUpdate();
}

void SmartScopeBrowserActivity::onExit() { Activity::onExit(); }

void SmartScopeBrowserActivity::loadSmartScopes() {
  state = BrowserState::LOADING;
  
  // Get smart scopes from the LibraryManager
  auto manager = getLibraryManager();
  smartScopes = manager.getSmartScopes(libraryId);
  
  // If no smart scopes, try to refresh the library
  if (smartScopes.empty()) {
    if (!manager.refreshLibrary(libraryId)) {
      state = BrowserState::ERROR;
      errorMessage = "Failed to load smart scopes";
      return;
    }
    smartScopes = manager.getSmartScopes(libraryId);
  }
  
  state = BrowserState::BROWSING;
}

void SmartScopeBrowserActivity::handleSelection() {
  if (state != BrowserState::BROWSING) return;
  
  if (selectedIndex < 0 || selectedIndex >= static_cast<int>(smartScopes.size())) return;
  
  const SmartScope& smartScope = smartScopes[selectedIndex];
  
  // Open the book list for this smart scope
  // TODO: Implement SmartScopeBookListActivity
  LOG_INF("SS_BROWSE", "Opening book list for smart scope: %s", smartScope.id.c_str());
}

void SmartScopeBrowserActivity::listScreen(UiApp::ScreenType& screen, void* user) {
  auto* self = static_cast<SmartScopeBrowserActivity*>(user);
  self->buildListScreen(screen);
}

void SmartScopeBrowserActivity::buildListScreen(UiApp::ScreenType& screen) {
  screen.clear();
  screen.setHeaderHeight(UITheme::getHeaderHeight());

  // Header
  const Library* library = LIBRARY_STORE.getLibraryById(libraryId);
  std::string title = library ? (std::string(library->name) + " - Smart Scopes") : "Smart Scopes";
  
  screen.addHeaderWidget([this, &title](fui::WidgetContext& ctx) {
    GUI.drawHeader(ctx.rect, title.c_str());
    TouchHeaderBackButton::draw(ctx);
  });

  if (state == BrowserState::LOADING) {
    // Show loading message
    screen.addRow([this](fui::WidgetContext& ctx) {
      const auto& theme = UITheme::get();
      ctx.gfx->fillRect(ctx.rect, theme->bgColor);
      ctx.gfx->setFont(theme->listItemFont);
      ctx.gfx->setTextColor(theme->fgColor);
      ctx.gfx->setTextCursor(ctx.rect.x + theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
      ctx.gfx->print(tr(STR_LOADING));
    });
    visibleRows = 1;
    uiReady = true;
    return;
  }

  if (state == BrowserState::ERROR) {
    // Show error message
    screen.addRow([this](fui::WidgetContext& ctx) {
      const auto& theme = UITheme::get();
      ctx.gfx->fillRect(ctx.rect, theme->bgColor);
      ctx.gfx->setFont(theme->listItemFont);
      ctx.gfx->setTextColor(theme->fgColor);
      ctx.gfx->setTextCursor(ctx.rect.x + theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
      ctx.gfx->print(errorMessage.c_str());
    });
    visibleRows = 1;
    uiReady = true;
    return;
  }

  // Show smart scopes
  const auto& theme = UITheme::get();
  
  for (size_t i = 0; i < smartScopes.size(); ++i) {
    const SmartScope& scope = smartScopes[i];
    screen.addRow([i, &scope, &theme](fui::WidgetContext& ctx) {
      const bool isSelected = (static_cast<int>(i) == selectedIndex);
      
      ctx.gfx->fillRect(ctx.rect, isSelected ? theme->selectedBgColor : theme->bgColor);
      ctx.gfx->setFont(theme->listItemFont);
      ctx.gfx->setTextColor(isSelected ? theme->selectedFgColor : theme->fgColor);
      ctx.gfx->setTextCursor(ctx.rect.x + theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
      ctx.gfx->print(scope.title.c_str());
      
      // Show query in subtitle
      ctx.gfx->setFont(theme->listItemSubtitleFont);
      ctx.gfx->setTextCursor(ctx.rect.x + theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding + theme->listItemLineHeight);
      ctx.gfx->print(scope.query.c_str());
      
      // Show book count if available
      if (scope.bookCount > 0) {
        ctx.gfx->setFont(theme->listItemFont);
        std::string countStr = std::to_string(scope.bookCount) + " books";
        const int16_t countWidth = ctx.gfx->getTextWidth(countStr.c_str());
        ctx.gfx->setTextCursor(ctx.rect.x + ctx.rect.w - countWidth - theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
        ctx.gfx->print(countStr.c_str());
      }
    }, ACTION_ROW, static_cast<int16_t>(i));
  }

  // Update visible rows
  visibleRows = screen.getRowCount();
  uiReady = true;
}

void SmartScopeBrowserActivity::loop() {
  if (state != BrowserState::BROWSING) {
    // Wait for loading to complete
    return;
  }

  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    handleSelection();
    return;
  }

  // Touch goes through the FreeInkApp: render() registered the row hit rects;
  // route the snapshot and let onRowEvent dispatch.
  if (uiReady) {
    const fui::InputSnapshot snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchReleased) {
      const auto event = app.route(snap);
      if (app.invalidated()) requestUpdate();
      if (event) return;  // dispatched to onRowEvent
    }
  }

  const int itemCount = static_cast<int>(smartScopes.size());
  if (itemCount > 0) {
    // Swipes scroll the viewport; the selection stays put and button
    // navigation pulls the view back to it.
    const auto swipe = mappedInput.wasSwipe();
    if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
      const int delta = swipe == MappedInputManager::SwipeDir::Up ? visibleRows : -visibleRows;
      const int next = scrollListBy(topIndex, delta, visibleRows, itemCount);
      if (next != topIndex) {
        topIndex = next;
        requestUpdate();
      }
      return;
    }
    const auto moveSelection = [this, itemCount](const int index) {
      selectedIndex = index;
      topIndex = followListSelection(selectedIndex, topIndex, visibleRows, itemCount);
      requestUpdate();
    };
    buttonNavigator.onNext(
        [this, itemCount, &moveSelection] { moveSelection(ButtonNavigator::nextIndex(selectedIndex, itemCount)); });
    buttonNavigator.onPrevious(
        [this, itemCount, &moveSelection] { moveSelection(ButtonNavigator::previousIndex(selectedIndex, itemCount)); });
  }
}

void SmartScopeBrowserActivity::render(RenderLock&&) {
  buildListScreen(app.getScreen());
  app.render();
}
