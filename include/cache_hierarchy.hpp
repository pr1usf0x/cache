#ifndef CACHE_HIERARCHY_HPP_
#define CACHE_HIERARCHY_HPP_

#include <functional>

#include "config.hpp"

namespace cache {

template <typename Key, typename Tp>
class CacheHierarchy {
 private:

  using loader = std::function<Tp(Key)>;
  std::vector<std::unique_ptr<Cache<Key, Tp>>> cache_vector_;
  std::vector<std::function<Tp(Key)>> slow_get_page_vector_;
  loader user_loader_;

 public:
  explicit CacheHierarchy(const Config& config) {


  }

  T LookUpUpdate(const Key& key)

};
}  // namespace cache

#endif  // CACHE_HIERARCHY_HPP_