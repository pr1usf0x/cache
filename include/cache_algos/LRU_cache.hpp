#ifndef LRU_CACHE_HPP_
#define LRU_CACHE_HPP_

#include <functional>
#include <list>
#include <unordered_map>

#include "cache.hpp"

template <typename T>
class LRUCache final : public Cache<T> {

  std::list<T> storage_;
  std::unordered_map<uint64_t, T> hash_map_;

 public:
  const T& LookUpUpdate(uint64_t key,
                        std::function<const T&()> slow_get_page) override {}
};

#endif  // LRU_CACHE_HPP