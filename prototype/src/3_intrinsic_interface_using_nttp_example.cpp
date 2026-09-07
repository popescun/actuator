// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Approach C demo: interfaces declared without any macro.
 */
#include <iostream>
#include <string>

#include "3_intrinsic_interface_using_nttp.hpp"

// The connector that buys the `a <pos_intf> b` spelling, written out rather than through
// ENABLE_CONNECT_OPERATOR(pos_intf) - which is the same line - so that this approach uses no
// preprocessor at all.
inline constexpr auto pos_intf =
    untangle::intf([](auto& end_point) -> auto& { return end_point.pos_intf; });

struct point {
  explicit point(std::string point_id) : id(std::move(point_id)) {}

  std::string id;
  int x = 0;
  int y = 0;

  void set(int nx, int ny) {
    if (pos_intf.is_actuator()) {
      pos_intf.act<&point::set>()(nx, ny);
    } else {
      x = nx;
      y = ny;
    }
    std::cout << "  " << id << ".set(" << nx << ", " << ny << ")" << std::endl;
  }

  int get_x() const { return x; }

  // ulang: pos_intf =interface { set; get_x; }
  untangle::interface<&point::set, &point::get_x> pos_intf{this};
};

int main() {
  std::cout << "\n[connection] a <pos_intf> b" << std::endl;
  point a("A");
  point b("B");
  point c("C");

  // clang-format off
  a <pos_intf> untangle::endpoints(b, c);
  // clang-format on
  a.pos_intf.act<&point::set>()(3, 4);

  std::cout << "[results]" << std::endl;
  a.pos_intf.act<&point::get_x>()();
  for (const auto result : a.pos_intf.act<&point::get_x>().results) {
    std::cout << "  get_x -> " << result << std::endl;
  }

  std::cout << "[parent]" << std::endl;
  for (const auto* action : a.pos_intf.act<&point::set>().actions) {
    const auto* parent = a.pos_intf.parent_of<point>(action);
    std::cout << "  action parent: " << (parent != nullptr ? parent->id : "<none>") << std::endl;
  }

  std::cout << "[disconnect]" << std::endl;
  // clang-format off
  a >pos_intf< b;
  // clang-format on
  a.pos_intf.act<&point::set>()(5, 6);

  std::cout << "[lifetime]" << std::endl;
  {
    point d("D");
    // clang-format off
    a <pos_intf> d;
    // clang-format on
    std::cout << "  actions while d is alive: " << a.pos_intf.act<&point::set>().actions.size()
              << std::endl;
  }
  std::cout << "  actions after d died: " << a.pos_intf.act<&point::set>().actions.size()
            << std::endl;
  a.pos_intf.act<&point::set>()(7, 8);
  return 0;
}
