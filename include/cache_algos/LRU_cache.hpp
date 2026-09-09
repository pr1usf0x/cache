#ifndef LRU_CACHE_HPP_
#define LRU_CACHE_HPP_

#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include "cache.hpp"

template <typename Key, typename Tp>
class LRUCache final : public Cache<Key, Tp> {
 private:
  using list_iter = typename std::list<std::pair<Key, Tp>>::iterator;

  const size_t cache_cap_;
  std::list<std::pair<Key, Tp>> storage_;
  std::unordered_map<Key, list_iter> hash_map_;

 public:
  explicit LRUCache(size_t cap) : cache_cap_(cap) {};

  Tp LookUpUpdate(Key key, std::function<Tp()> slow_get_page) override {
    assert(storage_.size() <= cache_cap_);

    this->access_count_++;

    auto ht_it = hash_map_.find(key);
    if (ht_it != hash_map_.end()) {
      auto list_it = ht_it->second;
      storage_.splice(storage_.begin(), storage_, list_it);

      return list_it->second;
    }

    this->misses_count_++;

    Tp element_copy = slow_get_page();

    if (storage_.size() == cache_cap_) {
      storage_.pop_back();
      // hash_map_.erase();
    }

    storage_.push_front(std::make_pair(key, element_copy));

    return element_copy;
  }
};

#endif  // LRU_CACHE_HPP