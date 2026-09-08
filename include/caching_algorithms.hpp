#ifndef CACHING_ALGORITHMS_HPP_
#define CACHING_ALGORITHMS_HPP_

#include <list>
#include <unordered_map>
#include <algorithm>
#include <optional>

#include "cache.hpp"

//
//          2Q Algorithm
//
//  what need:
//      * std::list in_cache- new elements
//      * std::list out_cache - key of nodes, who have been displaced
//      * std::list lru_cache - main cache
//      * std::unordered map in class Cache [key] --> [object]  (key_map)
//      * size_t cache_capacity
//
//  we need paraments:
//      * in_cap - capacity of input data (take 25% of memory)
//      * out_cap - capacity of array with keys (take 50% of memory)
//      * lru_cap - capacity of main cache (take 25% of memory)
//
template <typename T>
void TwQueueAlg(const T& key) {

}

//helpful function for 2Q algorithms
enum class HitStatus {
    Hit,
    Miss
};

template <typename T, typename U>
std::optional<T> Get(const T& key) {
    if (!key_map.contains(key)) {           // if node key not in hash table - miss
        return std::nullopt;
    }

    const U& node = key_map[key];           // get node from hash table

    auto lru_it = std::find(lru_cache.begin(), lru_cache.end(), node);
    if (lru_it != lru_cache.end()) {
        lru_cache.push_front(node);
        return node;
    }

    auto in_it = std::find(in_cache.begin(), in_cache.end(), node);
    if (in_it != in_cache.end()) {
        return node;
    }

    return std::nullopt;
}


#endif
