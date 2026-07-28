#include "TouchHeaderBackButton.h"

#include <FreeInkUIIcon.h>

#include <algorithm>

#include "UIThemeTokens.h"
#include "components/CompactHeader.h"
#include "components/HeaderDate.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "components/icons/touchHeaderIcons.h"
#include "fontIds.h"

namespace fui = freeink::ui;

namespace TouchHeaderBackButton {

namespace {
constexpr int actionSize = 52;
constexpr int touchSize = 68;
constexpr int titleGap = 8;
constexpr int defaultRightReserve = 52;
}  // namespace

Layout layout(const Rect& header) {
  const int actionWidth = std::min(actionSize, header.width);
  const int actionHeight = std::min(actionSize, header.height);
  // Center the action lane vertically so taller theme headers split their
  // spare space above and below the back button/title instead of placing all
  // of it above the title.
  const int actionX = header.x;
  const int actionY = header.y + (header.height - actionHeight) / 2;
  const int touchWidth = std::min(touchSize, header.width);
  const int touchHeight = touchSize;
  const int touchX = std::max(header.x, actionX + (actionWidth - touchWidth) / 2);
  // The common 45px header is shorter than the minimum touch target. Let the
  // hit box extend above and below the visual band instead of shrinking it.
  const int touchY = std::max(0, actionY + (actionHeight - touchHeight) / 2);
  return {Rect{actionX, actionY, actionWidth, actionHeight}, Rect{touchX, touchY, touchWidth, touchHeight},
          actionX + actionWidth + titleGap};
}

Rect standardHeaderRect(const GfxRenderer& renderer) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  return Rect{0, metrics.topPadding, renderer.getScreenWidth(), metrics.headerHeight};
}

Rect compactHeaderRect(const GfxRenderer& renderer) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  return Rect{0, metrics.topPadding, renderer.getScreenWidth(),
              CompactHeader::headerBottomY(metrics) - metrics.topPadding};
}

bool wasTapped(const MappedInputManager& input, const Rect& header) {
  if (!input.hasTouchHardware()) return false;
  const Rect touchRect = layout(header).touchRect;
  return input.wasTapInRect(touchRect.x, touchRect.y, touchRect.width, touchRect.height);
}

bool wasTapped(const MappedInputManager& input, const GfxRenderer& renderer) {
  return wasTapped(input, standardHeaderRect(renderer));
}

void draw(GfxRenderer& renderer, const Rect& header, const char* title, const bool readerContext,
          const int rightReserve) {
  auto target = makeUiTarget(renderer);
  draw(renderer, target, header, title, readerContext, rightReserve);
}

void draw(GfxRenderer& renderer, fui::GfxRendererTarget& target, const Rect& header, const char* title,
          const bool readerContext, const int rightReserve) {
  const Layout back = layout(header);
  const Rect visualHeader{header.x, header.y, header.width, back.iconRect.y + back.iconRect.height - header.y};
  GUI.drawHeader(renderer, visualHeader, "", nullptr, readerContext);

  fui::TextStyle titleStyle = uiThemeTokens(target).titleText;
  titleStyle.align = fui::TextAlign::Left;
  titleStyle.maxLines = 1;
  const int reservedRight = rightReserve > 0 ? rightReserve : defaultRightReserve;
  const int titleWidth = std::max(0, header.x + header.width - reservedRight - back.titleX);
  target.text(fui::Rect{static_cast<int16_t>(back.titleX), static_cast<int16_t>(back.iconRect.y),
                        static_cast<int16_t>(titleWidth), static_cast<int16_t>(back.iconRect.height)},
              title, titleStyle);

  const int iconX = back.iconRect.x + (back.iconRect.width - ICON_SIZE) / 2;
  const int iconY = back.iconRect.y + (back.iconRect.height - ICON_SIZE) / 2;
  target.bitmap(fui::Rect{static_cast<int16_t>(iconX), static_cast<int16_t>(iconY), ICON_SIZE, ICON_SIZE},
                fui::bitmapFromIcon(icon_back_32), fui::BitmapMode::Center);
}

void drawCompact(GfxRenderer& renderer, const char* title, const bool readerContext, const bool showDate) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect header = compactHeaderRect(renderer);
  const int rightReserve =
      metrics.batteryWidth + 2 * metrics.headerSidePadding + (showDate ? headerDateReservedWidth(renderer) : 0);
  draw(renderer, header, title, readerContext, rightReserve);
  if (showDate) {
    const Layout back = layout(header);
    const int titleBaselineY =
        back.iconRect.y + std::max(0, (back.iconRect.height - renderer.getLineHeight(UI_12_FONT_ID)) / 2) +
        renderer.getFontAscenderSize(UI_12_FONT_ID);
    drawHeaderDateAtBaseline(renderer, renderer.getScreenWidth(), titleBaselineY);
  }
}

}  // namespace TouchHeaderBackButton
