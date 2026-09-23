// Copyright (c) 2018 Nicolae Popescu. MIT License.

/**
 * @brief Interface to \ref untangle::actuator functor.
 */
#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace untangle {
// exception
/**
 * @brief Invalid action exception.
 *
 * @remark An action may be provided as a binding to a class function member, by using \ref
 * untangle::bind(). When the class object gets invalid, invoking the action will raise an exception
 * to this type.
 *
 */
struct invalid_action : std::exception {
  /**
   * @brief Construct a new invalid action object.
   *
   * @param text - A message text, describing the reason of this exception.
   */
  explicit invalid_action(std::string text) : message(std::move(text)) {}

  /**
   * @brief The message text describing the reason of this exception.
   *
   * @return const char* - The message text.
   */
  const char* what() const noexcept override { return message.c_str(); }

 private:
  std::string message;  //!< It holds the message text.
};

/**
 * @brief Get last argument of params pack;
 *
 * @tparam Args params template types
 * @param args params
 * @return last param value in case it exist, otherwise 0.
 */
template <typename... Args>
auto last_arg(Args&&... args) {
  if constexpr (sizeof...(Args) <= 0) {
    return 0;
  } else {
    return std::get<sizeof...(Args) - 1>(std::forward_as_tuple(args...));
  }
}

/**
 * @brief An actuator is a functor that can trigger a dynamic list of actions (of type
 * std::function<...>).
 *
 *@remark An actuator object can be constructed with an initial list of actions by \ref connect().
 *
 * @remark Callback convention: when the last argument of an invocation can be called with the
 * action return type R and returns nothing, it is not passed on as a plain argument alone --
 * it is also treated as a completion callback. The action is invoked with the full argument
 * list as usual, and the callback is then invoked with the action return value. Any void
 * returning callable qualifies: a std::function<void(R)>, a lambda taking R, a function
 * pointer. It applies to \ref operator()() and to \ref invoke_action() alike. Actions
 * returning void have no result to report, so no callback is invoked for them.
 *
 * @warning The return type is what separates a callback from an argument the action means to
 * consume itself. A trailing callable that returns a value -- a transform, a comparator --
 * is left alone, and by the same rule a callback written to return something is silently
 * not invoked. A callback returns nothing.
 *
 * @tparam action_t Action type. It is specified as std::function<...>.
 */
template <typename action_t>
struct actuator final {
  /**
   * @brief Actions container type.
   *
   * @remark The elements stored are of pointer type, that is required to implement the remove()
   * operation. std::function supports only equality operator for nullptr (two std::function(s) can
   * not compare).
   */
  using actions_t = std::list<action_t*>;
  using actions_map_t = std::map<std::string, action_t*>;
  using result_t = std::conditional<std::is_void<typename action_t::result_type>::value, int,
                                    typename action_t::result_type>;
  /**
   * @brief Results container type.
   *
   * It holds the return values of the actions that have a non-void return type.
   * Upon the actuator invocation, the returns can be extracted from \ref results.
   */
  using results_t = std::vector<typename result_t::type>;

  actions_t actions;          //!< Actions list.
  actions_map_t actions_map;  //!< Named actions map.
  results_t results;          //!< Actions return values list.

  actuator() = default;
  actuator(const actuator&) = default;
  actuator(actuator&&) noexcept = default;
  ~actuator() = default;
  /**
   * @brief Assignment operator.
   *
   * Example:
   * \snippet actuator_test.cpp test_assignment
   */
  actuator& operator=(const actuator& other) = default;
  actuator& operator=(actuator&&) noexcept = default;

  /**
   * @brief Helper method to access the action type of this object.
   *
   * @note Intended to be used in expressions by `decltype(<actuator instance>.type()`
   *
   * @return action_t
   */
  action_t type() const { return nullptr; }

  /**
   * @brief Remove all actions and any stored results.
   *
   * After this call the actuator is empty: actuator::is_connected() returns false.
   */
  void reset() {
    actions.clear();
    actions_map.clear();
    results.clear();
  }

  /**
   * @brief Invoke the trailing callback argument, if the invocation has one.
   *
   * Does nothing unless last_t can be called with the action return type and returns nothing,
   * so any other trailing argument is left alone. The test is on what the type can do, not on
   * what it is, so a plain lambda qualifies as well as a std::function. See the callback
   * convention on \ref actuator.
   *
   * @remark The single condition does the work of two tests: (1) whether last can be called
   * with the action return type at all, and (2) whether that call returns nothing. Spelled
   * out as traits they need two nested `if constexpr`:
   *
   * @code
   * // (1)
   * if constexpr (std::is_invocable_v<last_t&, typename result_t::type>) {
   *   // (2)
   *   if constexpr (std::is_void_v<std::invoke_result_t<last_t&, typename result_t::type>>) {
   *     last(results.back());
   *   }
   * }
   * @endcode
   *
   * Both are folded into the one expression instead, each in a different part of it:
   *
   * @code
   * requires { requires std::is_void_v<decltype( last(results.back()) )>; }
   * //         \________ (2) ________/           \_____ (1) _____/
   * @endcode
   *
   * @remark (1) Invocability comes for free. To form decltype(last(results.back())) the
   * compiler must first form the call last(...). If last is an int, that expression does not
   * exist -- and inside a requires-expression a substitution failure yields false instead of
   * being ill formed, the same property concepts are built on. So invocability is never
   * tested separately: it is a precondition of the return type question, and failing it is
   * what makes the whole condition false.
   *
   * @remark (2) The return type test is the inner `requires`, and the doubled keyword is not
   * a typo. `requires { expr; }` asks only whether expr is well formed, and
   * std::is_void_v<...> is well formed when it is false too -- that spelling would let a
   * value returning callable through. The second keyword makes it a nested requirement: a
   * condition that has to hold, not merely compile.
   *
   * @remark The trait spelling has to nest, which is part of why it is not the one used here:
   * the two `if constexpr` cannot be joined with `&&`. `&&` short circuits evaluation, not
   * instantiation, so std::invoke_result_t is formed whatever the first trait says, and for a
   * type that is not invocable at all it has no ::type -- a hard error rather than a false
   * result.
   *
   * @remark The caller must copy the callback out of the argument pack *before* invoking the
   * action: an action taking it by value moves from the caller's std::function, which would
   * leave this an empty function to call.
   *
   * @tparam last_t Type of the last argument, as returned by \ref last_arg().
   * @param last - Last argument value, copied before the action was invoked.
   */
  template <typename last_t>
  void invoke_callback(last_t& last) {
    if constexpr (requires { requires std::is_void_v<decltype(last(results.back()))>; }) {
      last(results.back());
    }
  }

  /**
   * @brief The call operator.
   *
   * Actions in the actuator#actions list are triggered by invoking the call operator.
   *
   * @param args - Arguments list must match the action arity. A trailing callback is invoked
   * once per action, with that action's return value; see the convention on \ref actuator.
   */
  template <typename... Args>
  void operator()(Args&&... args) {
    results.clear();

    // Copied up front, before any action can move the callback out of the argument pack.
    auto last = last_arg(args...);

    // Dead bindings are collected here and dropped after the loop.
    // The actuator does not own the actions it points at, so it must never
    // write through action_t* into a std::function belonging to the caller.
    std::vector<action_t*> dead_actions;

    for (const auto& action : actions) {
      // A null pointer or an empty std::function can never be invoked. Calling an
      // empty one throws std::bad_function_call, which is not an invalid_action and
      // would escape this operator, so drop it instead of invoking it.
      if (action == nullptr || !*action) {
        dead_actions.push_back(action);
        continue;
      }

      try {
        if constexpr (std::is_same_v<typename action_t::result_type, void>) {
          (*action)(std::forward<Args>(args)...);
        } else {
          results.push_back((*action)(std::forward<Args>(args)...));
          invoke_callback(last);
        }
      } catch (const invalid_action& ia) {
        std::cout << ia.what() << std::endl;
        dead_actions.push_back(action);
      }
    }

    for (const auto& dead_action : dead_actions) {
      actions.remove(dead_action);
    }
  }

  /**
   * @brief Invokes one single action associated with a key.
   *
   * @param name - Key associated with the action.
   * @param args - Arguments list must match the action arity. A trailing callback is invoked
   * with the action's return value; see the convention on \ref actuator.
   */
  template <typename... Args>
  void invoke_action(const std::string& name, Args&&... args) {
    results.clear();

    // Copied up front, before the action can move the callback out of the argument pack.
    auto last = last_arg(args...);
    const auto& it = actions_map.find(name);
    if (it != actions_map.end()) {
      try {
        if constexpr (std::is_same_v<typename action_t::result_type, void>) {
          (*it->second)(std::forward<Args>(args)...);
        } else {
          results.push_back((*it->second)(std::forward<Args>(args)...));
          invoke_callback(last);
        }
      } catch (const invalid_action& ia) {
        std::cout << ia.what() << std::endl;
        actions_map.erase(name);
      }
    }
  }

  /**
   * @brief Add an action to the actions list.
   *
   * @param action - Action to be added.
   *
   * Example:
   * \snippet actuator_test.cpp test_add
   */
  void add(action_t* action) { actions.push_back(action); }

  /**
   * @brief Add action to the actions map associated with a name.
   *
   * @param name - Name of the action.
   * @param action - Action to be added.
   */
  void add(const std::string& name, action_t* action) { actions_map.emplace(name, action); }

  /**
   * @brief Remove an action from the actions list.
   *
   * An invalid action (empty std::function) is implicitly removed when operator()() is invoked.
   *
   * @param action - Action to be removed.
   *
   * Example:
   * \snippet actuator_test.cpp test_remove
   */
  void remove(const action_t* action) {
    actions.remove_if([&action](const auto& a) { return (action == a); });
  }

  /**
   * @brief Remove an action from actions map.
   *
   * @param name -  Name of the action to remove.
   */
  void remove(const std::string& name) { actions_map.erase(name); }

  /**
   * @brief Check if this actuator is "connected" with other actions.
   *
   * @return true - if the actuator::actions list is not empty.
   * @return false - if the actuator::actions list is empty.
   */
  bool is_connected() const { return !actions.empty() || !actions_map.empty(); }

  /**
   * @brief Check if there is certain named action.
   *
   * @param name - Name associated with the action.
   * @return true - if name can be found in actuator::actions_map
   * @return false - if name can not be found in actuator::actions_map
   */
  bool has_action(const std::string& name) const {
    return actions_map.find(name) != actions_map.end();
  }
};

/**
 * @brief Creates an actuator holding an initial list of actions.
 *
 * @param A1 - The first action. It is specified as std::function<...>.
 * @param An - Any number of further actions, of the same type as A1.
 *
 * @return An \ref actuator.
 *
 * @ingroup untangle_functions
 *
 * Example:
 * \snippet actuator_test.cpp test_polymorphism1
 * \snippet actuator_test.cpp test_polymorphism2
 */
template <typename action_t, typename... Actions>
actuator<action_t> connect(action_t& A1, Actions&... An) {
  using actuator_t = untangle::actuator<action_t>;
  actuator_t actuator;
  actuator.actions = {&A1, &An...};

  // remove empty actions
  actuator.actions.remove_if([](const auto& action) { return (*action == nullptr); });
  return actuator;
}

template <typename actuator_t>
void remove_empty_actions(actuator_t& actuator) {
  std::vector<typename actuator_t::actions_map_t::const_iterator> removable_iterators;
  for (auto it = actuator.actions_map.begin(); it != actuator.actions_map.end(); ++it) {
    if (it->second == nullptr) {
      removable_iterators.push_back(it);
    }
  }
  for (auto it : removable_iterators) {
    actuator.actions_map.erase(it);
  }
}

template <typename key_t, typename action_t, typename... Actions>
actuator<action_t> connect(std::pair<key_t, action_t*> A1, Actions... An) {
  using actuator_t = untangle::actuator<action_t>;
  actuator_t actuator;
  actuator.actions_map = {A1, An...};

  // remove empty actions
  remove_empty_actions(actuator);
  return actuator;
}

// generic helpers to remove const qualifier from a function type,
// for instance const member functions
template <typename T>
struct function_remove_const;

template <typename R, typename... Args>
struct function_remove_const<R(Args...)> {
  using type = R(Args...);
};

template <typename R, typename... Args>
struct function_remove_const<R(Args...) const> {
  using type = R(Args...);
};

/**
 *  @defgroup untangle_functions namespace untangle: functions
 */

/**
 * @brief Binding to a class function member.
 *
 * It returns a std::function(lambda) that wraps the function member. It may be used to provide an
 * action for \ref connect() or \ref actuator::add().
 *
 * @remark It requires a shared pointer to the class type. A std::weak_ptr to it is captured
 * internally in a lambda, so the binding can always check whether the shared object is still alive
 * without keeping it alive itself. Therefore, it is safe to use actions provided by this binding
 * inside an \ref actuator, and safe for the action to outlive the caller's shared pointer.
 *
 * @param obj - Class object.
 * @param method - Pointer to function member. It is specified as &\<class type\>::\<function
 * member\>
 * @return action_t - A std::function that wraps the pointer to function member.
 *
 * @remark If the class object gets invalid, invoking this binding will throw an exception of type
 * invalid_action.
 *
 * @ingroup untangle_functions
 */
template <typename class_t, typename T,
          typename action_t = std::function<typename function_remove_const<T>::type>>
action_t bind(const std::shared_ptr<class_t>& obj, T class_t::* method) {
  return [wp = std::weak_ptr<class_t>(obj), method](auto&&... args) ->
         typename action_t::result_type {
           // lock() also keeps the object alive for the duration of the call
           const auto obj_ = wp.lock();
           if (!obj_) {
             // inform the actuator about dead binding
             throw invalid_action("bind: invalid object");
           }
           return ((*obj_).*method)(std::forward<decltype(args)>(args)...);
         };
}

/**
 * @brief Binding to a class method.
 *
 * @attention It is not safe to use this binding when the pointed-to object may be destroyed. A null
 * pointer is detected and reported as a dead action, but a pointer to an already destroyed object
 * can not be distinguished from a valid one. Prefer the overload taking a std::shared_ptr.
 *
 * @remark It is provided for convenience of use: within a class it is safe to create bindings
 * through <B>this</B> pointer.
 *
 * @remark Binding a null pointer produces an action that is always dead: invoking it throws
 * invalid_action, and an \ref actuator drops it.
 *
 * @param obj - Pointer to class.
 * @param method - Pointer to function member. It is specified as &\<class type\>::\<function
 * member\>
 * @return action_t - A std::function that wraps the pointer to function member.
 *
 * @ingroup untangle_functions
 */
template <typename class_t, typename T,
          typename action_t = std::function<typename function_remove_const<T>::type>>
action_t bind(class_t* obj, T class_t::* method) {
  return [obj, method](auto&&... args) -> typename action_t::result_type {
    if (!obj) {
      // inform the actuator about dead binding
      throw invalid_action("bind: invalid object");
    }
    return ((obj)->*method)(std::forward<decltype(args)>(args)...);
  };
}

}  // namespace untangle
