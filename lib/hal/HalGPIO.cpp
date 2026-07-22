#include <BatteryMonitor.h>
#include <BoardConfig.h>
#include <HalGPIO.h>
#include <Logging.h>
#include <PowerManager.h>
#include <Preferences.h>
#include <SPI.h>
#include <XteinkDetect.h>
#include <esp_sleep.h>

#ifdef STICKY_SIDE_BUTTON_DIAGNOSTICS
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

// Global HalGPIO instance
HalGPIO gpio;

namespace {
#ifdef STICKY_SIDE_BUTTON_DIAGNOSTICS
constexpr uint8_t SIDE_BUTTON_DIAGNOSTIC_QUEUE_SIZE = 32;

struct SideButtonDiagnosticEdge {
  TickType_t tick;
  int8_t pin;
  uint8_t level;
};

DRAM_ATTR SideButtonDiagnosticEdge sideButtonDiagnosticEdges[SIDE_BUTTON_DIAGNOSTIC_QUEUE_SIZE] = {};
DRAM_ATTR volatile uint8_t sideButtonDiagnosticReadIndex = 0;
DRAM_ATTR volatile uint8_t sideButtonDiagnosticWriteIndex = 0;
DRAM_ATTR volatile uint32_t sideButtonDiagnosticDroppedEdges = 0;
DRAM_ATTR portMUX_TYPE sideButtonDiagnosticMux = portMUX_INITIALIZER_UNLOCKED;
TickType_t sideButtonDiagnosticLastUpdateTick = 0;

void IRAM_ATTR recordSideButtonDiagnosticEdge(void* context) {
  const int8_t pin = static_cast<int8_t>(reinterpret_cast<intptr_t>(context));
  const SideButtonDiagnosticEdge edge = {xTaskGetTickCountFromISR(), pin,
                                         static_cast<uint8_t>(gpio_get_level(static_cast<gpio_num_t>(pin)))};

  portENTER_CRITICAL_ISR(&sideButtonDiagnosticMux);
  const uint8_t nextWriteIndex =
      static_cast<uint8_t>((sideButtonDiagnosticWriteIndex + 1) % SIDE_BUTTON_DIAGNOSTIC_QUEUE_SIZE);
  if (nextWriteIndex == sideButtonDiagnosticReadIndex) {
    sideButtonDiagnosticDroppedEdges = sideButtonDiagnosticDroppedEdges + 1;
  } else {
    sideButtonDiagnosticEdges[sideButtonDiagnosticWriteIndex] = edge;
    sideButtonDiagnosticWriteIndex = nextWriteIndex;
  }
  portEXIT_CRITICAL_ISR(&sideButtonDiagnosticMux);
}

bool popSideButtonDiagnosticEdge(SideButtonDiagnosticEdge& edge) {
  bool hasEdge = false;
  portENTER_CRITICAL(&sideButtonDiagnosticMux);
  if (sideButtonDiagnosticReadIndex != sideButtonDiagnosticWriteIndex) {
    edge = sideButtonDiagnosticEdges[sideButtonDiagnosticReadIndex];
    sideButtonDiagnosticReadIndex =
        static_cast<uint8_t>((sideButtonDiagnosticReadIndex + 1) % SIDE_BUTTON_DIAGNOSTIC_QUEUE_SIZE);
    hasEdge = true;
  }
  portEXIT_CRITICAL(&sideButtonDiagnosticMux);
  return hasEdge;
}

uint32_t takeDroppedSideButtonDiagnosticEdges() {
  portENTER_CRITICAL(&sideButtonDiagnosticMux);
  const uint32_t dropped = sideButtonDiagnosticDroppedEdges;
  sideButtonDiagnosticDroppedEdges = 0;
  portEXIT_CRITICAL(&sideButtonDiagnosticMux);
  return dropped;
}

const char* sideButtonDiagnosticName(const int8_t pin) { return pin == BoardConfig::ACTIVE.input.up ? "up" : "down"; }

void beginStickySideButtonDiagnostics() {
  if (!BoardConfig::isSticky()) return;

  const int8_t pins[] = {BoardConfig::ACTIVE.input.up, BoardConfig::ACTIVE.input.down};
  for (const int8_t pin : pins) {
    if (pin >= 0) {
      attachInterruptArg(pin, recordSideButtonDiagnosticEdge, reinterpret_cast<void*>(static_cast<intptr_t>(pin)),
                         CHANGE);
    }
  }
  sideButtonDiagnosticLastUpdateTick = xTaskGetTickCount();
  LOG_INF("BTNDIAG", "Sticky raw side-button diagnostics enabled (up=%d down=%d)", BoardConfig::ACTIVE.input.up,
          BoardConfig::ACTIVE.input.down);
}

void logStickySideButtonDiagnostics(const InputManager& inputManager) {
  if (!BoardConfig::isSticky()) return;

  const TickType_t updateTick = xTaskGetTickCount();
  const unsigned long pollGapMs =
      static_cast<unsigned long>(updateTick - sideButtonDiagnosticLastUpdateTick) * portTICK_PERIOD_MS;
  sideButtonDiagnosticLastUpdateTick = updateTick;

  SideButtonDiagnosticEdge edge{};
  while (popSideButtonDiagnosticEdge(edge)) {
    const unsigned long edgeMs = static_cast<unsigned long>(edge.tick) * portTICK_PERIOD_MS;
    const unsigned long observedAfterMs = static_cast<unsigned long>(updateTick - edge.tick) * portTICK_PERIOD_MS;
    LOG_INF("BTNDIAG", "raw %s %s edge_ms=%lu observed_after_ms=%lu poll_gap_ms=%lu",
            sideButtonDiagnosticName(edge.pin), edge.level == LOW ? "pressed" : "released", edgeMs, observedAfterMs,
            pollGapMs);
  }

  const uint32_t droppedEdges = takeDroppedSideButtonDiagnosticEdges();
  if (droppedEdges > 0) {
    LOG_ERR("BTNDIAG", "raw edge buffer overflow: dropped=%lu", static_cast<unsigned long>(droppedEdges));
  }

  const uint8_t buttons[] = {InputManager::BTN_UP, InputManager::BTN_DOWN};
  const char* const names[] = {"up", "down"};
  for (size_t i = 0; i < 2; ++i) {
    if (inputManager.wasPressed(buttons[i])) {
      LOG_INF("BTNDIAG", "manager %s pressed poll_gap_ms=%lu held_ms=%lu", names[i], pollGapMs,
              inputManager.getHeldTime());
    }
    if (inputManager.wasReleased(buttons[i])) {
      LOG_INF("BTNDIAG", "manager %s released poll_gap_ms=%lu held_ms=%lu", names[i], pollGapMs,
              inputManager.getHeldTime());
    }
  }
}
#endif

// The X3-vs-X4 fingerprint (freeink::detectXteinkVerdict) only makes sense on
// Xteink hardware; other boards keep their compile-time BoardConfig profile.
#if FREEINK_DEVICE_X4 || FREEINK_DEVICE_X3
constexpr char HW_NAMESPACE[] = "cphw";
constexpr char NVS_KEY_DEV_OVERRIDE[] = "dev_ovr";  // 0=auto, 1=x4, 2=x3
constexpr char NVS_KEY_DEV_CACHED[] = "dev_det";    // 0=unknown, 1=x4, 2=x3

enum class NvsDeviceValue : uint8_t { Unknown = 0, X4 = 1, X3 = 2 };

NvsDeviceValue readNvsDeviceValue(const char* key, NvsDeviceValue defaultValue) {
  Preferences prefs;
  if (!prefs.begin(HW_NAMESPACE, true)) {
    return defaultValue;
  }
  const uint8_t raw = prefs.getUChar(key, static_cast<uint8_t>(defaultValue));
  prefs.end();
  if (raw > static_cast<uint8_t>(NvsDeviceValue::X3)) {
    return defaultValue;
  }
  return static_cast<NvsDeviceValue>(raw);
}

void writeNvsDeviceValue(const char* key, NvsDeviceValue value) {
  Preferences prefs;
  if (!prefs.begin(HW_NAMESPACE, false)) {
    return;
  }
  prefs.putUChar(key, static_cast<uint8_t>(value));
  prefs.end();
}

HalGPIO::DeviceType nvsToDeviceType(NvsDeviceValue value) {
  return value == NvsDeviceValue::X3 ? HalGPIO::DeviceType::X3 : HalGPIO::DeviceType::X4;
}

HalGPIO::DeviceType detectDeviceTypeWithFingerprint() {
  // Explicit override for recovery/support:
  // 0 = auto, 1 = force X4, 2 = force X3
  const NvsDeviceValue overrideValue = readNvsDeviceValue(NVS_KEY_DEV_OVERRIDE, NvsDeviceValue::Unknown);
  if (overrideValue == NvsDeviceValue::X3 || overrideValue == NvsDeviceValue::X4) {
    LOG_INF("HW", "Device override active: %s", overrideValue == NvsDeviceValue::X3 ? "X3" : "X4");
    return nvsToDeviceType(overrideValue);
  }

  const NvsDeviceValue cachedValue = readNvsDeviceValue(NVS_KEY_DEV_CACHED, NvsDeviceValue::Unknown);
  if (cachedValue == NvsDeviceValue::X3 || cachedValue == NvsDeviceValue::X4) {
    LOG_INF("HW", "Using cached device type: %s", cachedValue == NvsDeviceValue::X3 ? "X3" : "X4");
    return nvsToDeviceType(cachedValue);
  }

  // No cache yet: run the SDK's X3 fingerprint probe and persist the result.
  uint8_t score1 = 0;
  uint8_t score2 = 0;
  const freeink::XteinkVerdict verdict = freeink::detectXteinkVerdict(&score1, &score2);
  LOG_INF("HW", "X3 probe scores: pass1=%u pass2=%u", score1, score2);

  switch (verdict) {
    case freeink::XteinkVerdict::X3Confirmed:
      writeNvsDeviceValue(NVS_KEY_DEV_CACHED, NvsDeviceValue::X3);
      return HalGPIO::DeviceType::X3;
    case freeink::XteinkVerdict::X4Confirmed:
      writeNvsDeviceValue(NVS_KEY_DEV_CACHED, NvsDeviceValue::X4);
      return HalGPIO::DeviceType::X4;
    case freeink::XteinkVerdict::Inconclusive:
      break;
  }

  // Conservative fallback for first boot with inconclusive probes; not cached,
  // so the next boot re-probes.
  return HalGPIO::DeviceType::X4;
}

#endif  // FREEINK_DEVICE_X4 || FREEINK_DEVICE_X3

}  // namespace

void HalGPIO::begin() {
#if FREEINK_DEVICE_X4 || FREEINK_DEVICE_X3
  SPI.begin(EPD_SCLK, SPI_MISO, EPD_MOSI, EPD_CS);

#ifdef FORCE_DEVICE_X3
  _deviceType = DeviceType::X3;
  LOG_INF("HW", "Device override active via build flag: X3");
#else
  _deviceType = detectDeviceTypeWithFingerprint();
#endif
  BoardConfig::selectDevice(deviceIsX3() ? BoardConfig::Board::XteinkX3 : BoardConfig::Board::XteinkX4);

  if (deviceIsX4()) {
    pinMode(BAT_GPIO0, INPUT);
    pinMode(UART0_RXD, INPUT);
  }
#endif
  inputMgr.begin();
#ifdef STICKY_SIDE_BUTTON_DIAGNOSTICS
  beginStickySideButtonDiagnostics();
#endif
}

void HalGPIO::update() {
  inputMgr.update();
#ifdef STICKY_SIDE_BUTTON_DIAGNOSTICS
  logStickySideButtonDiagnostics(inputMgr);
#endif
  const bool connected = isUsbConnected();
  usbStateChanged = (connected != lastUsbConnected);
  lastUsbConnected = connected;
}

bool HalGPIO::wasUsbStateChanged() const { return usbStateChanged; }

bool HalGPIO::isPressed(uint8_t buttonIndex) const { return inputMgr.isPressed(buttonIndex); }

bool HalGPIO::wasPressed(uint8_t buttonIndex) const { return inputMgr.wasPressed(buttonIndex); }

bool HalGPIO::wasAnyPressed() const { return inputMgr.wasAnyPressed(); }

bool HalGPIO::wasReleased(uint8_t buttonIndex) const { return inputMgr.wasReleased(buttonIndex); }

bool HalGPIO::wasAnyReleased() const { return inputMgr.wasAnyReleased(); }

unsigned long HalGPIO::getHeldTime() const { return inputMgr.getHeldTime(); }

unsigned long HalGPIO::getPowerButtonHeldTime() const { return inputMgr.getPowerButtonHeldTime(); }

#if CROSSINK_APP_CAP_TOUCH
bool HalGPIO::hasTouch() const { return inputMgr.hasTouch(); }

bool HalGPIO::wasTouchTap(float& nx, float& ny) const { return inputMgr.wasTouchTap(nx, ny); }

bool HalGPIO::wasTouchDown(float& nx, float& ny) const { return inputMgr.wasTouchPressedAt(nx, ny); }

bool HalGPIO::wasTouchReleased() const { return inputMgr.wasTouchReleased(); }

bool HalGPIO::isTouchTapCandidate(float& nx, float& ny, unsigned long& heldMs) const {
  return inputMgr.isTouchTapCandidate(nx, ny, heldMs);
}

bool HalGPIO::isTouchHeldAt(float& nx, float& ny) const { return inputMgr.isTouchHeldAt(nx, ny); }

unsigned long HalGPIO::lastTouchHeldMs() const { return inputMgr.lastTouchHeldMs(); }

bool HalGPIO::wasSwipe(float& nxStart, float& nyStart, float& nxEnd, float& nyEnd) const {
  return inputMgr.wasSwipe(nxStart, nyStart, nxEnd, nyEnd);
}

bool HalGPIO::wasTouchActivity() const { return inputMgr.wasTouchActivity(); }
#endif

void HalGPIO::setSharedConfirmPowerShortPressEmitsPower(const bool enabled) {
  InputManager::setSharedConfirmPowerShortPressEmitsPower(enabled);
}

bool HalGPIO::isXteinkDevice() const {
  return BoardConfig::ACTIVE.board == BoardConfig::Board::XteinkX3 ||
         BoardConfig::ACTIVE.board == BoardConfig::Board::XteinkX4;
}

bool HalGPIO::verifyPowerButtonWakeup(uint16_t requiredDurationMs, bool shortPressAllowed) {
  // Boards without a power button (or M5Paper's latch circuit) cannot verify a
  // hold; treat the wake as valid.
  if (BoardConfig::ACTIVE.input.power < 0) {
    return true;
  }
#if defined(FREEINK_DEVICE_M5PAPER) && FREEINK_DEVICE_M5PAPER
  return true;
#endif
  if (shortPressAllowed) {
    // Fast path - no duration check needed
    return true;
  }
  // TODO: Intermittent edge case remains: a single tap followed by another single tap
  // can still power on the device. Tighten wake debounce/state handling here.

  // Calibrate: subtract boot time already elapsed, assuming button held since boot.
  const unsigned long calibration = millis();
  const unsigned long calibratedDuration = (calibration < requiredDurationMs) ? (requiredDurationMs - calibration) : 1;

  const auto start = millis();
  inputMgr.update();
  // inputMgr.isPressed() may take up to ~500ms to return correct state
  while (!inputMgr.isPressed(BTN_POWER) && millis() - start < 1000) {
    delay(10);
    inputMgr.update();
  }
  if (inputMgr.isPressed(BTN_POWER)) {
    do {
      delay(10);
      inputMgr.update();
    } while (inputMgr.isPressed(BTN_POWER) && inputMgr.getPowerButtonHeldTime() < calibratedDuration);
    if (inputMgr.getPowerButtonHeldTime() < calibratedDuration) {
      return false;
    }
  } else {
    return false;
  }
  return true;
}

bool HalGPIO::isUsbConnected() const {
#if FREEINK_DEVICE_X3
  if (deviceIsX3()) {
    // X3 has no USB-detect pin; infer external power from the gauge's charge
    // current via the SDK's BatteryMonitor (BQ27220 Current() > 0 = charging).
    static const BatteryMonitor battery;
    return battery.isCharging();
  }
#endif
  if (BoardConfig::ACTIVE.usbDetect < 0) {
    return false;
  }
  return digitalRead(BoardConfig::ACTIVE.usbDetect) == HIGH;
}

HalGPIO::WakeupReason HalGPIO::getWakeupReason() const {
  const auto wakeupCause = esp_sleep_get_wakeup_cause();
  const auto resetReason = esp_reset_reason();

  const bool usbConnected = isUsbConnected();

  if (wakeupCause == ESP_SLEEP_WAKEUP_GPIO && resetReason == ESP_RST_DEEPSLEEP) {
    return WakeupReason::PowerButton;
  }
  if (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_POWERON && !usbConnected) {
    return WakeupReason::PowerButton;
  }
  if (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_UNKNOWN && usbConnected) {
    return WakeupReason::AfterFlash;
  }
  if (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_POWERON && usbConnected) {
    return WakeupReason::AfterUSBPower;
  }
  return WakeupReason::Other;
}
