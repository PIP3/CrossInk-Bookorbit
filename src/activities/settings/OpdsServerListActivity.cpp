#include "OpdsServerListActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include <cstring>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "OpdsServerStore.h"
#include "OpdsSettingsActivity.h"
#include "activities/ActivityManager.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
std::string normalizeDownloadFolder(std::string folder) {
  while (!folder.empty() && (folder.front() == ' ' || folder.front() == '\t')) folder.erase(folder.begin());
  while (!folder.empty() && (folder.back() == ' ' || folder.back() == '\t')) folder.pop_back();
  if (folder.empty() || folder == "/") return "";
  if (folder.front() != '/') folder.insert(folder.begin(), '/');
  while (folder.size() > 1 && folder.back() == '/') folder.pop_back();
  return folder;
}
}  // namespace

int OpdsServerListActivity::getItemCount() const {
  int count = static_cast<int>(OPDS_STORE.getCount());
  // In settings mode, append virtual "Add Server" and "Download Folder" items.
  if (!pickerMode) {
    count += 2;
  }
  return count;
}

void OpdsServerListActivity::onEnter() {
  Activity::onEnter();

  // Reload from disk in case servers were added/removed by a subactivity or the web UI
  OPDS_STORE.loadFromFile();
  selectedIndex = 0;
  requestUpdate();
}

void OpdsServerListActivity::onExit() { Activity::onExit(); }

void OpdsServerListActivity::loop() {
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    if (pickerMode) {
      activityManager.goHome(HomeMenuItem::OPDS_BROWSER);
    } else {
      finish();
    }
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    handleSelection();
    return;
  }

  const int itemCount = getItemCount();
  if (itemCount > 0) {
    buttonNavigator.onNext([this, itemCount] {
      selectedIndex = ButtonNavigator::nextIndex(selectedIndex, itemCount);
      requestUpdate();
    });

    buttonNavigator.onPrevious([this, itemCount] {
      selectedIndex = ButtonNavigator::previousIndex(selectedIndex, itemCount);
      requestUpdate();
    });
  }
}

void OpdsServerListActivity::handleSelection() {
  const auto serverCount = static_cast<int>(OPDS_STORE.getCount());

  if (pickerMode) {
    // Picker mode: selecting a server navigates to the OPDS browser
    if (selectedIndex < serverCount) {
      activityManager.goToOpdsServer(static_cast<uint32_t>(selectedIndex));
    }
    return;
  }

  // Item layout: configured servers, Add Server, Download Folder.
  if (selectedIndex == serverCount + 1) {
    auto resultHandler = [this](const ActivityResult& result) {
      if (result.isCancelled) return;

      const auto& keyboardResult = std::get<KeyboardResult>(result.data);
      const std::string folder = normalizeDownloadFolder(keyboardResult.text);
      strncpy(SETTINGS.opdsDownloadFolder, folder.c_str(), sizeof(SETTINGS.opdsDownloadFolder) - 1);
      SETTINGS.opdsDownloadFolder[sizeof(SETTINGS.opdsDownloadFolder) - 1] = '\0';
      if (!SETTINGS.saveToFile()) {
        LOG_ERR("OPDS", "Could not save download folder setting");
      }
      requestUpdate();
    };
    startActivityForResult(std::make_unique<KeyboardEntryActivity>(
                               renderer, mappedInput, tr(STR_OPDS_DOWNLOAD_FOLDER), SETTINGS.opdsDownloadFolder,
                               sizeof(SETTINGS.opdsDownloadFolder) - 1, InputType::Text),
                           resultHandler);
    return;
  }

  // Settings mode: open editor for selected server, or create a new one
  auto resultHandler = [this](const ActivityResult&) {
    // Reload server list when returning from editor
    OPDS_STORE.loadFromFile();
    selectedIndex = 0;
  };

  if (selectedIndex < serverCount) {
    startActivityForResult(std::make_unique<OpdsSettingsActivity>(renderer, mappedInput, selectedIndex), resultHandler);
  } else {
    startActivityForResult(std::make_unique<OpdsSettingsActivity>(renderer, mappedInput, -1), resultHandler);
  }
}

void OpdsServerListActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_OPDS_SERVERS));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing * 2;
  const int itemCount = getItemCount();

  if (itemCount == 0) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2, tr(STR_NO_SERVERS));
  } else {
    const auto& servers = OPDS_STORE.getServers();
    const auto serverCount = static_cast<int>(servers.size());

    // Primary label: server name (falling back to URL if unnamed).
    // Secondary label: server URL (shown as subtitle when name is set).
    GUI.drawList(
        renderer, Rect{0, contentTop, pageWidth, contentHeight}, itemCount, selectedIndex,
        [&servers, serverCount](int index) -> std::string {
          if (index < serverCount) {
            const auto& server = servers[index];
            return server.name.empty() ? server.url : server.name;
          }
          const StrId label = index == serverCount ? StrId::STR_ADD_SERVER : StrId::STR_OPDS_DOWNLOAD_FOLDER;
          return std::string(I18n::getInstance().get(label));
        },
        [&servers, serverCount](int index) -> std::string {
          if (index < serverCount && !servers[index].name.empty()) {
            return servers[index].url;
          }
          if (index == serverCount + 1) {
            return SETTINGS.opdsDownloadFolder[0] ? std::string(SETTINGS.opdsDownloadFolder)
                                                  : std::string(I18n::getInstance().get(StrId::STR_OPDS_SD_ROOT));
          }
          return std::string("");
        });
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
