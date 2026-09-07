// Copyright (c) 2026 Nicolae Popescu. MIT License.

/**
 * @brief Approach B: self registering slots, with the signature spelled out per member.
 *
 * @code
 * struct point {
 *   void set(int nx, int ny);
 *   int get_x() const;
 *
 *   INTRINSIC_INTERFACE_BEGIN(point, pos_intf)
 *     INTRINSIC_SLOT(set, void, int, int)   // INTERFACE(name, R, P1, P2), as sketched
 *     INTRINSIC_SLOT(get_x, int)
 *   INTRINSIC_INTERFACE_END(pos_intf)
 * };
 * ENABLE_CONNECT_OPERATOR(pos_intf);
 * @endcode
 *
 * Each slot is an object that registers itself with the interface it is declared in, so there is no
 * member list to iterate over in the preprocessor and no limit on the number of members. Connecting
 * pairs the slots of the two endpoints by name, and checks the action types match.
 *
 * @remark The declared signature is checked against the member by a static_assert, so a wrong `R`
 * or `P1` is a compile error, not a surprise at run time.
 * @remark Unlike approach A this one needs no untangle::enable_interfaces base, but it does repeat
 * the class name once, and each member spells out its signature.
 * @remark Endpoints are matched at run time, which is what allows an interface to be connected
 * across a module boundary where the peer type is not visible.
 */
#pragma once

#include <stdexcept>
#include <string>

#include "intrinsic_interface_core.hpp"

namespace untangle {

struct interface_registry;

/**
 * @brief The type erased view of an interface slot, used to pair the slots of two endpoints.
 */
struct slot_base {
  slot_base() = default;
  slot_base(const slot_base&) = delete;
  slot_base& operator=(const slot_base&) = delete;
  virtual ~slot_base() = default;

  virtual const char* slot_name() const = 0;
  virtual const std::type_info& action_type() const = 0;
  //! @brief The action slot of this endpoint, to be added into a peer's actuator.
  virtual void* action_ptr() = 0;
  virtual void add_action(void* action) = 0;
  virtual void remove_action(void* action) = 0;
};

/**
 * @brief The interface object: connection bookkeeping plus the slots declared in it.
 */
struct interface_registry : interface_base {
  void* owner = nullptr;                       //!< The declaring instance.
  const std::type_info* owner_type = nullptr;  //!< Its type, published to peers as `.parent`.
  std::vector<slot_base*> slots;               //!< Filled in by the slots themselves.

  interface_registry(void* owner_object, const std::type_info* type)
      : owner(owner_object), owner_type(type) {}

  /**
   * @remark The connections are torn down here, and not in the base destructor, so that the slots
   * of this interface are still alive while a peer unwires them.
   */
  ~interface_registry() { drop_links(); }

  slot_base* find(const std::string& name) const {
    for (auto* slot : slots) {
      if (name == slot->slot_name()) {
        return slot;
      }
    }
    return nullptr;
  }

  /**
   * @brief Adds the other endpoint's actions into this endpoint's actuators - `a <intf> b`.
   */
  void connect_to(interface_registry& other) {
    auto link = std::make_shared<interface_link>();
    link->peer = &other;
    link->unwire = [this, &other]() { unwire(other); };
    wire(other);
    links.push_back({link, true});
    other.links.push_back({link, false});
  }

  /**
   * @brief Removes the other endpoint's actions from this endpoint's actuators - `a >intf< b`.
   */
  void disconnect_from(interface_registry& other) {
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
  void wire(interface_registry& other) {
    for (auto* slot : slots) {
      auto* peer = matching_slot(other, *slot);
      void* action = peer->action_ptr();
      slot->add_action(action);
      parents[action] = parent_record{other.owner, other.owner_type};
    }
  }

  void unwire(interface_registry& other) {
    for (auto* slot : slots) {
      auto* peer = other.find(slot->slot_name());
      if (peer == nullptr) {
        continue;
      }
      void* action = peer->action_ptr();
      slot->remove_action(action);
      parents.erase(action);
    }
  }

  static slot_base* matching_slot(interface_registry& other, const slot_base& slot) {
    auto* peer = other.find(slot.slot_name());
    if (peer == nullptr) {
      throw std::runtime_error(std::string("connect: the other endpoint has no member '") +
                               slot.slot_name() + "'");
    }
    if (peer->action_type() != slot.action_type()) {
      throw std::runtime_error(std::string("connect: member '") + slot.slot_name() +
                               "' has a different signature on the other endpoint");
    }
    return peer;
  }
};

/**
 * @brief One interface member: the actuator, plus this endpoint's own action.
 *
 * @tparam signature_t The member signature, as `R(P1, P2)`.
 */
template <typename signature_t>
struct interface_slot;

template <typename R, typename... Args>
struct interface_slot<R(Args...)> final : slot_base {
  using action_t = std::function<R(Args...)>;
  using actuator_t = untangle::actuator<action_t>;

  /**
   * @param registry - The interface this slot belongs to. The slot adds itself to it.
   * @param name - The member name. Two endpoints are paired by it.
   * @param member - Pointer to the method, or to the std::function field, this slot stands for.
   */
  template <typename member_t>
  interface_slot(interface_registry& registry, const char* name, member_t member) : name_(name) {
    using traits_t = member_signature<member_t>;
    static_assert(std::is_same_v<typename traits_t::action_t, action_t>,
                  "the signature declared for this interface member does not match the member");

    action_ =
        action_slot_of(static_cast<typename traits_t::owner_t*>(registry.owner), member, bound_);
    registry.slots.push_back(this);
  }

  actuator_t act;  //!< The dispatcher. The forwarders below are the ulang surface over it.

  //! @brief Fires every connected action - ulang `<intf>.<member>_act(args)`.
  template <typename... call_args_t>
  void operator()(call_args_t&&... args) {
    act(std::forward<call_args_t>(args)...);
  }

  void add(action_t* action) { act.add(action); }
  void remove(const action_t* action) { act.remove(action); }
  bool is_connected() const { return act.is_connected(); }
  const typename actuator_t::actions_t& actions() const { return act.actions; }
  const typename actuator_t::results_t& results() const { return act.results; }

  const char* slot_name() const override { return name_; }
  const std::type_info& action_type() const override { return typeid(action_t); }
  void* action_ptr() override { return action_; }
  void add_action(void* action) override { act.add(static_cast<action_t*>(action)); }
  void remove_action(void* action) override { act.remove(static_cast<action_t*>(action)); }

 private:
  const char* name_ = nullptr;
  action_t bound_;              //!< Holds the binding when the member is a method.
  action_t* action_ = nullptr;  //!< Points at bound_, or at the owner's funcref field.
};

}  // namespace untangle

/**
 * @brief Opens an intrinsic interface declaration inside a class.
 *
 * @param owner_class - The declaring class. This approach does not use a CRTP base, so the class
 * name is named here instead.
 * @param intf_name - Interface name, also the name of the generated member object.
 */
#define INTRINSIC_INTERFACE_BEGIN(owner_class, intf_name) \
  struct intf_name##_t : untangle::interface_registry {   \
    using owner_t = owner_class;                          \
    explicit intf_name##_t(owner_t* owner)                \
        : untangle::interface_registry(owner, &typeid(owner_t)) {}

/**
 * @brief Declares one interface member, with its signature.
 *
 * @param member - A method, or a std::function data member, of the declaring class.
 * @param ret - Return type.
 * @param ... - Parameter types.
 */
#define INTRINSIC_SLOT(member, ret, ...) \
  untangle::interface_slot<ret(__VA_ARGS__)> member##_act{*this, #member, &owner_t::member};

/**
 * @brief Closes an intrinsic interface declaration.
 */
#define INTRINSIC_INTERFACE_END(intf_name) \
  }                                        \
  intf_name{this};
