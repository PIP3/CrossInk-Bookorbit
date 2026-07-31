#include "BookOrbitSettingsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <cstring>

#include "BookOrbitCredentialStore.h"
#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "activities/browser/BookOrbitCatalogBrowserActivity.h"
#include "activities/settings/BookOrbitAuthActivity.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int MENU_ITEMS = 5;
const StrId menuNames[MENU_ITEMS] = {StrId::STR_USERNAME, StrId::STR_PASSWORD, StrId::STR_BOOKORBIT_SERVER_URL,
                                     StrId::STR_AUTHENTICATE, StrId::STR_BOOKORBIT_CATALOG};
}  // namespace

void BookOrbitSettingsActivity::onEnter() {
  Activity::onEnter();

  selectedIndex = 0;
  requestUpdate();
}

void BookOrbitSettingsActivity::onExit() { Activity::onExit(); }

void BookOrbitSettingsActivity::loop() {
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    finishAfterBackPress();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    handleSelection();
    return;
  }

  buttonNavigator.onNext([this] {
    selectedIndex = (selectedIndex + 1) % MENU_ITEMS;
    requestUpdate();
  });

  buttonNavigator.onPrevious([this] {
    selectedIndex = (selectedIndex + MENU_ITEMS - 1) % MENU_ITEMS;
    requestUpdate();
  });
}

void BookOrbitSettingsActivity::handleSelection() {
  if (selectedIndex == 0) {
    // Username
    startActivityForResult(std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, tr(STR_BOOKORBIT_USERNAME),
                                                                   BOOKORBIT_STORE.getUsername(), 64, InputType::Text),
                           [this](const ActivityResult& result) {
                             if (!result.isCancelled) {
                               const auto& kb = std::get<KeyboardResult>(result.data);
                               BOOKORBIT_STORE.setCredentials(kb.text, BOOKORBIT_STORE.getPassword());
                               BOOKORBIT_STORE.saveToFile();
                             }
                           });
  } else if (selectedIndex == 1) {
    // Password
    startActivityForResult(
        std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, tr(STR_BOOKORBIT_PASSWORD),
                                                BOOKORBIT_STORE.getPassword(), 64, InputType::Password),
        [this](const ActivityResult& result) {
          if (!result.isCancelled) {
            const auto& kb = std::get<KeyboardResult>(result.data);
            BOOKORBIT_STORE.setCredentials(BOOKORBIT_STORE.getUsername(), kb.text);
            BOOKORBIT_STORE.saveToFile();
          }
        });
  } else if (selectedIndex == 2) {
    // Server URL - prefill with https:// if empty to save typing
    const std::string currentUrl = BOOKORBIT_STORE.getServerUrl();
    const std::string prefillUrl = currentUrl.empty() ? "https://" : currentUrl;
    startActivityForResult(std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, tr(STR_BOOKORBIT_SERVER_URL),
                                                                   prefillUrl, 128, InputType::Url),
                           [this](const ActivityResult& result) {
                             if (!result.isCancelled) {
                               const auto& kb = std::get<KeyboardResult>(result.data);
                               const std::string urlToSave =
                                   (kb.text == "https://" || kb.text == "http://") ? "" : kb.text;
                               BOOKORBIT_STORE.setServerUrl(urlToSave);
                               BOOKORBIT_STORE.saveToFile();
                             }
                           });
  } else if (selectedIndex == 3) {
    // Authenticate
    if (!BOOKORBIT_STORE.hasCredentials()) {
      // Can't authenticate without credentials - just show message briefly
      return;
    }
    startActivityForResult(std::make_unique<BookOrbitAuthActivity>(renderer, mappedInput),
                           [](const ActivityResult&) {});
  } else if (selectedIndex == 4) {
    // Browse Catalog
    if (!BOOKORBIT_STORE.hasCredentials()) {
      return;
    }
    activityManager.goToBookOrbitCatalog();
  }
}

void BookOrbitSettingsActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_BOOKORBIT_SYNC));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing * 2;
  GUI.drawList(
      renderer, Rect{0, contentTop, pageWidth, contentHeight}, static_cast<int>(MENU_ITEMS),
      static_cast<int>(selectedIndex), [](int index) { return std::string(I18N.get(menuNames[index])); }, nullptr,
      nullptr,
      [this](int index) {
        if (index == 0) {
          auto username = BOOKORBIT_STORE.getUsername();
          return username.empty() ? std::string(tr(STR_NOT_SET)) : username;
        } else if (index == 1) {
          return BOOKORBIT_STORE.getPassword().empty() ? std::string(tr(STR_NOT_SET)) : std::string("******");
        } else if (index == 2) {
          auto serverUrl = BOOKORBIT_STORE.getServerUrl();
          return serverUrl.empty() ? std::string(tr(STR_NOT_SET)) : serverUrl;
        } else if (index == 3) {
          return BOOKORBIT_STORE.hasCredentials() ? "" : std::string("[") + tr(STR_SET_CREDENTIALS_FIRST) + "]";
        } else if (index == 4) {
          return BOOKORBIT_STORE.hasCredentials() ? "" : std::string("[") + tr(STR_SET_CREDENTIALS_FIRST) + "]";
        }
        return std::string(tr(STR_NOT_SET));
      },
      true);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
