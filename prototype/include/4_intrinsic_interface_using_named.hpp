// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Approach D: a thin macro over approach C, so the actuators get their ulang names.
 *
 * @code
 * struct point : untangle::enable_interfaces<point> {
 *   void set(int nx, int ny);
 *   int get_x() const;
 *
 *   INTRINSIC_INTERFACE(pos_intf, set, get_x)
 * };
 * @endcode
 *
 * Approach C can not spell `pos_intf.set_act` because a template can not invent an identifier, and
 * approach A buys that name by generating the whole interface - the binds, the wiring, the
 * unwiring - in the preprocessor. This approach takes the name from A and the machinery from C: the
 * macro emits a member pointer pack and one reference per member, and everything else is inherited
 * from untangle::interface<>. What the macro expands to is a struct declaration and a list of
 * references, which is short enough to read at the point of use.
 *
 * @remark Because the action slots stay in the base, untangle::interface<>::~interface() still
 * tears the connections down while they are alive - this approach adds no destructor of its own.
 * @remark Members are paired positionally, as in approach C, and checked at compile time.
 * @remark The member list is limited by the expansions below - 8 members here, as in approach A.
 */
#pragma once

#include "3_intrinsic_interface_using_nttp.hpp"

// ---------------------------------------------------------------------------
// member pointer pack: (point, set, get_x) -> &point::set, &point::get_x
// ---------------------------------------------------------------------------
//! @brief Forces a second expansion pass, which the traditional MSVC preprocessor needs.
#define INTRINSIC_MP_EXPAND(x) x

#define INTRINSIC_MP_1(owner, x) &owner::x
#define INTRINSIC_MP_2(owner, x, ...) \
  &owner::x, INTRINSIC_MP_EXPAND(INTRINSIC_MP_1(owner, __VA_ARGS__))
#define INTRINSIC_MP_3(owner, x, ...) \
  &owner::x, INTRINSIC_MP_EXPAND(INTRINSIC_MP_2(owner, __VA_ARGS__))
#define INTRINSIC_MP_4(owner, x, ...) \
  &owner::x, INTRINSIC_MP_EXPAND(INTRINSIC_MP_3(owner, __VA_ARGS__))
#define INTRINSIC_MP_5(owner, x, ...) \
  &owner::x, INTRINSIC_MP_EXPAND(INTRINSIC_MP_4(owner, __VA_ARGS__))
#define INTRINSIC_MP_6(owner, x, ...) \
  &owner::x, INTRINSIC_MP_EXPAND(INTRINSIC_MP_5(owner, __VA_ARGS__))
#define INTRINSIC_MP_7(owner, x, ...) \
  &owner::x, INTRINSIC_MP_EXPAND(INTRINSIC_MP_6(owner, __VA_ARGS__))
#define INTRINSIC_MP_8(owner, x, ...) \
  &owner::x, INTRINSIC_MP_EXPAND(INTRINSIC_MP_7(owner, __VA_ARGS__))

#define INTRINSIC_MP_NTH(_1, _2, _3, _4, _5, _6, _7, _8, name, ...) name

/**
 * @brief The template argument list for untangle::interface<>, built from bare member names.
 */
#define INTRINSIC_MEMBER_POINTERS(owner, ...)                                                      \
  INTRINSIC_MP_EXPAND(INTRINSIC_MP_NTH(                                                            \
      __VA_ARGS__, INTRINSIC_MP_8, INTRINSIC_MP_7, INTRINSIC_MP_6, INTRINSIC_MP_5, INTRINSIC_MP_4, \
      INTRINSIC_MP_3, INTRINSIC_MP_2, INTRINSIC_MP_1)(owner, __VA_ARGS__))

// ---------------------------------------------------------------------------
// per member expansion: set -> set_act
// ---------------------------------------------------------------------------
/**
 * @brief The ulang `<member>_act`, as a reference to the actuator the base already holds.
 *
 * @remark A reference, and not a copy: the actuator lives in untangle::interface<>::slots, and a
 * slot must never be relocated.
 */
#define INTRINSIC_ACT_REF(member)                                              \
  typename untangle::member_slot<&owner_t::member>::actuator_t& member##_act = \
      act<&owner_t::member>();

#define INTRINSIC_AR_1(x) INTRINSIC_ACT_REF(x)
#define INTRINSIC_AR_2(x, ...) INTRINSIC_ACT_REF(x) INTRINSIC_MP_EXPAND(INTRINSIC_AR_1(__VA_ARGS__))
#define INTRINSIC_AR_3(x, ...) INTRINSIC_ACT_REF(x) INTRINSIC_MP_EXPAND(INTRINSIC_AR_2(__VA_ARGS__))
#define INTRINSIC_AR_4(x, ...) INTRINSIC_ACT_REF(x) INTRINSIC_MP_EXPAND(INTRINSIC_AR_3(__VA_ARGS__))
#define INTRINSIC_AR_5(x, ...) INTRINSIC_ACT_REF(x) INTRINSIC_MP_EXPAND(INTRINSIC_AR_4(__VA_ARGS__))
#define INTRINSIC_AR_6(x, ...) INTRINSIC_ACT_REF(x) INTRINSIC_MP_EXPAND(INTRINSIC_AR_5(__VA_ARGS__))
#define INTRINSIC_AR_7(x, ...) INTRINSIC_ACT_REF(x) INTRINSIC_MP_EXPAND(INTRINSIC_AR_6(__VA_ARGS__))
#define INTRINSIC_AR_8(x, ...) INTRINSIC_ACT_REF(x) INTRINSIC_MP_EXPAND(INTRINSIC_AR_7(__VA_ARGS__))

#define INTRINSIC_ACT_REFS(...)                                                                    \
  INTRINSIC_MP_EXPAND(INTRINSIC_MP_NTH(                                                            \
      __VA_ARGS__, INTRINSIC_AR_8, INTRINSIC_AR_7, INTRINSIC_AR_6, INTRINSIC_AR_5, INTRINSIC_AR_4, \
      INTRINSIC_AR_3, INTRINSIC_AR_2, INTRINSIC_AR_1)(__VA_ARGS__))

// ---------------------------------------------------------------------------
// the interface declaration
// ---------------------------------------------------------------------------
/**
 * @brief Declares an intrinsic interface whose actuators carry the member names.
 *
 * @param intf_name - The interface member, as in ulang `pos_intf =interface { ... }`.
 * @param owner_class - The declaring class. \ref INTRINSIC_INTERFACE drops this argument.
 * @param ... - The members, by bare name. Methods or std::function fields, up to 8.
 */
#define INTRINSIC_INTERFACE_OF(intf_name, owner_class, ...)                                  \
  struct intf_name##_t                                                                       \
      : untangle::interface<INTRINSIC_MEMBER_POINTERS(owner_class, __VA_ARGS__)> {           \
    using owner_t = owner_class;                                                             \
    using base_t = untangle::interface<INTRINSIC_MEMBER_POINTERS(owner_class, __VA_ARGS__)>; \
    using base_t::base_t;                                                                    \
                                                                                             \
    INTRINSIC_ACT_REFS(__VA_ARGS__)                                                          \
  } intf_name{this};

/**
 * @brief \ref INTRINSIC_INTERFACE_OF for a class deriving untangle::enable_interfaces<>,
 * which publishes the declaring type as `self_t` so it does not have to be repeated.
 */
#define INTRINSIC_INTERFACE(intf_name, ...) INTRINSIC_INTERFACE_OF(intf_name, self_t, __VA_ARGS__)
