#pragma once
#include <FreeInkUIGfxRenderer.h>
#include <HalGPIO.h>

#include "CrossPointSettings.h"
#include "UITheme.h"

namespace UiThemeTokensDetail {
inline int16_t scaledListMetric(const int metric) {
  constexpr int baseFontSize = 10;
  int scaleFontSize = baseFontSize;
  switch (SETTINGS.uiScale) {
    case CrossPointSettings::UI_SCALE_LARGE:
      scaleFontSize = 12;
      break;
    case CrossPointSettings::UI_SCALE_SMALL:
    default:
      break;
  }
  return static_cast<int16_t>((metric * scaleFontSize + baseFontSize / 2) / baseFontSize);
}
}  // namespace UiThemeTokensDetail

enum class UiListRowType : uint8_t {
  SingleLine,
  WithSubtitle,
};

inline int16_t uiListRowHeight(const freeink::ui::ThemeTokens& tokens, const UiListRowType rowType) {
  if (gpio.hasTouch()) return tokens.rowHeight;

  const ThemeMetrics& metrics = UITheme::getInstance().getMetrics();
  switch (rowType) {
    case UiListRowType::SingleLine:
      return UiThemeTokensDetail::scaledListMetric(metrics.listRowHeight);
    case UiListRowType::WithSubtitle:
      return UiThemeTokensDetail::scaledListMetric(metrics.listWithSubtitleRowHeight);
  }
  return tokens.rowHeight;
}

inline uint16_t configureUiList(freeink::ui::ListProps& props, const freeink::ui::ThemeTokens& tokens,
                                const freeink::ui::Rect rect, const UiListRowType rowType = UiListRowType::SingleLine) {
  if (props.rowHeight <= 0) props.rowHeight = uiListRowHeight(tokens, rowType);
  if (props.rowGap < 0) props.rowGap = tokens.listRowGap;
  return freeink::ui::listVisibleRows(rect, props.rowHeight, props.rowGap);
}

// Merges the active UITheme's shape with uiScale-derived sizes. Touch builds
// retain FreeInkUI's larger font-derived rows; button devices use the theme's
// compact one-line/two-line metrics, scaled with the selected UI font size.
inline freeink::ui::ThemeTokens uiThemeTokens(const freeink::ui::GfxRendererTarget& target) {
  namespace fui = freeink::ui;
  const ThemeMetrics& metrics = UITheme::getInstance().getMetrics();

  fui::ThemeTokens tokens = fui::themeTokensForLineHeight(target.lineHeight(fui::GfxRendererTarget::FONT_BODY));
  tokens.listRowGap = static_cast<int16_t>(metrics.listRowGap);
  tokens.listRowRadius = static_cast<uint8_t>(metrics.listRowRadius);
  tokens.listInset = static_cast<int16_t>(metrics.listInset);
  tokens.listSidePadding = static_cast<int16_t>(metrics.listSidePadding);
  tokens.listSelectionStyle = static_cast<fui::SelectionStyle>(metrics.listSelectionStyle);
  tokens.listScrollWidth = static_cast<int16_t>(metrics.listScrollWidth);
  tokens.listScrollSide = static_cast<uint8_t>(metrics.listScrollSide);
  tokens.headerSidePadding = static_cast<int16_t>(metrics.headerSidePadding);
  tokens.headerUnderline = static_cast<uint8_t>(metrics.headerUnderlineSize);
  tokens.headerTitleAlign = static_cast<fui::TextAlign>(metrics.headerTitleAlign);
  tokens.bodyText.bold = metrics.listTitleBold;
  return tokens;
}
