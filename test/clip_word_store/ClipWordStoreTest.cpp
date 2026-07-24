#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include "activities/reader/WordRef.h"

TEST(ClipWordStore, StoresNullTerminatedUtf8TextWithStableOffsets) {
  ClipWordStore store;
  WordRef first;
  WordRef second;

  ASSERT_TRUE(store.appendText(first, "first"));
  ASSERT_TRUE(store.appendText(second, "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D"));  // Hebrew UTF-8

  EXPECT_EQ(first.textOffset, 0);
  EXPECT_EQ(first.textLength, 5);
  EXPECT_STREQ(store.text(first), "first");
  EXPECT_STREQ(store.text(second), "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  EXPECT_TRUE(std::is_trivially_copyable_v<WordRef>);
}

TEST(ClipWordStore, RejectsTextPastTheUint16PoolBoundary) {
  ClipWordStore store;
  WordRef longWord;
  const std::string text(ClipWordStore::MAX_TEXT_POOL_BYTES - 1, 'x');

  ASSERT_TRUE(store.appendText(longWord, text.c_str()));
  EXPECT_EQ(store.textPool.size(), ClipWordStore::MAX_TEXT_POOL_BYTES);

  WordRef overflow;
  EXPECT_FALSE(store.appendText(overflow, "x"));
}
