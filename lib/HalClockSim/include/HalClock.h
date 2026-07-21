#pragma once

#include <cstddef>
#include <cstdint>

class HalClock;
extern HalClock halClock;

class HalClock {
 public:
  enum DateFormat : uint8_t {
    MONTH_DAY_YEAR_LONG = 0,
    DAY_MONTH_YEAR_LONG = 1,
    MONTH_DAY_YEAR_NUMERIC = 2,
    DAY_MONTH_YEAR_NUMERIC = 3,
    YEAR_MONTH_DAY_ISO = 4,
    YEAR_MONTH_DAY_NUMERIC = 5,
    MONTH_DAY_NUMERIC = 6,
    DAY_MONTH_NUMERIC = 7,
    MONTH_DAY_LONG = 8,
    DAY_MONTH_LONG = 9,
    DATE_FORMAT_COUNT
  };

  void begin() {}
  bool isAvailable() const { return false; }
  bool getTime(uint8_t& hour, uint8_t& minute) const;
  bool getDateTime(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const;
  bool formatTime(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased = 48, bool use12Hour = false) const;
  bool formatDate(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased = 48,
                  DateFormat dateFormat = MONTH_DAY_YEAR_LONG) const;
  bool syncFromNTP() { return false; }
};
