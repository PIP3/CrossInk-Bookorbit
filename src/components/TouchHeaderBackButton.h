#pragma once

#include <FreeInkUIGfxRenderer.h>

#include "GfxRenderer.h"
#include "MappedInputManager.h"
#include "themes/BaseTheme.h"

namespace TouchHeaderBackButton {

constexpr int ICON_SIZE = 32;

struct Layout {
  Rect iconRect;
  Rect touchRect;
  int titleX;
};

Layout layout(const Rect& header);
Rect standardHeaderRect(const GfxRenderer& renderer);
Rect compactHeaderRect(const GfxRenderer& renderer);
bool wasTapped(const MappedInputManager& input, const Rect& header);
bool wasTapped(const MappedInputManager& input, const GfxRenderer& renderer);
void draw(GfxRenderer& renderer, const Rect& header, const char* title, bool readerContext, int rightReserve = 0);
void draw(GfxRenderer& renderer, freeink::ui::GfxRendererTarget& target, const Rect& header, const char* title,
          bool readerContext, int rightReserve = 0);
void drawCompact(GfxRenderer& renderer, const char* title, bool readerContext = false, bool showDate = false);

}  // namespace TouchHeaderBackButton
