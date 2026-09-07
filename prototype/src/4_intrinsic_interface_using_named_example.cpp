// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Approach D demo: the declaration of approach C, spelled with the names of approach A.
 *
 * The same demo as 3_intrinsic_interface_using_nttp_example.cpp, so the two can be read side by
 * side: the only difference is `pos_intf.set_act(3, 4)` where approach C needs
 * `pos_intf.act<&point::set>()(3, 4)`.
 */
#include <iostream>
#include <string>

#include "4_intrinsic_interface_using_named.hpp"

namespace {
struct point : untangle::enable_interfaces<point> {
  explicit point(std::string point_id) : id(std::move(point_id)) {}

  std::string id;
  int x = 0;
  int y = 0;

  void set(int nx, int ny) {
    if (pos_intf.is_actuator()) {
      pos_intf.set_act(nx, ny);
    } else {
      x = nx;
      y = ny;
    }
    std::cout << "  " << id << ".set(" << nx << ", " << ny << ")" << std::endl;
  }

  [[nodiscard]] int get_x() const { return x; }

  // ulang: pos_intf =interface { set; get_x; }
  INTRINSIC_INTERFACE(pos_intf, set, get_x)
};
// Enable connection expression via `a <pos_intf> b` spelling, exactly as in approach C.
ENABLE_CONNECT_OPERATOR(pos_intf);
}  // namespace

int main() {
  std::cout << "\n[connection] a <pos_intf> b" << std::endl;
  point a("A");
  point b("B");
  point c("C");

  // clang-format off
  a <pos_intf> untangle::endpoints(b, c);
  // clang-format on
  a.pos_intf.set_act(3, 4);

  std::cout << "[results]" << std::endl;
  a.pos_intf.get_x_act();
  for (const auto result : a.pos_intf.get_x_act.results) {
    std::cout << "  get_x -> " << result << std::endl;
  }

  std::cout << "[parent]" << std::endl;
  for (const auto* action : a.pos_intf.set_act.actions) {
    const auto* parent = a.pos_intf.parent_of<point>(action);
    std::cout << "  action parent: " << (parent != nullptr ? parent->id : "<none>") << std::endl;
  }

  std::cout << "[disconnect]" << std::endl;
  // clang-format off
  a >pos_intf< b;
  // clang-format on
  a.pos_intf.set_act(5, 6);

  std::cout << "[lifetime]" << std::endl;
  {
    point d("D");
    // clang-format off
    a <pos_intf> d;
    // clang-format on
    std::cout << "  actions while d is alive: " << a.pos_intf.set_act.actions.size() << std::endl;
  }
  std::cout << "  actions after d died: " << a.pos_intf.set_act.actions.size() << std::endl;
  a.pos_intf.set_act(7, 8);
  return 0;
}
