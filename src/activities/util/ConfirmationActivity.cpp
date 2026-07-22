#include "ConfirmationActivity.h"

#include <I18n.h>

#include <algorithm>

#include "components/UITheme.h"
#include "fontIds.h"

ConfirmationActivity::ConfirmationActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                           const std::string& heading, const std::string& body,
                                           bool ignoreInitialConfirmRelease, bool overrideDisabledReaderTouchscreen)
    : Activity("Confirmation", renderer, mappedInput),
      ignoreConfirmRelease(ignoreInitialConfirmRelease),
      overrideDisabledReaderTouchscreen(overrideDisabledReaderTouchscreen) {
  popupTitle.reserve(heading.size() + body.size() + 1);
  popupTitle = heading;
  if (!heading.empty() && !body.empty()) {
    popupTitle += ' ';
  }
  popupTitle += body;
}

void ConfirmationActivity::onEnter() {
  Activity::onEnter();
  if (overrideDisabledReaderTouchscreen) {
    mappedInput.setReaderTouchscreenOverride(true);
  }

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int popupSideInset = metrics.optionPopupDialogSideMargin + metrics.optionPopupInnerPadding;
  const int maxTitleWidth = std::max(1, renderer.getScreenWidth() - popupSideInset * 2);
  popupTitle = renderer.truncatedText(UI_12_FONT_ID, popupTitle.c_str(), maxTitleWidth, EpdFontFamily::BOLD);

  const char* options[] = {I18N.get(StrId::STR_CANCEL), I18N.get(StrId::STR_CONFIRM)};
  confirmPopup.show(popupTitle.c_str(), options, 2, 0, [this](int idx) {
    ActivityResult res;
    res.isCancelled = (idx != 1);
    setResult(std::move(res));
    finish();
  });

  requestUpdate(true);
}

void ConfirmationActivity::onExit() {
  if (overrideDisabledReaderTouchscreen) {
    mappedInput.setReaderTouchscreenOverride(false);
  }
  Activity::onExit();
}

void ConfirmationActivity::render(RenderLock&&) {
  if (confirmPopup.processRender(renderer, mappedInput)) return;
}

void ConfirmationActivity::loop() {
  if (ignoreConfirmRelease) {
    const bool confirmReleased = mappedInput.wasReleased(MappedInputManager::Button::Confirm);
    if (confirmReleased || !mappedInput.isPressed(MappedInputManager::Button::Confirm)) {
      ignoreConfirmRelease = false;
      return;
    }
  }

  if (confirmPopup.handleInput(mappedInput, [this] { requestUpdate(); })) return;

  // Popup dismissed without a selection (Back button or tap outside): cancel.
  ActivityResult res;
  res.isCancelled = true;
  setResult(std::move(res));
  finish();
}
