#pragma once

#include <cstdint>

// Daily frontlight schedules are stored as quarter-hour slots. Keeping the
// policy free of hardware dependencies lets boot code and host tests use the
// same boundary rules.
namespace FrontlightSchedule {
constexpr uint8_t kSlotsPerDay = 24 * 4;
constexpr uint8_t kUnsetTimeSlot = 0xFF;

constexpr bool isTimeSlotValid(const uint8_t slot) { return slot < kSlotsPerDay; }

constexpr bool hasCompleteWindow(const bool enabled, const uint8_t startSlot, const uint8_t endSlot) {
  return enabled && isTimeSlotValid(startSlot) && isTimeSlotValid(endSlot) && startSlot != endSlot;
}

constexpr bool containsTimeSlot(const uint8_t startSlot, const uint8_t endSlot, const uint8_t currentSlot) {
  if (!isTimeSlotValid(startSlot) || !isTimeSlotValid(endSlot) || !isTimeSlotValid(currentSlot) ||
      startSlot == endSlot) {
    return false;
  }
  return startSlot < endSlot ? currentSlot >= startSlot && currentSlot < endSlot
                             : currentSlot >= startSlot || currentSlot < endSlot;
}

constexpr uint8_t localTimeSlot(const uint8_t utcHour, const uint8_t utcMinute,
                                const uint8_t utcOffsetQuarterHoursBiased) {
  const int offsetQuarterHours =
      static_cast<int>(utcOffsetQuarterHoursBiased > 104 ? 104 : utcOffsetQuarterHoursBiased) - 48;
  const int utcMinutes = static_cast<int>(utcHour) * 60 + static_cast<int>(utcMinute);
  const int localMinutes = ((utcMinutes + offsetQuarterHours * 15) % 1440 + 1440) % 1440;
  return static_cast<uint8_t>(localMinutes / 15);
}
}  // namespace FrontlightSchedule
