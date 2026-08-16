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
