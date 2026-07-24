#include "FileBrowserActionActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>

#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace fui = freeink::ui;

namespace {
constexpr int kTitleFontId = UI_10_FONT_ID;
constexpr int kTitleMaxLines = 2;
constexpr int kCompactTitleY = 14;
constexpr int kTallHeaderTitleBottomPadding = 8;
constexpr int kCompactHeaderTitleBottomPadding = 4;
constexpr int kTitleLineGap = 1;
constexpr int kBatteryTextReserveWidth = 90;
}  // namespace

FileBrowserActionActivity::FileBrowserActionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                     std::string title, std::vector<MenuItem> items,
                                                     const bool ignoreInitialConfirmRelease)
    : Activity("FileBrowserAction", renderer, mappedInput),
      title(std::move(title)),
      items(std::move(items)),
      ignoreConfirmRelease(ignoreInitialConfirmRelease),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void FileBrowserActionActivity::onEnter() {
  Activity::onEnter();
  selectedIndex = 0;
  uiReady = false;
  // A touch long-press opens this activity while the finger is still down.
  // Wait for that contact to end so its release cannot activate a menu row.
  int touchX = 0;
  int touchY = 0;
  ignoreTouchRelease = mappedInput.isScreenTouchHeld(touchX, touchY);
  app.setTheme(uiThemeTokens(uiTarget));
  app.on(ACTION_ROW, &FileBrowserActionActivity::onRowEvent, this);
  app.setScreen(&FileBrowserActionActivity::actionMenuScreen, this);
  uiItems.clear();
  uiItems.reserve(items.size());
  for (size_t i = 0; i < items.size(); ++i) {
    fui::ListItem item;
    item.label = I18N.get(items[i].labelId);
    item.actionValue = static_cast<int16_t>(i);
    uiItems.push_back(item);
  }
  requestUpdate();
}

void FileBrowserActionActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<FileBrowserActionActivity*>(user);
  if (event.value < 0 || event.value >= static_cast<int16_t>(self->items.size())) return;
  self->selectedIndex = event.value;
  self->app.clearTapFlash();
  self->setResult(FileBrowserActionResult{static_cast<int>(self->items[self->selectedIndex].action)});
  self->finish();
}

void FileBrowserActionActivity::actionMenuScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<FileBrowserActionActivity*>(user)->buildActionMenuScreen(screen);
}

void FileBrowserActionActivity::buildActionMenuScreen(UiApp::ScreenType& screen) {
  screen.setContentMargin(fui::Insets{static_cast<int16_t>(contentTop), 0,
                                      static_cast<int16_t>(renderer.getScreenHeight() - contentBottom), 0});

  fui::ListProps props;
  props.items = uiItems.data();
  props.count = static_cast<uint16_t>(uiItems.size());
  props.selectedIndex = static_cast<int16_t>(selectedIndex);
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.labelText = screen.theme().bodyText;
  // Touch menus use FreeInkUI's larger two-line rows; button-only devices
  // keep the historical compact one-line presentation in configureUiList().
  props.labelText.maxLines = 2;
  configureUiList(props, screen.theme(), screen.body());
  screen.list(props);
}

void FileBrowserActionActivity::loop() {
  if (ignoreTouchRelease) {
    if (mappedInput.wasScreenTouchReleased()) {
      ignoreTouchRelease = false;
    }
    return;
  }

  if (ignoreConfirmRelease) {
    const bool confirmReleased = mappedInput.wasReleased(MappedInputManager::Button::Confirm);
    if (confirmReleased || !mappedInput.isPressed(MappedInputManager::Button::Confirm)) {
      ignoreConfirmRelease = false;
      return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  const int itemCount = static_cast<int>(items.size());
  if (uiReady) {
    const fui::InputSnapshot snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchReleased) {
      const auto event = app.route(snap);
      if (app.invalidated()) requestUpdate();
      if (event) return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    setResult(FileBrowserActionResult{static_cast<int>(items[selectedIndex].action)});
    finish();
    return;
  }

  buttonNavigator.onNext([this] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, static_cast<int>(items.size()));
    requestUpdate();
  });

  buttonNavigator.onPrevious([this] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, static_cast<int>(items.size()));
    requestUpdate();
  });
}

void FileBrowserActionActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const int titleX = metrics.contentSidePadding;
  const int titleMaxWidth = std::max(0, pageWidth - titleX - metrics.contentSidePadding - kBatteryTextReserveWidth);
  const auto titleLines =
      renderer.wrappedText(kTitleFontId, title.c_str(), titleMaxWidth, kTitleMaxLines, EpdFontFamily::BOLD);
  const int titleLineHeight = renderer.getLineHeight(kTitleFontId);
  const int titleBlockHeight = static_cast<int>(titleLines.size()) * titleLineHeight +
                               std::max(0, static_cast<int>(titleLines.size()) - 1) * kTitleLineGap;
  const bool tallHeader = metrics.headerHeight > 60;
  const int titleY = metrics.topPadding + (tallHeader ? metrics.batteryBarHeight + 3 : kCompactTitleY);
  const int titleBottomPadding = tallHeader ? kTallHeaderTitleBottomPadding : kCompactHeaderTitleBottomPadding;
  const int actionHeaderHeight =
      std::max(metrics.headerHeight, titleY - metrics.topPadding + titleBlockHeight + titleBottomPadding);
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, actionHeaderHeight}, "");

  for (int i = 0; i < static_cast<int>(titleLines.size()); ++i) {
    renderer.drawText(kTitleFontId, titleX, titleY + i * (titleLineHeight + kTitleLineGap), titleLines[i].c_str(), true,
                      EpdFontFamily::BOLD);
  }

  const int contentTop = metrics.topPadding + actionHeaderHeight + metrics.verticalSpacing;
  contentBottom = pageHeight - metrics.buttonHintsHeight - metrics.verticalSpacing * 2;
  this->contentTop = contentTop;

  uiReady = false;
  app.render();
  uiReady = true;

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
