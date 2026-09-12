#include <cstdint>
#include <exception>
#include <iostream>

#include "config.hpp"
#include "cache_hierarchy.hpp"

int main() {
  try {
    cache::Config meow("config/config.json");

    const auto& vec = meow.GetCacheHierarchy();
    for (const auto& i : vec) {
      std::cout << static_cast<int>(i.first) << " " << i.second << "\n";
    }
    std::cout << meow.GetIsDumpEnabled() << "\n";

  } catch (const std::exception& ex) {
    std::cerr << ex.what();
  }
  return 0;
}
