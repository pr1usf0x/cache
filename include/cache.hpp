#ifndef CACHE_HPP_
#define CACHE_HPP_

#include <cstdint>
#include <functional>

namespace cache {

template <typename Key, typename Tp>
class Cache {
 protected:
  size_t misses_count_{};
  size_t access_count_{};

 public:
  virtual Tp LookUpUpdate(const Key& key,
                  std::function<Tp(const Key& key)> slow_get_page) = 0;

  // getters
  size_t GetCacheMissCount() const { return misses_count_; };
  size_t GetAccessCount() const { return access_count_; };

  virtual ~Cache() = default;
};
} // namespace cache

#endif  // CACHE_HPP_
