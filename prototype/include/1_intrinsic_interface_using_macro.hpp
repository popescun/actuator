// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Approach A: one macro per interface, signatures deduced from the members.
 *
 * @code
 * struct point : untangle::enable_interfaces<point> {
 *   void set(int nx, int ny);
 *   int get_x() const;
 *
 *   INTRINSIC_INTERFACE(pos_intf, set, get_x)   // ulang: pos_intf =interface { set; get_x; }
 * };
 * ENABLE_CONNECT_OPERATOR(pos_intf);
 * @endcode
 *
 * The return type and the parameter types are never spelled out: they are not known while the
 * preprocessor runs, but the expansion is ordinary C++, so `decltype(&self_t::set)` recovers them
 * when the compiler proper looks at the class.
 *
 * @remark Everything is resolved at compile time, including whether the two endpoints of a
 * connection declare compatible interfaces.
 * @remark The member list is limited by \ref INTRINSIC_FOR_EACH - 8 members here.
 * @remark An overloaded member can not be used directly, as `&self_t::member` is ambiguous. Declare
 * a uniquely named forwarding method and put that in the interface instead.
 */
#pragma once

#include "intrinsic_interface_core.hpp"

// ---------------------------------------------------------------------------
// for each helper
// ---------------------------------------------------------------------------
//! @remark INTRINSIC_EXPAND() is what makes this work with the traditional MSVC preprocessor.
#define INTRINSIC_EXPAND(x) x

#define INTRINSIC_FE_1(what, x) what(x)
#define INTRINSIC_FE_2(what, x, ...) what(x) INTRINSIC_EXPAND(INTRINSIC_FE_1(what, __VA_ARGS__))
#define INTRINSIC_FE_3(what, x, ...) what(x) INTRINSIC_EXPAND(INTRINSIC_FE_2(what, __VA_ARGS__))
#define INTRINSIC_FE_4(what, x, ...) what(x) INTRINSIC_EXPAND(INTRINSIC_FE_3(what, __VA_ARGS__))
#define INTRINSIC_FE_5(what, x, ...) what(x) INTRINSIC_EXPAND(INTRINSIC_FE_4(what, __VA_ARGS__))
#define INTRINSIC_FE_6(what, x, ...) what(x) INTRINSIC_EXPAND(INTRINSIC_FE_5(what, __VA_ARGS__))
#define INTRINSIC_FE_7(what, x, ...) what(x) INTRINSIC_EXPAND(INTRINSIC_FE_6(what, __VA_ARGS__))
#define INTRINSIC_FE_8(what, x, ...) what(x) INTRINSIC_EXPAND(INTRINSIC_FE_7(what, __VA_ARGS__))

#define INTRINSIC_FE_NTH(_1, _2, _3, _4, _5, _6, _7, _8, name, ...) name
#define INTRINSIC_FOR_EACH(what, ...)                                                            \
  INTRINSIC_EXPAND(INTRINSIC_FE_NTH(__VA_ARGS__, INTRINSIC_FE_8, INTRINSIC_FE_7, INTRINSIC_FE_6, \
                                    INTRINSIC_FE_5, INTRINSIC_FE_4, INTRINSIC_FE_3,              \
                                    INTRINSIC_FE_2, INTRINSIC_FE_1)(what, __VA_ARGS__))

// ---------------------------------------------------------------------------
// per member expansions
// ---------------------------------------------------------------------------
//! @brief The two members ulang generates per interface member: the actuator and the action.
#define INTRINSIC_INTERFACE_MEMBER(member)                                       \
  using member##_action_t =                                                      \
      typename untangle::member_signature<decltype(&owner_t::member)>::action_t; \
  member##_action_t member##_bound;                                              \
  member##_action_t* member##_action = nullptr;                                  \
  untangle::actuator<member##_action_t> member##_act;

#define INTRINSIC_INTERFACE_BIND(member) \
  member##_action = untangle::action_slot<&owner_t::member>(owner, member##_bound);

#define INTRINSIC_INTERFACE_WIRE(member)   \
  member##_act.add(other.member##_action); \
  parents[other.member##_action] = untangle::parent_record{other.self, &typeid(*other.self)};

#define INTRINSIC_INTERFACE_UNWIRE(member)    \
  member##_act.remove(other.member##_action); \
  parents.erase(other.member##_action);

/**
 * @brief Declares an intrinsic interface inside a class.
 *
 * The class must derive from untangle::enable_interfaces<itself>. Every member is either a method
 * or a std::function data member of that class, declared before this macro. For each member `m` the
 * generated interface object holds:
 *  - `m_act`    - the actuator, the ulang `<member>_act` dispatcher,
 *  - `m_action` - the action slot that a connection wires into a peer's actuator.
 *
 * @param intf_name - Interface name. It also names the generated member object, so both ulang
 * spellings work: `<var>.<intf>.<member>_act`, and `<intf>.<member>_act` inside a method of the
 * declaring class.
 * @param ... - The interface members, up to 8.
 */
#define INTRINSIC_INTERFACE(intf_name, ...)                                            \
  struct intf_name##_t : untangle::interface_base {                                    \
    using owner_t = self_t;                                                            \
                                                                                       \
    explicit intf_name##_t(owner_t* owner) : self(owner) {                             \
      INTRINSIC_FOR_EACH(INTRINSIC_INTERFACE_BIND, __VA_ARGS__)                        \
    }                                                                                  \
                                                                                       \
    /** @remark Tearing the connections down here, and not in the base destructor,     \
     * is what keeps the action slots below alive while a peer unwires them. */        \
    ~intf_name##_t() { drop_links(); }                                                 \
                                                                                       \
    owner_t* self = nullptr;                                                           \
                                                                                       \
    INTRINSIC_FOR_EACH(INTRINSIC_INTERFACE_MEMBER, __VA_ARGS__)                        \
                                                                                       \
    /** @brief Adds the other endpoint's actions into this endpoint's actuators. */    \
    template <typename other_t>                                                        \
    void connect_to(other_t& other) {                                                  \
      auto link = std::make_shared<untangle::interface_link>();                        \
      link->peer = &other;                                                             \
      link->unwire = [this, &other]() {                                                \
        INTRINSIC_FOR_EACH(INTRINSIC_INTERFACE_UNWIRE, __VA_ARGS__)                    \
      };                                                                               \
      INTRINSIC_FOR_EACH(INTRINSIC_INTERFACE_WIRE, __VA_ARGS__)                        \
      links.push_back({link, true});                                                   \
      other.links.push_back({link, false});                                            \
    }                                                                                  \
                                                                                       \
    /** @brief Removes the other endpoint's actions from this endpoint's actuators. */ \
    template <typename other_t>                                                        \
    void disconnect_from(other_t& other) {                                             \
      INTRINSIC_FOR_EACH(INTRINSIC_INTERFACE_UNWIRE, __VA_ARGS__)                      \
      for (auto& entry : links) {                                                      \
        if (entry.link->peer == &other) {                                              \
          entry.link->alive = false;                                                   \
        }                                                                              \
      }                                                                                \
      forget_link(&other);                                                             \
      other.forget_link(this);                                                         \
    }                                                                                  \
  } intf_name{this};
