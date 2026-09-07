// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Approach A demo: intrinsic interfaces declared with a signature deducing macro.
 *
 * It mirrors the ulang examples from the language SPEC, side by side with the C++ spelling.
 */
#include <iostream>
#include <string>

#include "1_intrinsic_interface_using_macro.hpp"

// ulang: pos_intf =interface { set; get_x; }
ENABLE_CONNECT_OPERATOR(pos_intf);
// ulang: notify_intf =interface { notify }  -- a funcref field member
ENABLE_CONNECT_OPERATOR(notify_intf);

/**
 * @brief A struct declaring an interface over two of its own methods.
 */
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

  int get_x() const { return x; }

  // ulang: pos_intf =interface { set; get_x; }
  INTRINSIC_INTERFACE(pos_intf, set, get_x)

  /**
   * @brief The interface is reachable without a `<var>.` prefix inside a method of its own struct.
   */
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
 * @brief A struct whose interface member is a funcref field rather than a method.
 */
struct emitter : untangle::enable_interfaces<emitter> {
  explicit emitter(std::string emitter_id) : id(std::move(emitter_id)) {}

  std::string id;
  std::function<void(int)> notify;  // ulang: notify: cb_t =fr

  INTRINSIC_INTERFACE(notify_intf, notify)
};

void test_connection() {
  std::cout << "\n[connection] a <pos_intf> b" << std::endl;
  point a("A");
  point b("B");

  // clang-format off
  a <pos_intf> b;  // ulang: a <pos_intf> b;
  // clang-format on

  a.set(3, 4);
  std::cout << "  a: " << a.x << "," << a.y << "  b: " << b.x << "," << b.y << std::endl;
}

void test_fan_out_and_chaining() {
  std::cout << "\n[fan out] a <pos_intf> {b, c}" << std::endl;
  point a("A");
  point b("B");
  point c("C");

  // clang-format off
  a <pos_intf> untangle::endpoints(b, c);  // ulang: a <pos_intf> {b, c};
  // clang-format on
  a.pos_intf.set_act(1, 2);

  std::cout << "[chaining] a <pos_intf> b <pos_intf> c" << std::endl;
  point d("D");
  point e("E");
  point f("F");
  // clang-format off
  d <pos_intf> e <pos_intf> f;  // d -> e, e -> f
  // clang-format on
  d.pos_intf.set_act(5, 6);
  std::cout << "  e fires f:" << std::endl;
  e.pos_intf.set_act(7, 8);
}

void test_disconnect() {
  std::cout << "\n[disconnect] a >pos_intf< b" << std::endl;
  point a("A");
  point b("B");

  // clang-format off
  a <pos_intf> b;
  // clang-format on
  // clang-format off
  a >pos_intf< b;  // ulang: a >pos_intf< b;
  // clang-format on
  std::cout << "  connected after disconnect: " << a.pos_intf.set_act.is_connected() << std::endl;
  a.pos_intf.set_act(9, 9);
  std::cout << "  b: " << b.x << "," << b.y << " (unchanged)" << std::endl;
}

void test_results() {
  std::cout << "\n[results] non void return type" << std::endl;
  point a("A");
  point b("B");
  point c("C");

  // clang-format off
  a <pos_intf> untangle::endpoints(b, c);
  // clang-format on
  b.set(11, 0);
  c.set(22, 0);

  a.pos_intf.get_x_act();
  for (const auto result : a.pos_intf.get_x_act.results) {
    std::cout << "  get_x -> " << result << std::endl;
  }
}

void test_parent() {
  std::cout << "\n[parent] identify the struct behind a connected action" << std::endl;
  point a("A");
  point b("B");
  point c("C");

  // clang-format off
  a <pos_intf> untangle::endpoints(b, c);
  // clang-format on

  for (const auto* action : a.pos_intf.set_act.actions) {
    const auto* parent = a.pos_intf.parent_of<point>(action);
    std::cout << "  action parent: " << (parent != nullptr ? parent->id : "<none>") << std::endl;
  }

  std::cout << "  a.pos_intf.is_actuator: " << a.pos_intf.is_actuator()
            << "  b.pos_intf.is_action: " << b.pos_intf.is_action() << std::endl;
}

void test_lifetime() {
  std::cout << "\n[lifetime] the action side is unwired when it dies" << std::endl;
  point a("A");
  {
    point b("B");
    // clang-format off
    a <pos_intf> b;
    // clang-format on
    std::cout << "  connected: " << a.pos_intf.set_act.is_connected() << std::endl;
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
  std::cout << "\n[funcref field] an interface member that is a std::function field" << std::endl;
  emitter source("SOURCE");
  emitter sink("SINK");

  sink.notify = [&sink](int value) {
    std::cout << "  " << sink.id << ".notify(" << value << ")" << std::endl;
  };

  // clang-format off
  source <notify_intf> sink;
  // clang-format on
  source.notify_intf.notify_act(42);

  // The field is the callable, so reassigning it is seen through the connection.
  sink.notify = [](int value) { std::cout << "  rebound notify(" << value << ")" << std::endl; };
  source.notify_intf.notify_act(43);
}

void test_internal_fire() {
  std::cout << "\n[internal] the declaring struct fires its own interface" << std::endl;
  point a("A");
  point b("B");
  a.fire(1, 1);
  // clang-format off
  a <pos_intf> b;
  // clang-format on
  a.fire(2, 2);
}

int main() {
  test_connection();
  test_fan_out_and_chaining();
  test_disconnect();
  test_results();
  test_parent();
  test_lifetime();
  test_funcref_field();
  test_internal_fire();
  return 0;
}
