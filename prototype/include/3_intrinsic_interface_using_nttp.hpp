// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Approach C: no macro at all - the interface is a member template parameterised by member
 * pointers.
 *
 * @code
 * struct point {
 *   void set(int nx, int ny);
 *   int get_x() const;
 *
 *   untangle::interface<&point::set, &point::get_x> pos_intf{this};
 * };
 * @endcode
 *
 * Everything the other approaches generate is here too - actuators, actions, connections, lifetime,
 * `.parent` - and it is all ordinary C++: no preprocessor, so the declaration is debuggable and
 * greppable. What is lost is the naming: a template can not invent the identifier `set_act`, so
 * dispatch reads `p.pos_intf.act<&point::set>()(3, 4)` instead of `p.pos_intf.set_act(3, 4)`.
 *
 * @remark Members are paired positionally between the two endpoints, and the pairing is checked at
 * compile time.
 * @remark This is the approach that C++26 static reflection would finish off: `^^point` plus
 * `define_aggregate` can generate the `<member>_act` names that a template can not.
 */
#pragma once

#include <cstddef>
#include <utility>

#include "intrinsic_interface_core.hpp"

namespace untangle {

/**
 * @brief One interface member: the actuator, plus this endpoint's own action.
 *
 * @tparam member Pointer to the method, or to the std::function field, this slot stands for.
 */
template <auto member>
struct member_slot {
  using traits_t = member_signature<decltype(member)>;
  using owner_t = typename traits_t::owner_t;
  using action_t = typename traits_t::action_t;
  using actuator_t = untangle::actuator<action_t>;

  explicit member_slot(owner_t* owner) { action = untangle::action_slot<member>(owner, bound); }

  //! @remark A slot points into itself, so it must be built in place and never relocated.
  member_slot(const member_slot&) = delete;
  member_slot& operator=(const member_slot&) = delete;
  member_slot(member_slot&&) = delete;
  member_slot& operator=(member_slot&&) = delete;

  actuator_t act;              //!< The dispatcher - the ulang `<member>_act`.
  action_t bound;              //!< Holds the binding when the member is a method.
  action_t* action = nullptr;  //!< Points at bound, or at the owner's funcref field.
};

template <auto first, auto...>
struct first_owner {
  using type = typename member_signature<decltype(first)>::owner_t;
};

/**
 * @brief An intrinsic interface over a list of members of the declaring class.
 */
template <auto... members>
struct interface : interface_base {
  using owner_t = typename first_owner<members...>::type;
  using slots_t = std::tuple<member_slot<members>...>;

  //! @remark The slots are built in place - the pack expansion repeats `owner` once per member.
  explicit interface(owner_t* owner) : self(owner), slots((static_cast<void>(members), owner)...) {}

  /**
   * @remark The connections are torn down here, and not in the base destructor, so that the slots
   * are still alive while a peer unwires them.
   */
  ~interface() { drop_links(); }

  owner_t* self = nullptr;
  slots_t slots;

  /**
   * @brief The actuator of one member - ulang `<intf>.<member>_act`.
   */
  template <auto member>
  auto& act() {
    return std::get<member_slot<member>>(slots).act;
  }

  /**
   * @brief The action slot of one member, to hand to a peer's actuator by hand.
   */
  template <auto member>
  auto* action() {
    return std::get<member_slot<member>>(slots).action;
  }

  /**
   * @brief Adds the other endpoint's actions into this endpoint's actuators - `a <intf> b`.
   */
  template <typename other_t>
  void connect_to(other_t& other) {
    static_assert(std::tuple_size_v<slots_t> == std::tuple_size_v<typename other_t::slots_t>,
                  "the two endpoints declare a different number of interface members");
    auto link = std::make_shared<interface_link>();
    link->peer = &other;
    link->unwire = [this, &other]() { unwire(other); };
    wire(other, std::make_index_sequence<sizeof...(members)>{});
    links.push_back({link, true});
    other.links.push_back({link, false});
  }

  /**
   * @brief Removes the other endpoint's actions from this endpoint's actuators - `a >intf< b`.
   */
  template <typename other_t>
  void disconnect_from(other_t& other) {
    unwire(other);
    for (auto& entry : links) {
      if (entry.link->peer == &other) {
        entry.link->alive = false;
      }
    }
    forget_link(&other);
    other.forget_link(this);
  }

 private:
  template <typename other_t, std::size_t... indexes>
  void wire(other_t& other, std::index_sequence<indexes...>) {
    static_assert(
        (std::is_same_v<
             typename std::tuple_element_t<indexes, slots_t>::action_t,
             typename std::tuple_element_t<indexes, typename other_t::slots_t>::action_t> &&
         ...),
        "the interface members of the two endpoints have different signatures");

    ((std::get<indexes>(slots).act.add(std::get<indexes>(other.slots).action),
      parents[std::get<indexes>(other.slots).action] =
          parent_record{other.self, &typeid(typename other_t::owner_t)}),
     ...);
  }

  template <typename other_t>
  void unwire(other_t& other) {
    unwire(other, std::make_index_sequence<sizeof...(members)>{});
  }

  template <typename other_t, std::size_t... indexes>
  void unwire(other_t& other, std::index_sequence<indexes...>) {
    ((std::get<indexes>(slots).act.remove(std::get<indexes>(other.slots).action),
      parents.erase(std::get<indexes>(other.slots).action)),
     ...);
  }
};

}  // namespace untangle
