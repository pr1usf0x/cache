#include <gtest/gtest.h>

#include <cache_algorithm/lru_cache.hpp>
#include <cache_algorithm/two_queues.hpp>

namespace {
int LoadPage(const int& key) {
  return key * 10;
}
} // namespace

// LRU cache tests
// Evicts the least recently used page when the cache is full.
TEST(LruCacheTest, ConstructorInitializesCountersToZero) {
  LruCache<int, int> cache(2);

  EXPECT_EQ(cache.GetCacheMissCount(), 0);
  EXPECT_EQ(cache.GetAccessCount(), 0);
}

TEST(LruCacheTest, FirstLookupLoadsPage) {
  LruCache<int, int> cache(2);

  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 1);
}

TEST(LruCacheTest, RepeatedLookupUsesCachedPage) {
  LruCache<int, int> cache(2);

  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.LookUpUpdate(7, [](const int&) { return 700; }), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 2);
}

TEST(LruCacheTest, InstancesHaveIndependentState) {
  LruCache<int, int> first(2);
  LruCache<int, int> second(2);

  EXPECT_EQ(first.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(second.GetCacheMissCount(), 0);
  EXPECT_EQ(second.GetAccessCount(), 0);
}

TEST(LruCacheTest, HoldsTwoPages) {
  LruCache<int, int> cache(2);

  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);

  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.GetAccessCount(), 5);
}

// LRU cache: a requested capacity of zero is increased to one.
TEST(LruCacheTest, ZeroCapacityIsClampedToOne) {
  LruCache<int, int> cache(0);

  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);

  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.GetAccessCount(), 4);
}

TEST(LruCacheTest, HitProtectsLeastRecentPageFromEviction) {
  LruCache<int, int> cache(2);
  cache.LookUpUpdate(1, LoadPage);
  cache.LookUpUpdate(2, LoadPage);
  cache.LookUpUpdate(1, LoadPage);
  cache.LookUpUpdate(3, LoadPage);

  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.GetAccessCount(), 7);
}

TEST(LruCacheTest, EvictedPageReloadsFreshValue) {
  LruCache<int, int> cache(1);
  int version = 0;
  auto loader = [&](const int&) { return ++version; };

  EXPECT_EQ(cache.LookUpUpdate(10, loader), 1);
  EXPECT_EQ(cache.LookUpUpdate(20, loader), 2);
  EXPECT_EQ(cache.LookUpUpdate(10, loader), 3);
  EXPECT_EQ(cache.LookUpUpdate(10, loader), 3);
  EXPECT_EQ(version, 3);
  EXPECT_EQ(cache.GetAccessCount(), 4);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
}

// TwoQueues cache tests
// Stores new pages in IN and reused pages from OUT in LRU.
TEST(TwoQueuesTest, ConstructorInitializesCountersToZero) {
  TwoQueues<int, int> cache(20);

  EXPECT_EQ(cache.GetCacheMissCount(), 0);
  EXPECT_EQ(cache.GetAccessCount(), 0);
}

TEST(TwoQueuesTest, FirstLookupLoadsPage) {
  TwoQueues<int, int> cache(20);

  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 1);
}

TEST(TwoQueuesTest, RepeatedLookupUsesCachedPage) {
  TwoQueues<int, int> cache(20);

  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.LookUpUpdate(7, [](const int&) { return 700; }), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 2);
}

TEST(TwoQueuesTest, InstancesHaveIndependentState) {
  TwoQueues<int, int> first(20);
  TwoQueues<int, int> second(20);

  EXPECT_EQ(first.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(second.GetCacheMissCount(), 0);
  EXPECT_EQ(second.GetAccessCount(), 0);
}

TEST(TwoQueuesTest, InQueueHoldsTwoPages) {
  TwoQueues<int, int> cache(20);

  // IN uses 10% of the capacity, so a cache of size 20 holds 2 pages in IN.
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);

  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.GetAccessCount(), 5);
}

TEST(TwoQueuesTest, InHitsDoNotChangeFifoEvictionOrder) {
  TwoQueues<int, int> cache(20);  // IN holds two pages.
  cache.LookUpUpdate(1, LoadPage);
  cache.LookUpUpdate(2, LoadPage);
  cache.LookUpUpdate(1, LoadPage);
  cache.LookUpUpdate(3, LoadPage);

  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.LookUpUpdate(1, [](const int&) { return 100; }), 100);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.GetAccessCount(), 6);
}

TEST(TwoQueuesTest, OutHitReloadsAndPromotesPage) {
  TwoQueues<int, int> cache(10);  // IN holds one page.
  cache.LookUpUpdate(1, LoadPage);
  cache.LookUpUpdate(2, LoadPage);
  EXPECT_EQ(cache.LookUpUpdate(1, [](const int&) { return 101; }), 101);

  for (int key : {3, 4}) {
    cache.LookUpUpdate(key, LoadPage);
  }
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 101);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  EXPECT_EQ(cache.GetAccessCount(), 6);
}

TEST(TwoQueuesTest, ForgottenOutPageReturnsToInInsteadOfLru) {
  TwoQueues<int, int> cache(10);  // OUT holds three keys.
  for (int key = 1; key <= 5; ++key) {
    cache.LookUpUpdate(key, LoadPage);
  }
  // Key 1 has left OUT, so it must enter IN on the next access.
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  cache.LookUpUpdate(6, LoadPage);
  EXPECT_EQ(cache.LookUpUpdate(1, [](const int&) { return 101; }), 101);
  EXPECT_EQ(cache.GetCacheMissCount(), 8);
  EXPECT_EQ(cache.GetAccessCount(), 8);
}

TEST(TwoQueuesTest, LruHitChangesVictimWhenPromotionFillsLru) {
  TwoQueues<int, int> cache(5);  // IN = 1, OUT = 1, LRU = 3.
  for (int key : {1, 2, 1, 3, 2, 4, 3}) {
    cache.LookUpUpdate(key, LoadPage);
  }
  // LRU contains 1, 2, 3. Touching 1 makes 2 the next victim.
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  cache.LookUpUpdate(5, LoadPage);
  cache.LookUpUpdate(4, LoadPage);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.LookUpUpdate(4, LoadPage), 40);
  EXPECT_EQ(cache.LookUpUpdate(2, [](const int&) { return 202; }), 202);
  EXPECT_EQ(cache.GetCacheMissCount(), 10);
  EXPECT_EQ(cache.GetAccessCount(), 14);
}

TEST(TwoQueuesTest, SmallCapacitiesKeepEachQueueUsable) {
  for (size_t capacity : {0, 1, 2}) {
    SCOPED_TRACE(capacity);
    TwoQueues<int, int> cache(capacity);
    for (int key : {1, 2, 1, 3, 2}) {
      EXPECT_EQ(cache.LookUpUpdate(key, LoadPage), LoadPage(key));
    }
    EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
    EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
    EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
    EXPECT_EQ(cache.GetCacheMissCount(), 6);
    EXPECT_EQ(cache.GetAccessCount(), 8);
  }
}
