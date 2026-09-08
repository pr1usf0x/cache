#ifndef LRU_CACHE_HPP_
#define LRU_CACHE_HPP_

#include <functional>
#include <list>
#include <unordered_map>

#include "cache.hpp"

template <typename Key, typename Tp>
class LRUCache final : public Cache<Key, Tp> {
 private:
  std::list<Tp> storage_;
  using iterator_ = std::list<Tp>::iterator;

  std::unordered_map<Key, size_t> hash_map_;

 public:
  const Tp& LookUpUpdate(Key key,
                         std::function<const Key&()> slow_get_page) override {
    auto& el = hash_map_.find(key);
    if (el != hash_map_.end()) {
      return el;
    }
  }
};

#endif  // LRU_CACHE_HPP