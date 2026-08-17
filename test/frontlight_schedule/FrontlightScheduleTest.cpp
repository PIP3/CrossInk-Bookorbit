#include <gtest/gtest.h>

#include "FrontlightSchedule.h"

namespace {
constexpr uint8_t slot(const uint8_t hour, const uint8_t minute = 0) {
  return static_cast<uint8_t>(hour * 4 + minute / 15);
}
}  // namespace

TEST(FrontlightSchedule, DisabledOrIncompleteWindowIsInactive) {
  EXPECT_FALSE(FrontlightSchedule::hasCompleteWindow(false, slot(18), slot(7)));
  EXPECT_FALSE(FrontlightSchedule::hasCompleteWindow(true, FrontlightSchedule::kUnsetTimeSlot, slot(7)));
  EXPECT_FALSE(FrontlightSchedule::hasCompleteWindow(true, slot(18), FrontlightSchedule::kUnsetTimeSlot));
}

TEST(FrontlightSchedule, RestoreOnWakeFallsThroughToScheduleOnlyWhenPreviouslyOff) {
  EXPECT_FALSE(FrontlightSchedule::shouldApplyOnWakeSchedule(false, true, true));
  EXPECT_TRUE(FrontlightSchedule::shouldApplyOnWakeSchedule(false, true, false));
  EXPECT_TRUE(FrontlightSchedule::shouldApplyOnWakeSchedule(false, false, true));
  EXPECT_FALSE(FrontlightSchedule::shouldApplyOnWakeSchedule(true, false, false));
}

TEST(FrontlightSchedule, SameEndpointIsAnEmptyWindow) {
  EXPECT_FALSE(FrontlightSchedule::hasCompleteWindow(true, slot(18), slot(18)));
  EXPECT_FALSE(FrontlightSchedule::containsTimeSlot(slot(18), slot(18), slot(18)));
}

TEST(FrontlightSchedule, NormalWindowIncludesStartAndExcludesEnd) {
  EXPECT_TRUE(FrontlightSchedule::containsTimeSlot(slot(7), slot(22), slot(7)));
  EXPECT_TRUE(FrontlightSchedule::containsTimeSlot(slot(7), slot(22), slot(21, 45)));
  EXPECT_FALSE(FrontlightSchedule::containsTimeSlot(slot(7), slot(22), slot(22)));
}

TEST(FrontlightSchedule, OvernightWindowWrapsMidnight) {
  EXPECT_TRUE(FrontlightSchedule::containsTimeSlot(slot(21), slot(7), slot(23)));
  EXPECT_TRUE(FrontlightSchedule::containsTimeSlot(slot(21), slot(7), slot(6, 45)));
  EXPECT_FALSE(FrontlightSchedule::containsTimeSlot(slot(21), slot(7), slot(12)));
}

TEST(FrontlightSchedule, LocalTimeSlotAppliesQuarterHourOffset) {
  EXPECT_EQ(FrontlightSchedule::localTimeSlot(23, 45, 52), slot(0, 45));  // UTC+1
  EXPECT_EQ(FrontlightSchedule::localTimeSlot(0, 15, 44), slot(23, 15));  // UTC-1
}

TEST(FrontlightSchedule, UnsetTimeStartsAtNoonAndRoundTripsTwelveHourValues) {
  const auto unset = FrontlightSchedule::timeOfDayFromSlot(FrontlightSchedule::kUnsetTimeSlot);
  EXPECT_EQ(unset.hour12, 12);
  EXPECT_EQ(unset.minuteQuarter, 0);
  EXPECT_TRUE(unset.isPm);

  const auto midnight = FrontlightSchedule::timeOfDayFromSlot(0);
  EXPECT_EQ(midnight.hour12, 12);
  EXPECT_FALSE(midnight.isPm);
  EXPECT_EQ(FrontlightSchedule::slotFromTimeOfDay(midnight.hour12, midnight.minuteQuarter, midnight.isPm), 0);

  const auto afternoon = FrontlightSchedule::timeOfDayFromSlot(67);  // 16:45
  EXPECT_EQ(afternoon.hour12, 4);
  EXPECT_EQ(afternoon.minuteQuarter, 3);
  EXPECT_TRUE(afternoon.isPm);
  EXPECT_EQ(FrontlightSchedule::slotFromTimeOfDay(afternoon.hour12, afternoon.minuteQuarter, afternoon.isPm), 67);
}
