#ifndef CACHE_HPP_
#define CACHE_HPP_

#include <cstdint>
#include <functional>

template <typename Key, typename Tp>
class Cache {
  virtual const Tp& LookUpUpdate(Key key,
                                 std::function<const Tp&()> slow_get_page);

  virtual ~Cache();
};

#endif  // CACHE_HPP_