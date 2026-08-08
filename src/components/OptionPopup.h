#pragma once
#include <I18n.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "GfxRenderer.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

class OptionPopup {
 public:
  void show(StrId titleId, const StrId* optionIds, int optionCount, int currentIndex,
            std::function<void(int)> onSelect) {
    title = I18N.get(titleId);
    ownedStrings.resize(optionCount);
    for (int i = 0; i < optionCount; i++) {
      ownedStrings[i] = I18N.get(optionIds[i]);
    }
    onSelectCallback = std::move(onSelect);
    prepareStandardShow();
    activate(currentIndex);
  }

  void show(const char* titleStr, const char* const* options, int optionCount, int currentIndex,
            std::function<void(int)> onSelect) {
    title = titleStr;
    ownedStrings.resize(optionCount);
    for (int i = 0; i < optionCount; i++) {
      ownedStrings[i] = options[i];
    }
    onSelectCallback = std::move(onSelect);
    prepareStandardShow();
    activate(currentIndex);
  }

  void show(StrId titleId, const std::vector<std::string>& options, int currentIndex,
            std::function<void(int)> onSelect) {
    title = I18N.get(titleId);
    ownedStrings = options;
    onSelectCallback = std::move(onSelect);
    prepareStandardShow();
    activate(currentIndex);
  }

  void showConfirmed(StrId titleId, const std::vector<std::string>& options, int currentIndex,
                     std::function<void(int)> onSave, std::function<void()> onCancel) {
    title = I18N.get(titleId);
    ownedStrings = options;
    onSelectCallback = std::move(onSave);
    onCancelCallback = std::move(onCancel);
    confirmationMode = true;
    activate(currentIndex);
  }

  void setCancelCallback(std::function<void()> onCancel) { onCancelCallback = std::move(onCancel); }

  void show(const char* titleStr, const std::vector<std::string>& options, int currentIndex,
            std::function<void(int)> onSelect) {
    title = titleStr;
    ownedStrings = options;
    onSelectCallback = std::move(onSelect);
    prepareStandardShow();
    activate(currentIndex);
  }

  void setDismissOnOutsideTouchDown(bool enabled) { dismissOnOutsideTouchDown = enabled; }

  bool handleInput(MappedInputManager& input, const std::function<void()>& requestUpdate) {
    if (!active) return false;

    const int count = static_cast<int>(ownedStrings.size());
    if (count <= 0) {
      active = false;
      return true;
    }
    int tx = 0;
    int ty = 0;
    if (input.wasScreenTouchDown(tx, ty)) {
      touchDownOptionIndex = -1;
      touchDownTarget = TouchTarget::None;
      const auto& hitLayout = getLayout(input.getRenderer());
      for (int i = 0; i < static_cast<int>(hitLayout.options.size()); i++) {
        if (contains(hitLayout.options[i], tx, ty)) {
          const int optionIndex = hitLayout.firstOptionIndex + i;
          touchDownOptionIndex = optionIndex;
          touchDownTarget = TouchTarget::Option;
          if (selectedIndex != optionIndex) {
            selectedIndex = optionIndex;
            layoutValid = false;
            requestUpdate();
          }
          break;
        }
      }
      if (confirmationMode && contains(hitLayout.cancel, tx, ty)) {
        touchDownTarget = TouchTarget::Cancel;
        return true;
      }
      if (confirmationMode && contains(hitLayout.save, tx, ty)) {
        touchDownTarget = TouchTarget::Save;
        return true;
      }
      if ((dismissOnOutsideTouchDown || confirmationMode) && !contains(hitLayout.dialog, tx, ty)) {
        touchDownTarget = TouchTarget::Outside;
      }
      return true;
    }

    if (input.wasScreenTapped(tx, ty)) {
      const auto& hitLayout = getLayout(input.getRenderer());
      if (touchDownTarget == TouchTarget::Cancel && contains(hitLayout.cancel, tx, ty)) {
        cancel(input, requestUpdate, false);
        return true;
      }
      if (touchDownTarget == TouchTarget::Save && contains(hitLayout.save, tx, ty)) {
        save(input, requestUpdate, false);
        return true;
      }
      if (touchDownTarget == TouchTarget::Outside && !contains(hitLayout.dialog, tx, ty)) {
        cancel(input, requestUpdate, false);
        return true;
      }
      if (touchDownTarget == TouchTarget::Option && touchDownOptionIndex >= 0) {
        selectedIndex = touchDownOptionIndex;
        touchDownOptionIndex = -1;
        if (confirmationMode) {
          requestUpdate();
          return true;
        }
        save(input, requestUpdate, false);
        return true;
      }
      for (int i = 0; i < static_cast<int>(hitLayout.options.size()); i++) {
        if (contains(hitLayout.options[i], tx, ty)) {
          selectedIndex = hitLayout.firstOptionIndex + i;
          if (confirmationMode) {
            requestUpdate();
            return true;
          }
          save(input, requestUpdate, false);
          return true;
        }
      }
      // Taps on the dialog chrome (title, padding) keep the popup open; taps outside dismiss it
      if (contains(hitLayout.dialog, tx, ty)) return true;
      cancel(input, requestUpdate, false);
      return true;
    }

    if (input.wasPressed(MappedInputManager::Button::Up) || input.wasPressed(MappedInputManager::Button::Left)) {
      selectedIndex = (selectedIndex - 1 + count) % count;
      layoutValid = false;
      requestUpdate();
      return true;
    } else if (input.wasPressed(MappedInputManager::Button::Down) ||
               input.wasPressed(MappedInputManager::Button::Right)) {
      selectedIndex = (selectedIndex + 1) % count;
      layoutValid = false;
      requestUpdate();
      return true;
    } else if (input.wasPressed(MappedInputManager::Button::Confirm)) {
      save(input, requestUpdate, true);
      return true;
    } else if (input.wasPressed(MappedInputManager::Button::Back)) {
      cancel(input, requestUpdate, true);
      return true;
    }
    return true;
  }

  bool processRender(GfxRenderer& renderer, const MappedInputManager& input) const {
    if (!active) return false;
    const auto popupLabels =
        input.mapLabels(confirmationMode ? tr(STR_CANCEL) : tr(STR_BACK),
                        confirmationMode ? tr(STR_SAVE) : tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, popupLabels.btn1, popupLabels.btn2, popupLabels.btn3, popupLabels.btn4, true);
    render(renderer);
    renderer.displayBuffer();
    return true;
  }

  void render(const GfxRenderer& renderer) const {
    if (!active) return;
    GUI.drawOptionPopup(renderer, title.c_str(), ownedStrings, selectedIndex, confirmationMode, tr(STR_CANCEL),
                        tr(STR_SAVE));
  }

  bool isActive() const { return active; }

 private:
  struct Layout {
    Rect dialog{0, 0, 0, 0};
    std::vector<Rect> options;
    Rect cancel{0, 0, 0, 0};
    Rect save{0, 0, 0, 0};
    int firstOptionIndex = 0;
  };

  enum class TouchTarget : uint8_t { None, Option, Cancel, Save, Outside };

  // Text measurement is expensive and wasScreenTouchDown() is level-triggered, so the
  // layout is computed once per show() and cached rather than rebuilt every loop().
  const Layout& getLayout(const GfxRenderer& renderer) const {
    if (layoutValid) return layout;

    const auto& metrics = UITheme::getInstance().getMetrics();
    const auto pageWidth = renderer.getScreenWidth();
    const auto pageHeight = renderer.getScreenHeight();
    const int optionFontId = metrics.optionPopupUseSmallFont ? UI_10_FONT_ID : UI_12_FONT_ID;
    const EpdFontFamily::Style optionStyle =
        metrics.optionPopupOptionFontBold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;

    const int itemSpacing = metrics.optionPopupItemSpacing;
    const int innerPadding = metrics.optionPopupInnerPadding;
    const int selectionHPadding = metrics.optionPopupSelectionHPadding;
    const int selectionVPadding = metrics.optionPopupSelectionVPadding;

    const int optionLineHeight = renderer.getLineHeight(optionFontId);
    const int titleLineHeight = renderer.getLineHeight(UI_12_FONT_ID);
    const int rowHeight = optionLineHeight + selectionVPadding * 2;

    int maxTextWidth = renderer.getTextWidth(UI_12_FONT_ID, title.c_str(), EpdFontFamily::BOLD);
    for (const auto& opt : ownedStrings) {
      const int width = renderer.getTextWidth(optionFontId, opt.c_str(), optionStyle);
      if (width > maxTextWidth) maxTextWidth = width;
    }

    const int optionCount = static_cast<int>(ownedStrings.size());
    constexpr int footerHeight = 56;
    const int footerSpace = confirmationMode ? footerHeight : 0;
    const int maxDialogH =
        std::max(rowHeight + titleLineHeight + metrics.optionPopupTitleGap + innerPadding * 2 + footerSpace,
                 pageHeight - metrics.buttonHintsHeight - metrics.optionPopupDialogSideMargin * 2);
    const int dialogW = std::min((maxTextWidth + innerPadding * 2 + selectionHPadding * 2 + metrics.scrollBarWidth +
                                  metrics.scrollBarRightOffset + selectionHPadding) *
                                     12 / 10,
                                 pageWidth - metrics.optionPopupDialogSideMargin * 2);
    const int titleContentWidth = std::max(1, dialogW - innerPadding * 2);
    const int maxTitleLines = std::max(
        1, (maxDialogH - innerPadding * 2 - metrics.optionPopupTitleGap - rowHeight - footerSpace) / titleLineHeight);
    const auto titleLines =
        renderer.wrappedText(UI_12_FONT_ID, title.c_str(), titleContentWidth, maxTitleLines, EpdFontFamily::BOLD);
    const int titleHeight = static_cast<int>(titleLines.size()) * titleLineHeight;
    const int maxListHeight =
        std::max(rowHeight, maxDialogH - innerPadding * 2 - titleHeight - metrics.optionPopupTitleGap - footerSpace);
    const int rowStep = rowHeight + itemSpacing;
    const int visibleCount = std::max(1, std::min(optionCount, (maxListHeight + itemSpacing) / rowStep));
    const int safeSelectedIndex = std::clamp(selectedIndex, 0, optionCount - 1);
    const int visibleStart = std::clamp(safeSelectedIndex - visibleCount / 2, 0, optionCount - visibleCount);
    const int listHeight = rowHeight * visibleCount + itemSpacing * (visibleCount - 1);
    const bool hasHiddenOptions = visibleCount < optionCount;
    const int scrollBarGutter =
        hasHiddenOptions ? metrics.scrollBarWidth + metrics.scrollBarRightOffset + selectionHPadding : 0;
    const int contentHeight = titleHeight + metrics.optionPopupTitleGap + listHeight;
    const int dialogH = contentHeight + innerPadding * 2 + footerSpace;
    const int dialogX = (pageWidth - dialogW) / 2;
    const int dialogY = (pageHeight - dialogH) / 2;
    const int itemRectX = dialogX + innerPadding;
    const int itemRectW = std::max(1, dialogW - innerPadding * 2 - scrollBarGutter);
    const int firstItemY = dialogY + innerPadding + titleHeight + metrics.optionPopupTitleGap;

    layout.dialog = Rect{dialogX, dialogY, dialogW, dialogH};
    layout.firstOptionIndex = visibleStart;
    layout.cancel =
        confirmationMode ? Rect{dialogX, dialogY + dialogH - footerHeight, dialogW / 2, footerHeight} : Rect{};
    layout.save = confirmationMode ? Rect{dialogX + dialogW / 2, dialogY + dialogH - footerHeight,
                                          dialogW - dialogW / 2, footerHeight}
                                   : Rect{};
    layout.options.clear();
    layout.options.reserve(visibleCount);
    for (int i = 0; i < visibleCount; i++) {
      layout.options.push_back(Rect{itemRectX, firstItemY + i * (rowHeight + itemSpacing), itemRectW, rowHeight});
    }
    layoutValid = true;
    return layout;
  }

  static bool contains(const Rect& rect, const int x, const int y) {
    return x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height;
  }

  bool active = false;
  bool dismissOnOutsideTouchDown = false;
  bool confirmationMode = false;
  std::string title;
  std::vector<std::string> ownedStrings;
  int selectedIndex = 0;
  int touchDownOptionIndex = -1;
  TouchTarget touchDownTarget = TouchTarget::None;
  std::function<void(int)> onSelectCallback;
  std::function<void()> onCancelCallback;
  mutable Layout layout;
  mutable bool layoutValid = false;

  void activate(int currentIndex) {
    layoutValid = false;
    touchDownOptionIndex = -1;
    touchDownTarget = TouchTarget::None;
    if (ownedStrings.empty()) {
      active = false;
      onSelectCallback = nullptr;
      selectedIndex = 0;
      return;
    }

    const int count = static_cast<int>(ownedStrings.size());
    if (currentIndex < 0) {
      selectedIndex = 0;
    } else if (currentIndex >= count) {
      selectedIndex = count - 1;
    } else {
      selectedIndex = currentIndex;
    }
    active = true;
  }

  void prepareStandardShow() {
    confirmationMode = false;
    onCancelCallback = nullptr;
  }

  void save(MappedInputManager& input, const std::function<void()>& requestUpdate, const bool suppressRelease) {
    active = false;
    if (suppressRelease) input.suppressNextConfirmRelease();
    if (onSelectCallback) onSelectCallback(selectedIndex);
    requestUpdate();
  }

  void cancel(MappedInputManager& input, const std::function<void()>& requestUpdate, const bool suppressRelease) {
    active = false;
    if (suppressRelease) input.suppressNextBackRelease();
    if (onCancelCallback) onCancelCallback();
    requestUpdate();
  }
};
