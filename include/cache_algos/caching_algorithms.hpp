#ifndef CACHING_ALGORITHMS_HPP_
#define CACHING_ALGORITHMS_HPP_

#include <list>
#include <unordered_map>
#include <functional>
#include <algorithm>

#include "cache.hpp"

enum class CacheType {
    In,
    Out,
    Lru
};


template <typename T, typename U>
struct CacheNode {
    T key;
    U val;
};

template <typename T,typename U>
struct ElInfo {
    CacheType cache_type;
    typename std::list<CacheNode<T, U>>::iterator list_it;
    typename std::list<T>::iterator key_it;
};

template <typename T, typename U>
class TwoQueues: public BaseAlgorithm {
public:
    TwoQueues(size_t cache_cap)
        : cache_cap(cache_cap)
        , in_cap(std::max<size_t>(1, cache_cap * 0.1))
        , out_cap(std::max<size_t>(1, cache_cap * 0.3))
        , lru_cap(std::max<size_t>(1, cache_cap * 0.6)) {}

    U LookupUpdate(const T& key, std::function<U(const T& key)> slow_get_page) override {
        auto hash_it = data_base.find(key);
        if (hash_it == data_base.end()) {
            U old_elem = slow_get_page(key);
            if (in_cache.size() >= in_cap) {
                auto node = in_cache.back();
                in_cache.pop_back();
                if (out_cache.size() >= out_cap) {
                    T out_key = out_cache.back();
                    out_cache.pop_back();
                    data_base.erase(out_key);
                }
                auto pos = out_cache.emplace(out_cache.begin(), node.key);
                data_base[node.key] = {CacheType::Out, {}, pos};
            }
            auto pos = in_cache.emplace(in_cache.begin(), {key, old_elem});
            data_base[key] = {CacheType::In, pos, {}};
            return old_elem;
        }
        switch(hash_it->second.cache_type) {
            case CacheType::Lru: {
                auto list_it = hash_it->second.list_it;
                lru_cache.splice(lru_cache.begin(), lru_cache, list_it);
                return lru_cache.front().val;
            }
            case CacheType::Out: {
                U old_elem = slow_get_page(key);
                out_cache.erase(hash_it->second.key_it);
                if (lru_cache.size() >= lru_cap) {
                    auto node = lru_cache.back();
                    lru_cache.pop_back();
                    data_base.erase(node.key);

                }
                auto pos = lru_cache.emplace(lru_cache.begin(), {key, old_elem});
                data_base[key] = {CacheType::Lru, pos, {}};
                return old_elem;
            }
            case CacheType::In:
                return (hash_it->second).list_it->val;
            default:
        }
        return {};
    }

private:
size_t cache_cap;
size_t in_cap;
size_t out_cap;
size_t lru_cap;
std::list<CacheNode<T, U>> in_cache;
std::list<T> out_cache;
std::list<CacheNode<T, U>> lru_cache;
std::unordered_map<T, ElInfo<T, U>> data_base;
};


#endif
