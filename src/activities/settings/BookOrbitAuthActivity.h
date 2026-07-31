#pragma once

#include <functional>

#include "activities/Activity.h"

/**
 * Activity for testing BookOrbit credentials.
 * Connects to WiFi and authenticates with the BookOrbit server.
 */
class BookOrbitAuthActivity final : public Activity {
 public:
  explicit BookOrbitAuthActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("BookOrbitAuth", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return state == CONNECTING || state == AUTHENTICATING; }

 private:
  enum State { WIFI_SELECTION, CONNECTING, AUTHENTICATING, SUCCESS, FAILED };

  State state = WIFI_SELECTION;
  std::string statusMessage;
  std::string errorMessage;

  void onWifiSelectionComplete(bool success);
  void performAuthentication();
};
