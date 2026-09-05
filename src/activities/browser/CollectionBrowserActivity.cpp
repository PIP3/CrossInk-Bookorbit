#include "CollectionBrowserActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"
#include "managers/LibraryManager.h"

namespace fui = freeink::ui;

namespace {
constexpr fui::ActionId ACTION_ROW = 1;
constexpr int MAX_COLLECTIONS = 100;
}  // namespace

CollectionBrowserActivity::CollectionBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
    const std::string& libraryId)
    : Activity("CollectionBrowser", renderer, mappedInput),
      libraryId(libraryId),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void CollectionBrowserActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<CollectionBrowserActivity*>(user);
  if (event.value < 0 || event.value >= static_cast<int16_t>(self->collections.size() + (self->showSmartScopes ? self->smartScopes.size() : 0))) return;
  self->selectedIndex = event.value;
  self->app.clearTapFlash();
  self->handleSelection();
  self->requestUpdate();
}

void CollectionBrowserActivity::onEnter() {
  Activity::onEnter();

  state = BrowserState::LOADING;
  selectedIndex = 0;
  uiReady = false;
  visibleRows = 1;
  topIndex = 0;
  
  // Clear navigation stack and start fresh
  navigationStack.clear();
  
  applySharedUiTheme(app, uiTarget);
  app.on(ACTION_ROW, &CollectionBrowserActivity::onRowEvent, this);
  app.setScreen(&CollectionBrowserActivity::listScreen, this);
  
  // Load collections for the current library
  loadCollections();
  
  requestUpdate();
}

void CollectionBrowserActivity::onExit() { Activity::onExit(); }

void CollectionBrowserActivity::loadCollections() {
  state = BrowserState::LOADING;
  
  // Get collections from the LibraryManager
  auto manager = getLibraryManager();
  collections = manager.getCollections(libraryId);
  
  // If no collections, try to refresh the library
  if (collections.empty()) {
    if (!manager.refreshLibrary(libraryId)) {
      state = BrowserState::ERROR;
      errorMessage = "Failed to load collections";
      return;
    }
    collections = manager.getCollections(libraryId);
  }
  
  state = BrowserState::BROWSING;
}

void CollectionBrowserActivity::loadSmartScopes() {
  auto manager = getLibraryManager();
  smartScopes = manager.getSmartScopes(libraryId);
}

void CollectionBrowserActivity::navigateToCollection(const Collection& collection) {
  // If the collection is a smart scope, treat it as such
  if (collection.isSmartScope) {
    // Find the corresponding smart scope
    for (const auto& scope : smartScopes) {
      if (scope.id == collection.id) {
        navigateToSmartScope(scope);
        return;
      }
    }
    return;
  }
  
  // Check if the collection has children
  auto manager = getLibraryManager();
  auto children = manager.getCollections(collection.libraryId);
  bool hasChildren = false;
  for (const auto& child : children) {
    if (child.parentId == collection.id) {
      hasChildren = true;
      break;
    }
  }
  
  if (hasChildren) {
    // Navigate into the collection
    navigationStack.push_back(collection.id);
    // Reload collections for the new parent
    collections = manager.getCollections(collection.libraryId);
    // Filter to only show children of the selected collection
    std::vector<Collection> filtered;
    for (const auto& col : collections) {
      if (col.parentId == collection.id) {
        filtered.push_back(col);
      }
    }
    collections = filtered;
    selectedIndex = 0;
    topIndex = 0;
  } else {
    // Open the book list for this collection
    // TODO: Implement BookListActivity for collections
    LOG_INF("COL_BROWSE", "Opening book list for collection: %s", collection.id.c_str());
  }
  
  requestUpdate();
}

void CollectionBrowserActivity::navigateToSmartScope(const SmartScope& smartScope) {
  // TODO: Implement SmartScopeBookListActivity
  LOG_INF("COL_BROWSE", "Opening book list for smart scope: %s", smartScope.id.c_str());
}

void CollectionBrowserActivity::navigateBack() {
  if (navigationStack.empty()) {
    // Return to library selection
    finish();
    return;
  }
  
  // Pop the last collection from the stack
  navigationStack.pop_back();
  
  // Reload collections for the parent
  auto manager = getLibraryManager();
  if (navigationStack.empty()) {
    // Back to root collections
    collections = manager.getCollections(libraryId);
  } else {
    // Filter to show children of the parent
    std::string parentId = navigationStack.back();
    collections = manager.getCollections(libraryId);
    std::vector<Collection> filtered;
    for (const auto& col : collections) {
      if (col.parentId == parentId) {
        filtered.push_back(col);
      }
    }
    collections = filtered;
  }
  
  selectedIndex = 0;
  topIndex = 0;
  requestUpdate();
}

void CollectionBrowserActivity::handleSelection() {
  if (state != BrowserState::BROWSING) return;
  
  const int totalItems = collections.size() + (showSmartScopes ? smartScopes.size() : 0);
  
  if (selectedIndex < 0 || selectedIndex >= totalItems) return;
  
  if (selectedIndex < static_cast<int>(collections.size())) {
    // Collection selected
    navigateToCollection(collections[selectedIndex]);
  } else if (showSmartScopes) {
    // Smart scope selected
    int smartScopeIndex = selectedIndex - collections.size();
    if (smartScopeIndex >= 0 && smartScopeIndex < static_cast<int>(smartScopes.size())) {
      navigateToSmartScope(smartScopes[smartScopeIndex]);
    }
  }
}

void CollectionBrowserActivity::listScreen(UiApp::ScreenType& screen, void* user) {
  auto* self = static_cast<CollectionBrowserActivity*>(user);
  self->buildListScreen(screen);
}

void CollectionBrowserActivity::buildListScreen(UiApp::ScreenType& screen) {
  screen.clear();
  screen.setHeaderHeight(UITheme::getHeaderHeight());

  // Header
  const Library* library = LIBRARY_STORE.getLibraryById(libraryId);
  std::string title = library ? library->name : "Collections";
  
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

  // Show collections
  const auto& theme = UITheme::get();
  
  for (size_t i = 0; i < collections.size(); ++i) {
    const Collection& collection = collections[i];
    screen.addRow([i, &collection, &theme](fui::WidgetContext& ctx) {
      const bool isSelected = (static_cast<int>(i) == selectedIndex);
      
      ctx.gfx->fillRect(ctx.rect, isSelected ? theme->selectedBgColor : theme->bgColor);
      ctx.gfx->setFont(theme->listItemFont);
      ctx.gfx->setTextColor(isSelected ? theme->selectedFgColor : theme->fgColor);
      ctx.gfx->setTextCursor(ctx.rect.x + theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
      ctx.gfx->print(collection.title.c_str());
      
      // Show book count if available
      if (collection.bookCount > 0) {
        std::string countStr = std::to_string(collection.bookCount) + " books";
        const int16_t countWidth = ctx.gfx->getTextWidth(countStr.c_str());
        ctx.gfx->setTextCursor(ctx.rect.x + ctx.rect.w - countWidth - theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
        ctx.gfx->print(countStr.c_str());
      }
    }, ACTION_ROW, static_cast<int16_t>(i));
  }

  // Show smart scopes if enabled
  if (showSmartScopes) {
    for (size_t i = 0; i < smartScopes.size(); ++i) {
      const SmartScope& scope = smartScopes[i];
      int rowIndex = collections.size() + i;
      screen.addRow([rowIndex, &scope, &theme](fui::WidgetContext& ctx) {
        const bool isSelected = (rowIndex == selectedIndex);
        
        ctx.gfx->fillRect(ctx.rect, isSelected ? theme->selectedBgColor : theme->bgColor);
        ctx.gfx->setFont(theme->listItemFont);
        ctx.gfx->setTextColor(isSelected ? theme->selectedFgColor : theme->fgColor);
        ctx.gfx->setTextCursor(ctx.rect.x + theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
        ctx.gfx->print(scope.title.c_str());
        
        // Show book count if available
        if (scope.bookCount > 0) {
          std::string countStr = std::to_string(scope.bookCount) + " books";
          const int16_t countWidth = ctx.gfx->getTextWidth(countStr.c_str());
          ctx.gfx->setTextCursor(ctx.rect.x + ctx.rect.w - countWidth - theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
          ctx.gfx->print(countStr.c_str());
        }
      }, ACTION_ROW, static_cast<int16_t>(rowIndex));
    }
  }

  // Update visible rows
  visibleRows = screen.getRowCount();
  uiReady = true;
}

void CollectionBrowserActivity::loop() {
  if (state != BrowserState::BROWSING) {
    // Wait for loading to complete
    return;
  }

  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    navigateBack();
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

  const int itemCount = collections.size() + (showSmartScopes ? smartScopes.size() : 0);
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

void CollectionBrowserActivity::render(RenderLock&&) {
  buildListScreen(app.getScreen());
  app.render();
}
