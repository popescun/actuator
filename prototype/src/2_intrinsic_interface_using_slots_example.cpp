// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Approach B demo: interfaces declared as self registering slots.
 */
#include <iostream>
#include <string>

#include "2_intrinsic_interface_using_slots.hpp"

ENABLE_CONNECT_OPERATOR(pos_intf);
ENABLE_CONNECT_OPERATOR(notify_intf);

/**
 * @brief A struct declaring an interface over two of its own methods.
 */
struct point {
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

  int get_x() const { return x; }

  // ulang: pos_intf =interface { set; get_x; }
  INTRINSIC_INTERFACE_BEGIN(point, pos_intf)
  INTRINSIC_SLOT(set, void, int, int)
  INTRINSIC_SLOT(get_x, int)
  INTRINSIC_INTERFACE_END(pos_intf)

  bool fire(int nx, int ny) {
    if (!pos_intf.set_act.is_connected()) {
      std::cout << "  " << id << ": pos_intf is not connected, skip!" << std::endl;
      return false;
    }
    pos_intf.set_act(nx, ny);
    return true;
  }
};

/**
 * @brief A different class declaring the same interface: the endpoints of a connection do not have
 * to be of the same type.
 */
struct logger {
  std::string id = "LOG";

  void set(int nx, int ny) {
    std::cout << "  " << id << " observed set(" << nx << ", " << ny << ")" << std::endl;
  }

  int get_x() const { return -1; }

  INTRINSIC_INTERFACE_BEGIN(logger, pos_intf)
  INTRINSIC_SLOT(set, void, int, int)
  INTRINSIC_SLOT(get_x, int)
  INTRINSIC_INTERFACE_END(pos_intf)
};

/**
 * @brief An interface member that is a funcref field.
 */
struct emitter {
  explicit emitter(std::string emitter_id) : id(std::move(emitter_id)) {}

  std::string id;
  std::function<void(int)> notify;

  INTRINSIC_INTERFACE_BEGIN(emitter, notify_intf)
  INTRINSIC_SLOT(notify, void, int)
  INTRINSIC_INTERFACE_END(notify_intf)
};

void test_connection() {
  std::cout << "\n[connection] a <pos_intf> b, heterogeneous endpoints" << std::endl;
  point a("A");
  point b("B");
  logger l;

  // clang-format off
  a <pos_intf> b;
  // clang-format on
  // clang-format off
  a <pos_intf> l;
  // clang-format on
  a.set(3, 4);
}

void test_results_and_parent() {
  std::cout << "\n[results, parent]" << std::endl;
  point a("A");
  point b("B");
  point c("C");

  // clang-format off
  a <pos_intf> untangle::endpoints(b, c);
  // clang-format on
  b.set(11, 0);
  c.set(22, 0);

  a.pos_intf.get_x_act();
  for (const auto result : a.pos_intf.get_x_act.results()) {
    std::cout << "  get_x -> " << result << std::endl;
  }

  for (const auto* action : a.pos_intf.set_act.actions()) {
    const auto* parent = a.pos_intf.parent_of<point>(action);
    std::cout << "  action parent: " << (parent != nullptr ? parent->id : "<none>") << std::endl;
  }
}

void test_disconnect_and_lifetime() {
  std::cout << "\n[disconnect, lifetime]" << std::endl;
  point a("A");
  {
    point b("B");
    // clang-format off
    a <pos_intf> b;
    // clang-format on
    // clang-format off
    a >pos_intf< b;
    // clang-format on
    std::cout << "  connected after disconnect: " << a.pos_intf.set_act.is_connected() << std::endl;
    // clang-format off
    a <pos_intf> b;
    // clang-format on
  }
  std::cout << "  connected after b died: " << a.pos_intf.set_act.is_connected() << std::endl;
  a.pos_intf.set_act(1, 1);  // no dangling action is invoked

  // the other destruction order: the actuator side dies first
  {
    point d("D");
    {
      point e("E");
      // clang-format off
      d <pos_intf> e;
      // clang-format on
    }
  }
  std::cout << "  both destruction orders survived" << std::endl;
}

void test_funcref_field() {
  std::cout << "\n[funcref field]" << std::endl;
  emitter source("SOURCE");
  emitter sink("SINK");
  sink.notify = [](int value) { std::cout << "  SINK.notify(" << value << ")" << std::endl; };

  // clang-format off
  source <notify_intf> sink;
  // clang-format on
  source.notify_intf.notify_act(42);
}

void test_mismatch() {
  std::cout << "\n[mismatch] connecting an interface the peer does not declare" << std::endl;
  point a("A");
  emitter e("E");
  try {
    // pos_intf on the left, but the emitter's pos_intf does not exist: this is a compile error.
    // A member missing at run time is reported instead:
    a.pos_intf.connect_to(e.notify_intf);
  } catch (const std::runtime_error& error) {
    std::cout << "  " << error.what() << std::endl;
  }
}

int main() {
  test_connection();
  test_results_and_parent();
  test_disconnect_and_lifetime();
  test_funcref_field();
  test_mismatch();
  return 0;
}
