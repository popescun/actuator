// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Prototype: machinery shared by every "intrinsic interface" approach.
 *
 * It provides what none of the approaches wants to reinvent:
 *  - \ref untangle::member_signature - the action type of an interface member,
 *  - \ref untangle::interface_base   - connection bookkeeping, lifetime and `.parent`,
 *  - the `a <intf> b` / `a >intf< b` infix operators.
 *
 * ulang spelling                        | C++ spelling
 * --------------------------------------|--------------------------------------------------
 * `a <pos_intf> b;`                     | `a <pos_intf> b;`
 * `a <pos_intf> {b, c};`                | `a <pos_intf> untangle::endpoints(b, c);`
 * `a <p1> b <p2> c;`                    | `a <p1> b <p2> c;`
 * `a >pos_intf< b;`                     | `a >pos_intf< b;`
 * `<intf>.is_actuator` / `.is_action`   | `<intf>.is_actuator()` / `<intf>.is_action()`
 * `<act>.actions[i].parent`             | `<intf>.parent_of<T>(action)`
 */
#pragma once

#include <actuator.hpp>
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace untangle {

/**
 * @brief Deduces the action type of an interface member.
 *
 * An interface member is either a method of the declaring class, or a data member of type
 * std::function<...> - the equivalent of a ulang funcref field. Both map to the same action type,
 * which is what makes the two member kinds interchangeable at every use site.
 */
template <typename T>
struct member_signature;

template <typename class_t, typename R, typename... Args>
struct member_signature<R (class_t::*)(Args...)> {
  using owner_t = class_t;
  using action_t = std::function<R(Args...)>;
  static constexpr bool is_method = true;
};

template <typename class_t, typename R, typename... Args>
struct member_signature<R (class_t::*)(Args...) const> {
  using owner_t = class_t;
  using action_t = std::function<R(Args...)>;
  static constexpr bool is_method = true;
};

template <typename class_t, typename R, typename... Args>
struct member_signature<R (class_t::*)(Args...) noexcept> {
  using owner_t = class_t;
  using action_t = std::function<R(Args...)>;
  static constexpr bool is_method = true;
};

template <typename class_t, typename R, typename... Args>
struct member_signature<R (class_t::*)(Args...) const noexcept> {
  using owner_t = class_t;
  using action_t = std::function<R(Args...)>;
  static constexpr bool is_method = true;
};

/**
 * @brief Specialization for a funcref field: a data member of type std::function<...>.
 */
template <typename class_t, typename R, typename... Args>
struct member_signature<std::function<R(Args...)> class_t::*> {
  using owner_t = class_t;
  using action_t = std::function<R(Args...)>;
  static constexpr bool is_method = false;
};

/**
 * @brief Carries the declaring type into the class body, so an interface declaration does not have
 * to repeat the class name.
 *
 * A class body is not a complete-class context, so `decltype(*this)` is not available where the
 * interface members are declared. Inheriting this base publishes `self_t` before the class is
 * complete, which is enough for `decltype(&self_t::method)`.
 */
template <typename derived_t>
struct enable_interfaces {
  using self_t = derived_t;
};

/**
 * @brief Returns the action slot of an interface member.
 *
 * For a method it binds the method to the owner and returns the bound storage; for a funcref field
 * it returns the field itself, so a later reassignment of the field is seen by every actuator the
 * field is wired into.
 */
template <auto member, typename owner_t, typename action_t>
action_t* action_slot(owner_t* owner, action_t& storage) {
  if constexpr (std::is_member_function_pointer_v<decltype(member)>) {
    storage = untangle::bind(owner, member);
    return &storage;
  } else {
    return &(owner->*member);
  }
}

/**
 * @brief Runtime flavour of \ref action_slot(), for a member pointer that is a constructor
 * argument rather than a template argument.
 */
template <typename owner_t, typename member_t, typename action_t>
action_t* action_slot_of(owner_t* owner, member_t member, action_t& storage) {
  if constexpr (std::is_member_function_pointer_v<member_t>) {
    storage = untangle::bind(owner, member);
    return &storage;
  } else {
    return &(owner->*member);
  }
}

/**
 * @brief One `a <intf> b` connection, shared by both endpoints.
 *
 * The record lets whichever endpoint is destroyed first tear the connection down: the action side
 * unwires itself from the still living actuator side, the actuator side only marks the record dead.
 */
struct interface_link {
  const void* peer = nullptr;    //!< The other endpoint's interface object.
  std::function<void()> unwire;  //!< Removes the action side's actions from the actuator side.
  bool alive = true;             //!< False once either endpoint tore the connection down.
};

/**
 * @brief Identifies the struct behind a connected action - the ulang `.parent`.
 */
struct parent_record {
  void* object = nullptr;                //!< The connected instance.
  const std::type_info* type = nullptr;  //!< Its type, checked by \ref interface_base::parent_of().
};

/**
 * @brief Connection bookkeeping common to every generated interface object.
 */
struct interface_base {
  struct link_entry {
    std::shared_ptr<interface_link> link;
    bool actuator_side = false;
  };

  std::vector<link_entry> links;                 //!< Connections this interface takes part in.
  std::map<const void*, parent_record> parents;  //!< Action slot -> connected instance.

  interface_base() = default;
  interface_base(const interface_base&) = delete;
  interface_base& operator=(const interface_base&) = delete;
  interface_base(interface_base&&) = delete;
  interface_base& operator=(interface_base&&) = delete;
  ~interface_base() { drop_links(); }

  /**
   * @brief True if this endpoint is the sender of at least one connection - ulang `is_actuator`.
   */
  bool is_actuator() const {
    return std::any_of(links.begin(), links.end(),
                       [](const auto& e) { return e.link->alive && e.actuator_side; });
  }

  /**
   * @brief True if this endpoint is the receiver of at least one connection - ulang `is_action`.
   */
  bool is_action() const {
    return std::any_of(links.begin(), links.end(),
                       [](const auto& e) { return e.link->alive && !e.actuator_side; });
  }

  /**
   * @brief The struct that a connection wired in as the action behind @p action.
   *
   * @param action - An element of an actuator's actions list.
   * @return parent_t* - Null when the action was added by hand rather than by a connection, or when
   * the connected instance is not a parent_t. The type check makes the pointer safe to follow.
   */
  template <typename parent_t>
  parent_t* parent_of(const void* action) const {
    const auto it = parents.find(action);
    if (it == parents.end() || it->second.type == nullptr) {
      return nullptr;
    }
    if (*it->second.type != typeid(parent_t)) {
      return nullptr;
    }
    return static_cast<parent_t*>(it->second.object);
  }

  /**
   * @brief Tears down every connection this endpoint takes part in.
   */
  void drop_links() {
    for (auto& entry : links) {
      if (!entry.link->alive) {
        continue;
      }
      entry.link->alive = false;
      // Only the action side has to unwire: the actuator side owns the actuators, and they go away
      // together with it.
      if (!entry.actuator_side && entry.link->unwire) {
        entry.link->unwire();
      }
    }
    links.clear();
  }

  /**
   * @brief Drops the bookkeeping of the connections with @p peer, and of any dead connection.
   */
  void forget_link(const void* peer) {
    links.erase(
        std::remove_if(links.begin(), links.end(),
                       [peer](const auto& e) { return !e.link->alive || e.link->peer == peer; }),
        links.end());
  }
};

/**
 * @brief An interface name, reified as a member projection.
 *
 * The `<intf>` operators need a name that is not a member, and that can be applied to either
 * endpoint independently - a member pointer can not, because the two endpoints may be of different
 * types. This carries the projection in its *type*: @p accessor_t is a captureless lambda, whose
 * closure type is default constructible in C++20, so \ref of() needs no stored object and the
 * reference itself stays empty.
 *
 * @remark It lives in namespace `untangle`, which is what puts the operators in the associated
 * namespaces of `a <intf> b` and lets argument dependent lookup find them.
 */
template <typename accessor_t>
struct interface_ref {
  template <typename endpoint_t>
  static auto& of(endpoint_t& end_point) {
    return accessor_t{}(end_point);
  }
};

/**
 * @brief Builds an \ref interface_ref from an accessor, deducing the closure type.
 */
template <typename accessor_t>
constexpr interface_ref<accessor_t> intf(accessor_t) {
  return {};
}

template <typename T>
inline constexpr bool is_interface_ref_impl_v = false;

template <typename accessor_t>
inline constexpr bool is_interface_ref_impl_v<interface_ref<accessor_t>> = true;

template <typename T>
inline constexpr bool is_interface_ref_v = is_interface_ref_impl_v<std::remove_cvref_t<T>>;

/**
 * @brief The left hand side of a pending `a <intf> b` connection.
 */
template <typename lhs_t, typename ref_t>
struct connect_expression {
  lhs_t* lhs;
};

/**
 * @brief The left hand side of a pending `a >intf< b` disconnection.
 */
template <typename lhs_t, typename ref_t>
struct disconnect_expression {
  lhs_t* lhs;
};

/**
 * @brief A `{b, c}` fan out list: `a <intf> untangle::endpoints(b, c)`.
 */
template <typename... endpoints_t>
struct endpoint_list {
  std::tuple<endpoints_t*...> items;
};

template <typename... endpoints_t>
endpoint_list<endpoints_t...> endpoints(endpoints_t&... es) {
  return {std::make_tuple(&es...)};
}

/**
 * @brief `a <intf ...` - the first half of a connection.
 *
 * @remark `a <intf> b` parses as `(a < intf) > b`, which is what makes the ulang spelling legal C++
 * without any language extension.
 */
template <typename lhs_t, typename ref_t, std::enable_if_t<is_interface_ref_v<ref_t>, int> = 0>
connect_expression<lhs_t, ref_t> operator<(lhs_t& lhs, const ref_t&) {
  return {&lhs};
}

/**
 * @brief `... > b` - the second half of a connection.
 *
 * @return rhs_t& - The right endpoint, so that `a <i1> b <i2> c` chains as it does in ulang.
 */
template <typename lhs_t, typename ref_t, typename rhs_t,
          std::enable_if_t<!is_interface_ref_v<rhs_t>, int> = 0>
rhs_t& operator>(connect_expression<lhs_t, ref_t> expression, rhs_t& rhs) {
  ref_t::of(*expression.lhs).connect_to(ref_t::of(rhs));
  return rhs;
}

/**
 * @brief `... > untangle::endpoints(b, c)` - fan out to several endpoints at once.
 */
template <typename lhs_t, typename ref_t, typename... endpoints_t>
lhs_t& operator>(connect_expression<lhs_t, ref_t> expression, endpoint_list<endpoints_t...> list) {
  std::apply(
      [&expression](auto*... endpoint) {
        (ref_t::of(*expression.lhs).connect_to(ref_t::of(*endpoint)), ...);
      },
      list.items);
  return *expression.lhs;
}

/**
 * @brief `a >intf ...` - the first half of a disconnection.
 */
template <typename lhs_t, typename ref_t, std::enable_if_t<is_interface_ref_v<ref_t>, int> = 0>
disconnect_expression<lhs_t, ref_t> operator>(lhs_t& lhs, const ref_t&) {
  return {&lhs};
}

/**
 * @brief `... < b` - the second half of a disconnection.
 */
template <typename lhs_t, typename ref_t, typename rhs_t,
          std::enable_if_t<!is_interface_ref_v<rhs_t>, int> = 0>
rhs_t& operator<(disconnect_expression<lhs_t, ref_t> expression, rhs_t& rhs) {
  ref_t::of(*expression.lhs).disconnect_from(ref_t::of(rhs));
  return rhs;
}

}  // namespace untangle

/**
 * @brief Declares the connector that enables `a <intf_name> b` and `a >intf_name< b`.
 *
 * It must appear at namespace scope, once per interface name - not per class, because the
 * projection is a template and applies to any endpoint declaring that member. It expands to the
 * one liner it is a shorthand for:
 *
 * @code
 * inline constexpr auto pos_intf =
 *     untangle::intf([](auto& end_point) -> auto& { return end_point.pos_intf; });
 * @endcode
 *
 * @remark Writing that line by hand is equally valid, and needs no preprocessor at all.
 */
#define ENABLE_CONNECT_OPERATOR(intf_name) \
  inline constexpr auto intf_name =        \
      untangle::intf([](auto& end_point) -> auto& { return end_point.intf_name; })
