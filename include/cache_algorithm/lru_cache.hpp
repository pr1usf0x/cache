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
class LruCache final : public Cache<Key, Tp> {
 private:
  using list_iter = typename std::list<std::pair<Key, Tp>>::iterator;

  const size_t cache_cap_;
  std::list<std::pair<Key, Tp>> storage_;
  std::unordered_map<Key, list_iter> hash_map_;

 public:
  explicit LruCache(size_t cap) : cache_cap_(std::max<size_t>(cap, 1)) {};

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
      auto del_list_it = storage_.pop_back();
      hash_map_.erase(del_list_it->first);
    }

    auto list_it_new = storage_.push_front(std::make_pair(key, element_copy));
    hash_map_.insert(key, list_it_new);

    return element_copy;
  }
};

#endif  // LRU_CACHE_HPP
