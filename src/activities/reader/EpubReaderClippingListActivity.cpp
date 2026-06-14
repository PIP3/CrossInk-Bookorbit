#include "EpubReaderClippingListActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "MappedInputManager.h"
#include "activities/ActivityResult.h"
#include "activities/home/FileBrowserActionActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int ROW_HEIGHT = 72;
constexpr int LIST_START_Y = 60;
constexpr unsigned long CLIPPING_DELETE_HOLD_MS = 1000;
}  // namespace

int EpubReaderClippingListActivity::getPageItems() const {
  const auto orientation = renderer.getOrientation();
  const bool isPortraitInverted = orientation == GfxRenderer::Orientation::PortraitInverted;
  const int hintGutterHeight = isPortraitInverted ? 50 : 0;
  const int startY = LIST_START_Y + hintGutterHeight;
  const int available = renderer.getScreenHeight() - startY - ROW_HEIGHT;
  return std::max(1, available / ROW_HEIGHT);
}

void EpubReaderClippingListActivity::onEnter() {
  Activity::onEnter();
  selectedIndex = 0;
  requestUpdate();
}

void EpubReaderClippingListActivity::deleteSelectedClipping() {
  if (clippings.empty() || selectedIndex < 0 || selectedIndex >= static_cast<int>(clippings.size())) return;

  if (!CLIPPINGS.removeClippingAt(static_cast<size_t>(selectedIndex))) return;

  clippings = CLIPPINGS.getClippings();
  if (clippings.empty()) {
    selectedIndex = 0;
  } else if (selectedIndex >= static_cast<int>(clippings.size())) {
    selectedIndex = static_cast<int>(clippings.size()) - 1;
  }
  requestUpdate();
}

void EpubReaderClippingListActivity::showClippingActionMenu(const bool ignoreInitialConfirmRelease) {
  if (clippings.empty() || selectedIndex < 0 || selectedIndex >= static_cast<int>(clippings.size())) return;

  const Clipping selectedClipping = clippings[selectedIndex];
  const char* title = selectedClipping.chapterTitle[0] != '\0' ? selectedClipping.chapterTitle : tr(STR_CLIPPINGS);
  std::vector<FileBrowserActionActivity::MenuItem> items;
  items.reserve(1);
  items.push_back({FileBrowserAction::Delete, StrId::STR_DELETE});

  startActivityForResult(
      std::make_unique<FileBrowserActionActivity>(renderer, mappedInput, title, std::move(items),
                                                  ignoreInitialConfirmRelease),
      [this, selectedClipping](const ActivityResult& result) {
        longPressConfirmHandled = false;
        if (result.isCancelled) {
          requestUpdate();
          return;
        }

        const auto* actionResult = std::get_if<FileBrowserActionResult>(&result.data);
        if (!actionResult || static_cast<FileBrowserAction>(actionResult->action) != FileBrowserAction::Delete) {
          requestUpdate();
          return;
        }

        const auto it = std::find_if(clippings.begin(), clippings.end(), [&selectedClipping](const Clipping& clipping) {
          return clipping.spineIndex == selectedClipping.spineIndex &&
                 clipping.startPage == selectedClipping.startPage &&
                 clipping.startWordIndex == selectedClipping.startWordIndex &&
                 clipping.timestamp == selectedClipping.timestamp;
        });
        if (it != clippings.end()) {
          selectedIndex = static_cast<int>(std::distance(clippings.begin(), it));
          deleteSelectedClipping();
        } else {
          requestUpdate();
        }
      });
}

void EpubReaderClippingListActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  if (!clippings.empty() && !longPressConfirmHandled && mappedInput.isPressed(MappedInputManager::Button::Confirm) &&
      mappedInput.getHeldTime() >= CLIPPING_DELETE_HOLD_MS) {
    longPressConfirmHandled = true;
    showClippingActionMenu(true);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (longPressConfirmHandled) {
      longPressConfirmHandled = false;
      return;
    }
    if (!clippings.empty() && selectedIndex >= 0 && selectedIndex < static_cast<int>(clippings.size())) {
      const Clipping& clipping = clippings[selectedIndex];
      setResult(
          ClippingJumpResult{clipping.spineIndex, clipping.startPage, clipping.pageCount, clipping.paragraphIndex});
      finish();
    }
    return;
  }

  const int total = static_cast<int>(clippings.size());
  if (total == 0) return;

  const int pageItems = getPageItems();
  buttonNavigator.onNextRelease([this, total] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, total);
    requestUpdate();
  });
  buttonNavigator.onPreviousRelease([this, total] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, total);
    requestUpdate();
  });
  buttonNavigator.onNextContinuous([this, total, pageItems] {
    selectedIndex = ButtonNavigator::nextPageIndex(selectedIndex, total, pageItems);
    requestUpdate();
  });
  buttonNavigator.onPreviousContinuous([this, total, pageItems] {
    selectedIndex = ButtonNavigator::previousPageIndex(selectedIndex, total, pageItems);
    requestUpdate();
  });
}

void EpubReaderClippingListActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto orientation = renderer.getOrientation();
  const bool isLandscapeCw = orientation == GfxRenderer::Orientation::LandscapeClockwise;
  const bool isLandscapeCcw = orientation == GfxRenderer::Orientation::LandscapeCounterClockwise;
  const bool isPortraitInverted = orientation == GfxRenderer::Orientation::PortraitInverted;
  const int hintGutterWidth = (isLandscapeCw || isLandscapeCcw) ? 30 : 0;
  const int contentX = isLandscapeCw ? hintGutterWidth : 0;
  const int contentWidth = pageWidth - hintGutterWidth;
  const int hintGutterHeight = isPortraitInverted ? 50 : 0;
  const int contentY = hintGutterHeight;

  const int titleX =
      contentX + (contentWidth - renderer.getTextWidth(UI_12_FONT_ID, tr(STR_CLIPPINGS), EpdFontFamily::BOLD)) / 2;
  renderer.drawText(UI_12_FONT_ID, titleX, 15 + contentY, tr(STR_CLIPPINGS), true, EpdFontFamily::BOLD);

  if (clippings.empty()) {
    renderer.drawCenteredText(UI_10_FONT_ID, LIST_START_Y + contentY + 20, tr(STR_NO_CLIPPINGS));
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4, true);
    renderer.displayBuffer();
    return;
  }

  const int pageItems = getPageItems();
  const int total = static_cast<int>(clippings.size());
  const int pageStartIndex = (selectedIndex / pageItems) * pageItems;
  const int marginLeft = contentX + 20;

  for (int i = 0; i < pageItems; i++) {
    const int itemIndex = pageStartIndex + i;
    if (itemIndex >= total) break;

    const int rowY = LIST_START_Y + contentY + i * ROW_HEIGHT;
    const bool isSelected = itemIndex == selectedIndex;
    if (isSelected) {
      renderer.fillRect(contentX, rowY, contentWidth - 1, ROW_HEIGHT, true);
    }

    const Clipping& clipping = clippings[itemIndex];
    const std::string snippetTrunc = renderer.truncatedText(UI_10_FONT_ID, clipping.text.c_str(), contentWidth - 40);
    renderer.drawText(UI_10_FONT_ID, marginLeft, rowY + 5, snippetTrunc.c_str(), !isSelected);

    const char* chapter = clipping.chapterTitle[0] != '\0' ? clipping.chapterTitle : tr(STR_UNKNOWN_CHAPTER);
    const std::string chapterTrunc = renderer.truncatedText(SMALL_FONT_ID, chapter, contentWidth - 40);
    renderer.drawText(SMALL_FONT_ID, marginLeft, rowY + 31, chapterTrunc.c_str(), !isSelected);

    char pageBuf[24];
    snprintf(pageBuf, sizeof(pageBuf), "%u", static_cast<unsigned>(clipping.startPage + 1));
    renderer.drawText(SMALL_FONT_ID, marginLeft, rowY + 51, pageBuf, !isSelected);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4, true);
  renderer.displayBuffer();
}
