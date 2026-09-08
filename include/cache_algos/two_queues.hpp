#ifndef CACHING_ALGORITHMS_HPP_
#define CACHING_ALGORITHMS_HPP_

#include <algorithm>
#include <functional>
#include <list>
#include <unordered_map>

#include "cache.hpp"

template <typename Key, typename Tp>
class TwoQueues : public Cache<Key, Tp> {
 public:
  explicit TwoQueues(size_t cache_cap_)
      : cache_cap_(cache_cap_),
        in_cap_(std::max<size_t>(1, cache_cap_ * 0.1)),
        out_cap_(std::max<size_t>(1, cache_cap_ * 0.3)),
        lru_cap_(std::max<size_t>(1, cache_cap_ * 0.6)) {}

  Tp LookupUpdate(const Key& key,
                  std::function<Tp(const Key& key)> slow_get_page) override {
    auto hash_it = data_base_.find(key);
    if (hash_it == data_base_.end()) {
      return AbsoluteMiss(key, slow_get_page);
    }

    switch (hash_it->second.cache_type) {
      case CacheType::kLru:
        return LruHit(hash_it);
      case CacheType::kOut:
        return OutHit(key, hash_it, slow_get_page);
      case CacheType::kIn:
        return (hash_it->second).list_it->val;
      default:
        break;
    }

    return {};
  }

 private:
  enum class CacheType { kIn, kOut, kLru };

  struct CacheNode {
    Key key;
    Tp val;
  };

  struct ElInfo {
    CacheType cache_type;
    typename std::list<CacheNode>::iterator list_it;
    typename std::list<Key>::iterator key_it;
  };

  size_t cache_cap_;
  size_t in_cap_;
  size_t out_cap_;
  size_t lru_cap_;

  std::list<CacheNode> in_cache_;
  std::list<Key> out_cache_;
  std::list<CacheNode> lru_cache_;
  std::unordered_map<Key, ElInfo> data_base_;

  Tp AbsoluteMiss(const Key& key,
                  std::function<Tp(const Key& key)> slow_get_page) {
    Tp old_elem = slow_get_page(key);

    if (in_cache_.size() >= in_cap_) {
      auto node = in_cache_.back();
      in_cache_.pop_back();

      if (out_cache_.size() >= out_cap_) {
        Key out_key = out_cache_.back();
        out_cache_.pop_back();
        data_base_.erase(out_key);
      }

      auto pos = out_cache_.emplace(out_cache_.begin(), node.key);
      data_base_[node.key] = {CacheType::kOut, {}, pos};
    }

    auto pos = in_cache_.emplace(in_cache_.begin(), {key, old_elem});
    data_base_[key] = {CacheType::kIn, pos, {}};

    return old_elem;
  }

  Tp LruHit(
      typename std::unordered_map<Key, ElInfo>::iterator hash_it) {
    auto list_it = hash_it->second.list_it;
    lru_cache_.splice(lru_cache_.begin(), lru_cache_, list_it);

    return lru_cache_.front().val;
  }

  Tp OutHit(const Key& key,
            typename std::unordered_map<Key, ElInfo>::iterator hash_it,
            std::function<Tp(const Key& key)> slow_get_page) {
    Tp old_elem = slow_get_page(key);
    out_cache_.erase(hash_it->second.key_it);

    if (lru_cache_.size() >= lru_cap_) {
      auto node = lru_cache_.back();
      lru_cache_.pop_back();
      data_base_.erase(node.key);
    }

    auto pos = lru_cache_.emplace(lru_cache_.begin(), {key, old_elem});
    data_base_[key] = {CacheType::kLru, pos, {}};

    return old_elem;
  }
};

#endif
