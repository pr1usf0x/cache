#include <gtest/gtest.h>

#include <array>

#include <cache_algorithm/arc.hpp>
#include <cache_algorithm/lru_cache.hpp>
#include <cache_algorithm/two_queues.hpp>

namespace {
int LoadPage(const int& key) {
  return key * 10;
}
} // namespace

// ============================================================================
// === LRU cache ===
// ============================================================================
// Evicts the least recently used page when the cache is full.
TEST(LruCacheTest, ConstructorInitializesCountersToZero) {
  cache::Lru<int, int> cache(2);

  EXPECT_EQ(cache.GetCacheMissCount(), 0);
  EXPECT_EQ(cache.GetAccessCount(), 0);
}

TEST(LruCacheTest, FirstLookupLoadsPage) {
  cache::Lru<int, int> cache(2);

  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 1);
}

TEST(LruCacheTest, RepeatedLookupUsesCachedPage) {
  cache::Lru<int, int> cache(2);

  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.LookUpUpdate(7, [](const int&) { return 700; }), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 2);
}

TEST(LruCacheTest, InstancesHaveIndependentState) {
  cache::Lru<int, int> first(2);
  cache::Lru<int, int> second(2);

  EXPECT_EQ(first.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(second.GetCacheMissCount(), 0);
  EXPECT_EQ(second.GetAccessCount(), 0);
}

TEST(LruCacheTest, HoldsTwoPages) {
  cache::Lru<int, int> cache(2);

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
  cache::Lru<int, int> cache(0);

  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);

  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.GetAccessCount(), 4);
}

TEST(LruCacheTest, HitProtectsLeastRecentPageFromEviction) {
  cache::Lru<int, int> cache(2);
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
  cache::Lru<int, int> cache(1);
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


// === LRU cache: corner case with 50 accesses ===
TEST(LruCacheTest, FiftyAccessesWithChangingWorkingSet) {
  cache::Lru<int, int> cache(4);
  const std::array<int, 50> keys = {
      // Fill the cache, refresh pages and overflow capacity.
      1, 2, 3, 4, 1, 2, 5, 1, 2, 3,
      // Reuse surviving pages, then introduce page 6.
      4, 5, 3, 4, 5, 6, 3, 4, 5, 6,
      // Replace the working set with pages 7 through 10.
      7, 8, 9, 10, 7, 8, 9, 10, 7, 8,
      // Return to the original working set.
      1, 2, 1, 2, 3, 4, 1, 2, 3, 4,
      // Keep page 4 hot while replacing its neighbours.
      4, 4, 5, 4, 6, 4, 7, 4, 5, 6,
  };
  // Cumulative misses after each block of ten accesses.
  const std::array<size_t, 5> expected_misses = {6, 9, 13, 17, 20};
  size_t loader_calls = 0;
  auto loader = [&](const int& key) {
    ++loader_calls;
    return LoadPage(key);
  };

  for (size_t i = 0; i < keys.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "access=" << i + 1
                                  << ", key=" << keys[i]);
    EXPECT_EQ(cache.LookUpUpdate(keys[i], loader), LoadPage(keys[i]));
    EXPECT_EQ(cache.GetAccessCount(), i + 1);
    if ((i + 1) % 10 == 0) {
      EXPECT_EQ(cache.GetCacheMissCount(), expected_misses[i / 10]);
      EXPECT_EQ(loader_calls, expected_misses[i / 10]);
    }
  }
}

// ============================================================================
// === TwoQueues cache ===
// ============================================================================
// Stores new pages in IN and reused pages from OUT in LRU.
TEST(TwoQueuesTest, ConstructorInitializesCountersToZero) {
  cache::TwoQueues<int, int> cache(20);

  EXPECT_EQ(cache.GetCacheMissCount(), 0);
  EXPECT_EQ(cache.GetAccessCount(), 0);
}

TEST(TwoQueuesTest, FirstLookupLoadsPage) {
  cache::TwoQueues<int, int> cache(20);

  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 1);
}

TEST(TwoQueuesTest, RepeatedLookupUsesCachedPage) {
  cache::TwoQueues<int, int> cache(20);

  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.LookUpUpdate(7, [](const int&) { return 700; }), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 2);
}

TEST(TwoQueuesTest, InstancesHaveIndependentState) {
  cache::TwoQueues<int, int> first(20);
  cache::TwoQueues<int, int> second(20);

  EXPECT_EQ(first.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(second.GetCacheMissCount(), 0);
  EXPECT_EQ(second.GetAccessCount(), 0);
}

TEST(TwoQueuesTest, InQueueHoldsTwoPages) {
  cache::TwoQueues<int, int> cache(20);

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
  cache::TwoQueues<int, int> cache(20);  // IN holds two pages.
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
  cache::TwoQueues<int, int> cache(10);  // IN holds one page.
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
  cache::TwoQueues<int, int> cache(10);  // OUT holds three keys.
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
  cache::TwoQueues<int, int> cache(5);  // IN = 1, OUT = 1, LRU = 3.
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
    cache::TwoQueues<int, int> cache(capacity);
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


// === TwoQueues cache: corner case with 50 accesses ===
TEST(TwoQueuesTest, FiftyAccessesWithChangingWorkingSet) {
  cache::TwoQueues<int, int> cache(5);
  const std::array<int, 50> keys = {
      // Fill LRU through OUT hits; IN and OUT each hold one page.
      1, 2, 1, 3, 2, 4, 3, 1, 2, 3,
      // Scan new pages, reuse LRU, then revisit a forgotten OUT key.
      5, 6, 7, 8, 9, 10, 1, 2, 3, 9,
      // Promote pages from OUT until the old LRU set is replaced.
      1, 2, 3, 10, 11, 10, 12, 11, 13, 12,
      // Overflow OUT while reusing the new LRU set.
      10, 11, 12, 14, 15, 16, 10, 11, 12, 15,
      // Alternate new pages, ghost promotions and LRU hits.
      17, 16, 18, 17, 19, 18, 16, 17, 18, 19,
  };
  // Cumulative misses after each block of ten accesses.
  const std::array<size_t, 5> expected_misses = {7, 14, 21, 25, 31};
  size_t loader_calls = 0;
  auto loader = [&](const int& key) {
    ++loader_calls;
    return LoadPage(key);
  };

  for (size_t i = 0; i < keys.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "access=" << i + 1
                                  << ", key=" << keys[i]);
    EXPECT_EQ(cache.LookUpUpdate(keys[i], loader), LoadPage(keys[i]));
    EXPECT_EQ(cache.GetAccessCount(), i + 1);
    if ((i + 1) % 10 == 0) {
      EXPECT_EQ(cache.GetCacheMissCount(), expected_misses[i / 10]);
      EXPECT_EQ(loader_calls, expected_misses[i / 10]);
    }
  }
}

// ============================================================================
// === ARC cache ===
// ============================================================================
// T1/T2 store pages; B1/B2 keep only the keys of evicted pages.
// === ARC cache: basic operations ===
TEST(ArcCacheTest, ConstructorInitializesCountersToZero) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.GetCacheMissCount(), 0);
  EXPECT_EQ(cache.GetAccessCount(), 0);
}

TEST(ArcCacheTest, FirstLookupLoadsPage) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 1);
}

TEST(ArcCacheTest, RepeatedLookupUsesCachedPageInBothResidentQueues) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  // The first hit moves the page from T1 to T2; the next hit stays in T2.
  auto unexpected_loader = [](const int&) {
    ADD_FAILURE() << "A resident page must not be loaded again";
    return 700;
  };
  EXPECT_EQ(cache.LookUpUpdate(7, unexpected_loader), 70);
  EXPECT_EQ(cache.LookUpUpdate(7, unexpected_loader), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 3);
}

TEST(ArcCacheTest, InstancesHaveIndependentState) {
  cache::Arc<int, int> cache(2);
  cache::Arc<int, int> second(2);
  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(second.GetCacheMissCount(), 0);
  EXPECT_EQ(second.GetAccessCount(), 0);
  EXPECT_EQ(second.LookUpUpdate(7, [](const int&) { return 700; }), 700);
  EXPECT_EQ(cache.LookUpUpdate(7, LoadPage), 70);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.GetAccessCount(), 2);
  EXPECT_EQ(second.GetCacheMissCount(), 1);
  EXPECT_EQ(second.GetAccessCount(), 1);
}

// === ARC cache: resident pages and eviction order ===
TEST(ArcCacheTest, HoldsTwoPages) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.GetAccessCount(), 4);
}

TEST(ArcCacheTest, FullT1EvictsOldestPage) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.GetAccessCount(), 6);
}

TEST(ArcCacheTest, T1HitProtectsPageFromEviction) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);  // Promote 1 to T2.
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);   // Evict 2 from T1 to B1.
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.GetAccessCount(), 7);
}

TEST(ArcCacheTest, T2HitChangesLeastRecentlyUsedVictim) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);  // T2 order is now 1, 2.
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);   // Evict 2 to B2.
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.GetAccessCount(), 9);
}

// === ARC cache: ghost hits and adaptation ===
TEST(ArcCacheTest, B1HitGivesRecentPagesMoreSpace) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  // T1 = [3], T2 = [1], B1 = [2].
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  // Increase p to 1 and evict 1 from T2.
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  EXPECT_EQ(cache.GetAccessCount(), 8);
}

TEST(ArcCacheTest, B2HitGivesFrequentPagesMoreSpace) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  // p = 1, T1 = [3], T2 = [2], B2 = [1].
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  // Decrease p to 0 and evict 3 from T1.
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 6);
  EXPECT_EQ(cache.GetAccessCount(), 9);
}

TEST(ArcCacheTest, GhostHitsReloadFreshValuesAndThenCacheThem) {
  cache::Arc<int, int> cache(2);
  int version = 0;
  auto loader = [&](const int&) { return ++version; };
  EXPECT_EQ(cache.LookUpUpdate(1, loader), 1);
  EXPECT_EQ(cache.LookUpUpdate(1, loader), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, loader), 2);
  EXPECT_EQ(cache.LookUpUpdate(3, loader), 3);
  EXPECT_EQ(cache.LookUpUpdate(2, loader), 4);  // B1 hit.
  EXPECT_EQ(cache.LookUpUpdate(2, loader), 4);
  EXPECT_EQ(cache.LookUpUpdate(1, loader), 5);  // B2 hit.
  EXPECT_EQ(cache.LookUpUpdate(1, loader), 5);
  EXPECT_EQ(version, 5);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  EXPECT_EQ(cache.GetAccessCount(), 8);
}

TEST(ArcCacheTest, B2HitAtZeroTargetDoesNotUnderflow) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  // p is still 0; 1 is now in B2.
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  // Keep p at 0 and evict 3, preserving 2.
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  EXPECT_EQ(cache.GetAccessCount(), 9);
}

// === ARC cache: scans and bounded ghost history ===
TEST(ArcCacheTest, FrequentPageSurvivesLongScan) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  for (int key = 2; key <= 100; ++key) {
    EXPECT_EQ(cache.LookUpUpdate(key, LoadPage), LoadPage(key));
    EXPECT_EQ(cache.GetCacheMissCount(), static_cast<size_t>(key));
  }
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 100);
  EXPECT_EQ(cache.LookUpUpdate(100, LoadPage), 1000);
  EXPECT_EQ(cache.GetCacheMissCount(), 100);
  EXPECT_EQ(cache.GetAccessCount(), 103);
}

TEST(ArcCacheTest, ForgottenB1PageReturnsToT1) {
  cache::Arc<int, int> cache(2);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 1);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 2);
  EXPECT_EQ(cache.LookUpUpdate(3, LoadPage), 30);  // B1 contains 2.
  EXPECT_EQ(cache.GetCacheMissCount(), 3);
  // Forget 2; B1 now contains 3.
  EXPECT_EQ(cache.LookUpUpdate(4, LoadPage), 40);
  EXPECT_EQ(cache.GetCacheMissCount(), 4);
  // A new page, so insert into T1.
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  // Evict 2, preserving the frequent page 1.
  EXPECT_EQ(cache.LookUpUpdate(5, LoadPage), 50);
  EXPECT_EQ(cache.GetCacheMissCount(), 6);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 6);
  EXPECT_EQ(cache.LookUpUpdate(2, LoadPage), 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 7);
  EXPECT_EQ(cache.GetAccessCount(), 9);
}

TEST(ArcCacheTest, FullHistoryForgetsOldestB2Page) {
  cache::Arc<int, int> cache(2);
  for (int key = 1; key <= 4; ++key) {
    EXPECT_EQ(cache.LookUpUpdate(key, LoadPage), LoadPage(key));
    EXPECT_EQ(cache.GetCacheMissCount(), static_cast<size_t>(key));
    EXPECT_EQ(cache.LookUpUpdate(key, LoadPage), LoadPage(key));
    EXPECT_EQ(cache.GetCacheMissCount(), static_cast<size_t>(key));
  }
  // T2 = [4, 3], B2 = [2, 1]: the directory has reached 2 * capacity.
  // Forget 1 from B2 before replacing a page.
  EXPECT_EQ(cache.LookUpUpdate(5, LoadPage), 50);
  EXPECT_EQ(cache.GetCacheMissCount(), 5);
  // Forgotten 1 enters T1, not T2.
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 6);
  EXPECT_EQ(cache.LookUpUpdate(6, LoadPage), 60);  // Evict 1 from T1.
  EXPECT_EQ(cache.GetCacheMissCount(), 7);
  EXPECT_EQ(cache.LookUpUpdate(4, LoadPage), 40);
  EXPECT_EQ(cache.GetCacheMissCount(), 7);
  EXPECT_EQ(cache.LookUpUpdate(1, LoadPage), 10);
  EXPECT_EQ(cache.GetCacheMissCount(), 8);
  EXPECT_EQ(cache.GetAccessCount(), 13);
}

// === ARC cache: capacity boundaries ===
TEST(ArcCacheTest, WorkingSetFitsWithoutFurtherLoads) {
  for (size_t capacity : {1, 2, 3, 16}) {
    SCOPED_TRACE(capacity);
    cache::Arc<int, int> cache(capacity);
    size_t loader_calls = 0;
    auto loader = [&](const int& key) {
      ++loader_calls;
      return LoadPage(key);
    };
    for (int round = 0; round < 10; ++round) {
      for (size_t key = 0; key < capacity; ++key) {
        EXPECT_EQ(cache.LookUpUpdate(static_cast<int>(key), loader),
                  LoadPage(static_cast<int>(key)));
      }
    }
    EXPECT_EQ(loader_calls, capacity);
    EXPECT_EQ(cache.GetCacheMissCount(), capacity);
    EXPECT_EQ(cache.GetAccessCount(), 10 * capacity);
  }
}

TEST(ArcCacheTest, ScanLargerThanCapacityMissesOnEveryAccess) {
  for (size_t capacity : {1, 2, 3, 16}) {
    SCOPED_TRACE(capacity);
    cache::Arc<int, int> cache(capacity);
    size_t loader_calls = 0;
    auto loader = [&](const int& key) {
      ++loader_calls;
      return LoadPage(key);
    };
    for (int round = 0; round < 10; ++round) {
      for (size_t key = 0; key <= capacity; ++key) {
        EXPECT_EQ(cache.LookUpUpdate(static_cast<int>(key), loader),
                  LoadPage(static_cast<int>(key)));
      }
    }
    EXPECT_EQ(loader_calls, 10 * (capacity + 1));
    EXPECT_EQ(cache.GetCacheMissCount(), loader_calls);
    EXPECT_EQ(cache.GetAccessCount(), loader_calls);
  }
}

TEST(ArcCacheTest, AlternatingPromotedPagesReloadsGhosts) {
  cache::Arc<int, int> cache(1);
  int version = 0;
  auto loader = [&](const int&) { return ++version; };
  for (int round = 0; round < 20; ++round) {
    const int key = round % 2;
    EXPECT_EQ(cache.LookUpUpdate(key, loader), round + 1);
    EXPECT_EQ(cache.LookUpUpdate(key, loader), round + 1);
  }
  EXPECT_EQ(version, 20);
  EXPECT_EQ(cache.GetCacheMissCount(), 20);
  EXPECT_EQ(cache.GetAccessCount(), 40);
}

// === ARC cache: corner case with 50 accesses ===
TEST(ArcCacheTest, FiftyAccessesWithChangingWorkingSet) {
  cache::Arc<int, int> cache(4);
  const std::array<int, 50> keys = {
      // Promote repeated pages into T2 and start filling B2.
      1, 1, 2, 2, 3, 3, 4, 4, 5, 5,
      // Create unequal ghost sizes and revisit both B1 and B2.
      6, 6, 7, 8, 7, 9, 10, 8, 7, 6,
      // Scan new pages, then switch to a small reused set.
      11, 12, 13, 14, 7, 8, 7, 8, 15, 16,
      // Return to old pages and rebuild a four-page working set.
      1, 2, 3, 4, 1, 2, 3, 4, 1, 2,
      // Switch again, mixing new pages and immediate reuse.
      5, 6, 5, 6, 7, 8, 5, 6, 7, 8,
  };
  // Cumulative misses after each block of ten accesses.
  const std::array<size_t, 5> expected_misses = {5, 12, 20, 27, 31};
  size_t loader_calls = 0;
  auto loader = [&](const int& key) {
    ++loader_calls;
    return LoadPage(key);
  };

  for (size_t i = 0; i < keys.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "access=" << i + 1
                                  << ", key=" << keys[i]);
    EXPECT_EQ(cache.LookUpUpdate(keys[i], loader), LoadPage(keys[i]));
    EXPECT_EQ(cache.GetAccessCount(), i + 1);
    if ((i + 1) % 10 == 0) {
      EXPECT_EQ(cache.GetCacheMissCount(), expected_misses[i / 10]);
      EXPECT_EQ(loader_calls, expected_misses[i / 10]);
    }
  }
}
