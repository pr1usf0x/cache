#ifndef CACHE_HPP_
#define CACHE_HPP_

#include <cstdint>
#include <functional>

template <typename Key, typename Tp>
class Cache {
 protected:
  size_t misses_count_{};
  size_t access_count_{};

 public:
  virtual const Tp& LookUpUpdate(Key key,
                                 std::function<const Tp&()> slow_get_page) = 0;

// getters
  size_t GetCacheMissCount() const { return misses_count_; };
  size_t GetAccessCount() const { return access_count_; };

  virtual ~Cache() = default;
};

#endif  // CACHE_HPP_