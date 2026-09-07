// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Class endpoints: the same connections, declared in a `class` instead of a `struct`.
 *
 * Nothing in the mechanism is struct specific - `struct` and `class` differ only in default access,
 * and access is what the three rules demonstrated here are about:
 *
 *  - the interface member has to be reachable where the connection is written, either because it is
 *    public, or through an accessor the endpoints befriend;
 *  - the members an interface lists may stay private - they are named inside the generated nested
 *    class, which is a member of the declaring class and therefore has access to its privates;
 *  - the interface has to be declared after the members it lists, because a class body is not a
 *    complete-class context - true of a struct too, but a class invites a leading `public:` block.
 *
 * Approach D is used throughout; the rules hold for A, B and C as well. The one difference is
 * approach C's dispatch, `p.pos_intf.act<&point::set>()`, which names the member at the call site
 * and so needs it public unless the dispatch happens inside the class.
 */
#include <iostream>
#include <string>

#include "4_intrinsic_interface_using_named.hpp"

namespace {
// ulang: pos_intf =interface { set; get_x; }
ENABLE_CONNECT_OPERATOR(pos_intf);

/**
 * @brief A class whose state and methods are private, and whose interface is public.
 *
 * @remark The interface is declared last: the members it lists must already be visible.
 */
class point : public untangle::enable_interfaces<point> {
 public:
  explicit point(std::string point_id) : id(std::move(point_id)) {}

 private:
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

 public:
  INTRINSIC_INTERFACE(pos_intf, set, get_x)
};

/**
 * @brief A different class declaring the same interface over its own private members.
 */
class display : public untangle::enable_interfaces<display> {
 private:
  int shown = 0;

  void set(int nx, int ny) {
    shown = nx;
    std::cout << "  display shows (" << nx << ", " << ny << ")" << std::endl;
  }

  [[nodiscard]] int get_x() const { return shown; }

 public:
  INTRINSIC_INTERFACE(pos_intf, set, get_x)
};

// ---------------------------------------------------------------------------
// an interface that stays private, reached through a befriended accessor
// ---------------------------------------------------------------------------
/**
 * @brief What ENABLE_CONNECT_OPERATOR declares, written as a named class so it can be befriended.
 *
 * @remark The macro's accessor is a lambda, and a closure type has no name to put in a `friend`
 * declaration - which is the only reason this one is spelled out.
 */
struct sync_accessor {
  template <typename endpoint_t>
  auto& operator()(endpoint_t& end_point) const {
    return end_point.sync_intf;
  }
};

inline constexpr auto sync_intf = untangle::intf(sync_accessor{});

/**
 * @brief A class that keeps its interface private, and still connects with the infix spelling.
 */
class counter {
 public:
  explicit counter(std::string counter_id) : id(std::move(counter_id)) {}

  [[nodiscard]] int value() const { return count; }

  friend struct sync_accessor;

 private:
  std::string id;
  int count = 0;

  void bump(int by) {
    count += by;
    std::cout << "  " << id << ".bump(" << by << ") -> " << count << std::endl;
  }

  INTRINSIC_INTERFACE_OF(sync_intf, counter, bump)
};
}  // namespace

int main() {
  std::cout << "\n[connection] a class connected to a class of another type" << std::endl;
  point a("A");
  display screen;

  // clang-format off
  a <pos_intf> screen;
  // clang-format on
  std::cout << "[dispatch] private members, actuated through the public interface" << std::endl;
  a.pos_intf.set_act(3, 4);

  std::cout << "[results]" << std::endl;
  a.pos_intf.get_x_act();
  for (const auto result : a.pos_intf.get_x_act.results) {
    std::cout << "  get_x -> " << result << std::endl;
  }

  std::cout << "[private interface] wired through the befriended accessor" << std::endl;
  counter first("first");
  counter second("second");

  // clang-format off
  first <sync_intf> second;
  // clang-format on
  sync_accessor{}(first).bump_act(2);
  std::cout << "  second.value() = " << second.value() << std::endl;

  std::cout << "[disconnect]" << std::endl;
  // clang-format off
  a >pos_intf< screen;
  // clang-format on
  a.pos_intf.set_act(5, 6);
  std::cout << "  actions left: " << a.pos_intf.set_act.actions.size() << std::endl;
  return 0;
}
