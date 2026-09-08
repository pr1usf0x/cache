#ifndef CACHE_HPP_
#define CACHE_HPP_

#include <cstdint>
#include <functional>

template <typename T>
class Cache {
  virtual const T& LookUpUpdate(uint64_t key,
                                std::function<const T&()> slow_get_page);

  virtual ~Cache();
};

#endif  // CACHE_HPP_