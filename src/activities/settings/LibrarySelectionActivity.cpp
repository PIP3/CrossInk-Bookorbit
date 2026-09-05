#include "LibrarySelectionActivity.h"

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
#include "stores/LibraryStore.h"

namespace fui = freeink::ui;

namespace {
constexpr fui::ActionId ACTION_ROW = 1;
}  // namespace

int LibrarySelectionActivity::getItemCount() const {
  int count = static_cast<int>(LIBRARY_STORE.getCount());
  // In settings mode, append virtual "Add Library" item.
  if (!pickerMode) {
    count += 1;
  }
  return count;
}

LibrarySelectionActivity::LibrarySelectionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                const bool pickerMode)
    : Activity("LibrarySelection", renderer, mappedInput),
      pickerMode(pickerMode),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void LibrarySelectionActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<LibrarySelectionActivity*>(user);
  if (event.value < 0 || event.value >= static_cast<int16_t>(self->getItemCount())) return;
  self->selectedIndex = event.value;
  // Activation opens an editor/browser or repaints a new value; a lingering
  // flash would gray an unrelated row.
  self->app.clearTapFlash();
  self->handleSelection();
  self->requestUpdate();
}

void LibrarySelectionActivity::onEnter() {
  Activity::onEnter();

  // Reload from disk in case libraries were added/removed by a subactivity or the web UI
  LIBRARY_STORE.loadFromFile();
  selectedIndex = 0;
  uiReady = false;
  visibleRows = 1;
  topIndex = 0;
  applySharedUiTheme(app, uiTarget);
  app.on(ACTION_ROW, &LibrarySelectionActivity::onRowEvent, this);
  app.setScreen(&LibrarySelectionActivity::listScreen, this);
  requestUpdate();
}

void LibrarySelectionActivity::onExit() { Activity::onExit(); }

void LibrarySelectionActivity::loop() {
  if (optionPopup.handleInput(mappedInput, [this] { requestUpdate(); })) return;

  auto activateSelected = [this] { handleSelection(); };

  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    if (pickerMode) {
      activityManager.goHome();
    } else {
      finish();
    }
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activateSelected();
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

  const int itemCount = getItemCount();
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

void LibrarySelectionActivity::handleSelection() {
  const auto libraryCount = static_cast<int>(LIBRARY_STORE.getCount());

  if (pickerMode) {
    // Picker mode: selecting a library navigates to the library browser
    if (selectedIndex < libraryCount) {
      const Library* library = LIBRARY_STORE.getLibrary(selectedIndex);
      if (library) {
        // TODO: Implement library browsing activity
        // activityManager.goToLibraryBrowser(library->id);
        LOG_INF("LIB_SEL", "Selected library: %s", library->id.c_str());
      }
    }
    return;
  }

  // Item layout: configured libraries, Add Library.
  if (selectedIndex == libraryCount) {
    // TODO: Implement Add Library flow
    LOG_INF("LIB_SEL", "Add Library selected");
    return;
  }

  // Library selected: show options (Edit, Delete, Set as Default)
  const Library* library = LIBRARY_STORE.getLibrary(selectedIndex);
  if (!library) return;

  optionPopup.show({
    {tr(STR_EDIT), [this, library] {
      // TODO: Implement Edit Library flow
      LOG_INF("LIB_SEL", "Edit library: %s", library->id.c_str());
      requestUpdate();
    }},
    {tr(STR_DELETE), [this, library] {
      if (LIBRARY_STORE.removeLibrary(selectedIndex)) {
        // Refresh the list
        selectedIndex = std::min(selectedIndex, static_cast<int>(LIBRARY_STORE.getCount()) - 1);
        requestUpdate();
      }
    }},
    {tr(STR_SET_AS_DEFAULT), [this, library] {
      if (LIBRARY_STORE.setDefaultLibrary(library->id)) {
        requestUpdate();
      }
    }},
  });
}

void LibrarySelectionActivity::listScreen(UiApp::ScreenType& screen, void* user) {
  auto* self = static_cast<LibrarySelectionActivity*>(user);
  self->buildListScreen(screen);
}

void LibrarySelectionActivity::buildListScreen(UiApp::ScreenType& screen) {
  const int itemCount = getItemCount();
  const int libraryCount = static_cast<int>(LIBRARY_STORE.getCount());

  screen.clear();
  screen.setHeaderHeight(UITheme::getHeaderHeight());

  // Header
  screen.addHeaderWidget([this](fui::WidgetContext& ctx) {
    GUI.drawHeader(ctx.rect, tr(pickerMode ? STR_SELECT_LIBRARY : STR_LIBRARIES));
    TouchHeaderBackButton::draw(ctx);
  });

  // List of libraries
  for (int i = 0; i < libraryCount; ++i) {
    const Library* library = LIBRARY_STORE.getLibrary(i);
    if (!library) continue;

    screen.addRow([i, library](fui::WidgetContext& ctx) {
      const bool isSelected = (i == selectedIndex);
      const auto& theme = UITheme::get();
      
      // Draw row background
      ctx.gfx->fillRect(ctx.rect, isSelected ? theme->selectedBgColor : theme->bgColor);
      
      // Draw library name and type
      const char* providerType = library->getProviderTypeString().c_str();
      const std::string displayText = library->name + " (" + providerType + ")";
      
      ctx.gfx->setFont(theme->listItemFont);
      ctx.gfx->setTextColor(isSelected ? theme->selectedFgColor : theme->fgColor);
      ctx.gfx->setTextCursor(ctx.rect.x + theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
      ctx.gfx->print(displayText.c_str());
      
      // Draw default library indicator
      if (library->isDefault) {
        const char* defaultText = tr(STR_DEFAULT);
        const int16_t defaultWidth = ctx.gfx->getTextWidth(defaultText);
        ctx.gfx->setTextCursor(ctx.rect.x + ctx.rect.w - defaultWidth - theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
        ctx.gfx->print(defaultText);
      }
    }, ACTION_ROW, i);
  }

  // Add Library button (in settings mode)
  if (!pickerMode) {
    screen.addRow([this](fui::WidgetContext& ctx) {
      const bool isSelected = (libraryCount == selectedIndex);
      const auto& theme = UITheme::get();
      
      ctx.gfx->fillRect(ctx.rect, isSelected ? theme->selectedBgColor : theme->bgColor);
      ctx.gfx->setFont(theme->listItemFont);
      ctx.gfx->setTextColor(isSelected ? theme->selectedFgColor : theme->fgColor);
      ctx.gfx->setTextCursor(ctx.rect.x + theme->listItemHPadding, ctx.rect.y + theme->listItemVPadding);
      ctx.gfx->print(tr(STR_ADD_LIBRARY));
    }, ACTION_ROW, libraryCount);
  }

  // Update visible rows
  visibleRows = screen.getRowCount();
  uiReady = true;
}

void LibrarySelectionActivity::render(RenderLock&&) {
  buildListScreen(app.getScreen());
  app.render();
}
