#include "DictionarySelectActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "CrossPointSettings.h"
#include "I18nKeys.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/DictionaryRegistry.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DictionarySelectActivity::onEnter() {
  Activity::onEnter();

  // Suppress Confirm bleed-through only in settings mode: when launched from a list,
  // the Confirm release that opened the picker fires again in the picker's first loop.
  // In per-book mode (launched via reader menu) the menu already consumed the event.
  ignoreNextConfirmRelease = bookCachePath.empty();

  scanDictionaries();

  if (bookCachePath.empty()) {
    // Settings mode: validate global path, pre-select from SETTINGS.
    Dictionary::isValidDictionary();

    selectedIndex = 0;  // default: None
    {
      const std::string activePath = Dictionary::readDictPath();
      if (!activePath.empty()) {
        for (int i = 0; i < static_cast<int>(dictFolders.size()); i++) {
          if (folderForIndex(i + 1) == activePath) {
            selectedIndex = i + 1;
            break;
          }
        }
      }
    }
  } else {
    // Per-book mode: read saved per-book path, pre-select it.
    currentBookDictPath = "";
    HalFile f;
    if (Storage.openFileForRead("DSEL", bookCachePath + "/dictionary.bin", f)) {
      const int sz = static_cast<int>(f.fileSize());
      if (sz > 0) {
        std::string path(sz, '\0');
        const int n = f.read(&path[0], sz);
        if (n > 0) {
          path.resize(n);
          currentBookDictPath = path;
        }
      }
      f.close();
    }

    selectedIndex = 0;  // default: Use Global
    if (!currentBookDictPath.empty()) {
      for (int i = 0; i < static_cast<int>(dictFolders.size()); i++) {
        if (folderForIndex(i + 1) == currentBookDictPath) {
          selectedIndex = i + 1;
          break;
        }
      }
    }

    // Build augmented "Use Global" label showing the active global dictionary name.
    // Path format: <dictRoot>/<folder>/<stem> — extract <folder>.
    const std::string globalPath = Dictionary::readDictPath();
    std::string globalFolderName;
    if (globalPath.empty()) {
      globalFolderName = tr(STR_DICT_NONE);
    } else {
      const size_t lastSlash = globalPath.rfind('/');
      if (lastSlash != std::string::npos && lastSlash > 0) {
        const size_t prevSlash = globalPath.rfind('/', lastSlash - 1);
        globalFolderName = (prevSlash != std::string::npos)
                               ? globalPath.substr(prevSlash + 1, lastSlash - prevSlash - 1)
                               : globalPath.substr(0, lastSlash);
      } else {
        globalFolderName = globalPath;
      }
    }
    useGlobalLabel = std::string(tr(STR_DICT_USE_GLOBAL)) + " (" + globalFolderName + ")";
    currentEffectiveDictPath = Dictionary::readDictPath(bookCachePath.c_str());
  }

  totalItems = 1 + static_cast<int>(dictFolders.size());
  if (disableCurrentSelection) {
    selectedIndex = firstSelectableIndexFrom(selectedIndex);
  }
  requestUpdate();
}

void DictionarySelectActivity::onExit() {
  dictionaryRegistry.clear();
  Activity::onExit();
}

// ---------------------------------------------------------------------------
// SD card scan
// ---------------------------------------------------------------------------

void DictionarySelectActivity::scanDictionaries() {
  // Discovery lives in DictionaryRegistry (shared with the settings list and web UI).
  // Re-scan on every picker open (matches prior behaviour), then mirror the results into
  // the activity's parallel vectors so folderForIndex()/metadata/per-book logic is unchanged.
  dictionaryRegistry.discover();
  dictRoot = dictionaryRegistry.root();
  dictFolders.clear();
  dictStems.clear();
  const auto& entries = dictionaryRegistry.getEntries();
  dictFolders.reserve(entries.size());
  dictStems.reserve(entries.size());
  for (const auto& e : entries) {
    dictFolders.push_back(e.name);
    dictStems.push_back(e.stem);
  }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string DictionarySelectActivity::folderForIndex(int index) const {
  if (index <= 0 || index > static_cast<int>(dictFolders.size())) return "";
  return dictRoot + "/" + dictFolders[index - 1] + "/" + dictStems[index - 1];
}

std::string DictionarySelectActivity::effectiveFolderForIndex(int index) const {
  if (index == 0 && !bookCachePath.empty()) return Dictionary::readDictPath();
  return folderForIndex(index);
}

bool DictionarySelectActivity::rowIsDisabled(int index) const {
  if (!disableCurrentSelection || currentEffectiveDictPath.empty()) return false;
  return effectiveFolderForIndex(index) == currentEffectiveDictPath;
}

int DictionarySelectActivity::firstSelectableIndexFrom(int start) const {
  if (totalItems <= 0) return 0;
  for (int offset = 0; offset < totalItems; offset++) {
    const int idx = (start + offset) % totalItems;
    if (!rowIsDisabled(idx)) return idx;
  }
  return start;
}

const char* DictionarySelectActivity::nameForIndex(int index) const {
  if (index == 0) return bookCachePath.empty() ? tr(STR_DICT_NONE) : useGlobalLabel.c_str();
  if (index <= static_cast<int>(dictFolders.size())) return dictFolders[index - 1].c_str();
  return "";
}

bool DictionarySelectActivity::applySelection() {
  if (rowIsDisabled(selectedIndex)) return false;

  std::string folder = folderForIndex(selectedIndex);

  if (bookCachePath.empty()) {
    // Settings mode: update global dictionary.bin.
    if (Dictionary::readDictPath() == folder) return false;
    Dictionary::saveGlobalDictPath(folder.c_str());
  } else {
    // Per-book mode: save to book cache.
    if (currentBookDictPath == folder) return false;
    HalFile f;
    if (Storage.openFileForWrite("DSEL", bookCachePath + "/dictionary.bin", f)) {
      f.write(reinterpret_cast<const uint8_t*>(folder.c_str()), folder.size());
      f.close();
    } else {
      LOG_ERR("DSEL", "Could not save per-book dictionary");
    }
    currentBookDictPath = folder;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void DictionarySelectActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (ignoreNextConfirmRelease) {
      ignoreNextConfirmRelease = false;
      return;
    }

    if (!applySelection()) {
      ActivityResult result;
      result.isCancelled = true;
      setResult(std::move(result));
    }
    finish();
    return;
  }

  const int pageItems = UITheme::getNumberOfItemsPerPage(renderer, true, false, true, false);

  buttonNavigator.onNextRelease([this] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, totalItems);
    selectedIndex = firstSelectableIndexFrom(selectedIndex);
    requestUpdate();
  });

  buttonNavigator.onPreviousRelease([this] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, totalItems);
    if (totalItems > 0) {
      for (int offset = 0; offset < totalItems && rowIsDisabled(selectedIndex); offset++) {
        selectedIndex = ButtonNavigator::previousIndex(selectedIndex, totalItems);
      }
    }
    requestUpdate();
  });

  buttonNavigator.onNextContinuous([this, pageItems] {
    selectedIndex = ButtonNavigator::nextPageIndex(selectedIndex, totalItems, pageItems);
    selectedIndex = firstSelectableIndexFrom(selectedIndex);
    requestUpdate();
  });

  buttonNavigator.onPreviousContinuous([this, pageItems] {
    selectedIndex = ButtonNavigator::previousPageIndex(selectedIndex, totalItems, pageItems);
    if (totalItems > 0) {
      for (int offset = 0; offset < totalItems && rowIsDisabled(selectedIndex); offset++) {
        selectedIndex = ButtonNavigator::previousIndex(selectedIndex, totalItems);
      }
    }
    requestUpdate();
  });
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void DictionarySelectActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const auto& metrics = UITheme::getInstance().getMetrics();

  // --- Picker screen ---
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_DICTIONARY));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  // Show "None found" note when no dictionaries are available
  if (dictFolders.empty()) {
    const int textY = contentTop + contentHeight / 3;
    renderer.drawCenteredText(UI_10_FONT_ID, textY, tr(STR_DICT_NONE_FOUND));
  }

  GUI.drawList(
      renderer, Rect{0, contentTop, pageWidth, contentHeight}, totalItems, selectedIndex,
      [this](int index) { return std::string(nameForIndex(index)); }, nullptr, nullptr,
      [this](int index) -> std::string {
        // Show "Selected" marker for the currently active dictionary.
        // In per-book mode compare against currentBookDictPath; in settings mode against global.
        std::string folder = folderForIndex(index);
        const std::string activePath = disableCurrentSelection
                                           ? currentEffectiveDictPath
                                           : (bookCachePath.empty() ? Dictionary::readDictPath() : currentBookDictPath);
        if (folder.empty() && activePath.empty()) return tr(STR_SELECTED);
        if (!folder.empty() && folder == activePath) return tr(STR_SELECTED);
        return "";
      },
      true, [this](int index) { return rowIsDisabled(index); });

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
