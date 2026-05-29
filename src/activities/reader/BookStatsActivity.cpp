#include "BookStatsActivity.h"

#include "BookStatsView.h"
#include "MappedInputManager.h"

BookStatsActivity::BookStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const std::string& title,
                                     const BookReadingStats& stats, const GlobalReadingStats& globalStats)
    : Activity("BookStats", renderer, mappedInput), bookTitle(title), stats(stats), globalStats(globalStats) {}

BookStatsActivity::BookStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const std::string& title,
                                     const BookReadingStats& stats, const GlobalReadingStats& globalStats,
                                     const GlobalReadingStats& allDevicesStats)
    : Activity("BookStats", renderer, mappedInput),
      bookTitle(title),
      stats(stats),
      globalStats(globalStats),
      allDevicesStats(allDevicesStats),
      showAllDevicesStats(true) {}

void BookStatsActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void BookStatsActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back) ||
      mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    finish();
  }
}

void BookStatsActivity::render(RenderLock&&) {
  renderBookStatsView(renderer, &mappedInput, bookTitle, stats, globalStats,
                      showAllDevicesStats ? &allDevicesStats : nullptr, true);
  renderer.displayBuffer();
}
