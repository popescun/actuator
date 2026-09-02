/**
 * @brief Interface to \ref untangle::actuator functor.
 *
 * @file actuator.hpp
 * @author Nicolae Popescu
 * @date 2018 - 2025
 */
#pragma once

#include <vector>
#include <list>
#include <map>
#include <utility>
#include <functional>
#include <memory>
#include <iostream>
#include <type_traits>
#include <cassert>
#include <exception>

namespace untangle
{
// exception
/**
 * @brief Invalid action exception.
 *
 * @remark An action may be provided as a binding to a class function member, by using \ref bind().
 *         When the class object gets invalid, invoking the action will raise an exception to this type.
 *
 */
struct invalid_action : private std::exception
{
  /**
   * @brief Construct a new invalid action object.
   *
   * @param text - A message text, describing the reason of this exception.
   */
  explicit invalid_action(std::string  text) : what(std::move(text)){}

  std::string what; //!< It holds the message text.
};

/**
 * @brief An actuator is a functor that can trigger a dynamic list of actions (of type std::function<...>).
 *
 *@remark An actuator object can be constructed with an initial list of actions by \ref connect().
 *
 * @tparam action_t Action type. It is specified as std::function<...>.
 */
template<typename action_t>
struct actuator final
{
  /**
   * @brief Actions container type.
   *
   * @remark The elements stored are of pointer type, that is required to implement the remove() operation.
   * std::function supports only equality operator for nullptr (two std::function(s) can not compare).
   */
  using actions_t = std::list<action_t*>;
  using map_actions_t = std::map<std::string, action_t*>;
  using result_t = std::conditional<std::is_void<typename action_t::result_type>::value, int, typename action_t::result_type>;
  /**
   * @brief Results container type.
   *
   * It holds the return values of the actions that have a non-void return type.
   * Upon the actuator invocation, the returns can be extracted from \ref results.
   */
  using results_t = std::vector<typename result_t::type>;

  actions_t actions; //!< Actions list.
  map_actions_t map_actions; //!< Actions list.
  results_t results; //!< Actions return values list.

  actuator() = default;
  ~actuator()
  {
    actions.clear();
  }

  /**
   * @brief Helper method to access the action type of this object.
   *
   * @note Intended to be used in expressions by `decltype(<actuator instance>.type()`
   *
   * @return action_t
   */
  action_t type()
  {
    return nullptr;
  }

  /**
   * @brief Assignment operator.
   *
   * Example:
   * \snippet test_actuator.cpp test_assignment
   */
  actuator& operator=(const actuator& other)
  {
    if (this == &other)
    {
      return *this;
    }

    actions = other.actions;
    map_actions = other.map_actions;
    results = other.results;
    return *this;
  }

  /**
   * @brief Remove all actions and any stored results.
   *
   * After this call the actuator is empty: actuator::is_connected() returns false.
   */
  void reset()
  {
    actions.clear();
    map_actions.clear();
    results.clear();
  }

  /**
   * @brief The call operator.
   *
   * Actions in the actuator#actions list are triggered by invoking the call operator.
   *
   * @param args - Arguments list must match the action arity.
   */
  template<typename ...Args>
  void operator()(Args&&... args)
  {
    results.clear();

    // Dead bindings are collected here and dropped after the loop.
    // The actuator does not own the actions it points at, so it must never
    // write through action_t* into a std::function belonging to the caller.
    std::vector<action_t*> dead_actions;

    for (const auto& action : actions)
    {
      // A null pointer or an empty std::function can never be invoked. Calling an
      // empty one throws std::bad_function_call, which is not an invalid_action and
      // would escape this operator, so drop it instead of invoking it.
      if (action == nullptr || !*action)
      {
        dead_actions.push_back(action);
        continue;
      }

      try
      {
        // todo: to be removed?
        // with SFINAE
        // select_actuate(action, std::forward<Args>(args)...);

        if constexpr (std::is_same_v<typename action_t::result_type, void>) {
          (*action)(std::forward<Args>(args)...);
        } else {
          results.push_back((*action)(std::forward<Args>(args)...));
        }
      }
      catch (const invalid_action& ia)
      {
        std::cout << ia.what.c_str() << std::endl;
        dead_actions.push_back(action);
      }
    }

    for (const auto& dead_action : dead_actions)
    {
      actions.remove(dead_action);
    }
  }

  /**
   * @brief Invokes one single action associated with a key.
   *
   * @param name - Key associated with the action.
   * @param args - Arguments list must match the action arity.
   */
  template<typename ...Args>
  void invoke_action(std::string name, Args&&... args)
  {
    results.clear();
    const auto& it = map_actions.find(name);
    if (it != map_actions.end())
    {
      try
      {
        select_actuate(it->second, std::forward<Args>(args)...);
      }
      catch (const invalid_action& ia)
      {
        std::cout << ia.what.c_str() << std::endl;
        map_actions.erase(name);
      }
    }
  }

  /**
   * @brief Add an action to the actions list.
   *
   * @param action - Action to be added.
   *
   * Example:
   * \snippet test_actuator.cpp test_add
   */
  void add(action_t* action)
  {
    actions.push_back(action);
  }

  /**
   * @brief Add action to the actions map associated with a name.
   *
   * @param name - Name of the action.
   * @param action - Action to be added.
   */
  void add(std::string name, action_t* action)
  {
    map_actions.emplace(name, action);
  }

  /**
   * @brief Remove an action from the actions list.
   *
   * An invalid action (empty std::function) is implicitly removed when operator()() is invoked.
   *
   * @param action - Action to be removed.
   *
   * Example:
   * \snippet test_actuator.cpp test_add
   */
  void remove(const action_t* action)
  {
    actions.remove_if([&action](const auto& a)
    {
      return (action == a);
    });
  }

  /**
   * @brief Remove an action from actions map.
   *
   * @param name -  Name of the action to remove.
   */
  void remove(const std::string& name)
  {
    map_actions.erase(name);
  }

  /**
   * @brief Check if this actuator is "connected" with other actions.
   *
   * @return true - if the actuator::actions list is not empty.
   * @return false - if the actuator::actions list is empty.
   */
  bool is_connected() { return !actions.empty() || !map_actions.empty(); }

  /**
   * @brief Check if there is certain named action.
   *
   * @param name - Name associated with the action.
   * @return true - if name can be found in actuator::map_actions
   * @return false - if name can not be found in actuator::map_actions
   */
  bool has_action(std::string name) { return map_actions.find(name) != map_actions.end(); }

  private:
   /**
   * @brief SFINAE for void return.
   *
   */
  template<typename T, typename ...Args>
  std::enable_if_t<std::is_void_v<typename T::result_type>, typename T::result_type> select_actuate(T* action, Args&&... args)
  {
     (*action)(std::forward<Args>(args)...);
  }

  /**
   * @brief SFINAE for non-void return.
   *
   */
  template<typename T, typename ...Args>
  std::enable_if_t<!std::is_void_v<typename T::result_type>, typename T::result_type> select_actuate(T* action, Args&&... args)
  {
    results.push_back((*action)(std::forward<Args>(args)...));
    return typename T::result_type();
  }
};

/**
 * @brief Creates an actuator holding an initial list of actions.
 *
 * @param A1..An Any number of actions. They are specified as std::function<...>.
 *
 * @return An \ref actuator.
 *
 * @ingroup untangle_functions
 *
 * Example:
 * \snippet test_actuator.cpp test_polymorphism1
 * \snippet test_actuator.cpp test_polymorphism2
 */
template<typename action_t, typename ...Actions>
auto connect(action_t& A1, Actions&... An)
{
  using actuator_t = untangle::actuator<action_t>;
  actuator_t actuator;
  actuator.actions = {&A1, &An...};

  // remove empty actions
  actuator.actions.remove_if([](const auto& action)
  {
    return (*action == nullptr);
  });
  return std::move(actuator);
}

template <typename actuator_t>
void remove_empty_actions(actuator_t& actuator)
{
  std::vector<typename actuator_t::map_actions_t::const_iterator> removable_iterators;
  for (auto it = actuator.map_actions.begin(); it != actuator.map_actions.end(); ++it)
  {
    if (it->second == nullptr)
    {
      removable_iterators.push_back(it);
    }
  }
  for (auto it: removable_iterators)
  {
    actuator.map_actions.erase(it);
  }
}

template<typename key_t, typename action_t, typename ...Actions>
auto connect(std::pair<key_t, action_t*> A1, Actions... An)
{
  using actuator_t = untangle::actuator<action_t>;
  actuator_t actuator;
  actuator.map_actions = {A1, An...};

  // remove empty actions
  remove_empty_actions(actuator);
  return std::move(actuator);
}

// generic helpers to remove const qualifier from a function type,
// for instance const member functions
template <typename T>
struct function_remove_const;

template <typename R, typename... Args>
struct function_remove_const<R(Args...)>
{
    using type = R(Args...);
};

template <typename R, typename... Args>
struct function_remove_const<R(Args...)const>
{
    using type = R(Args...);
};

/**
 *  @defgroup untangle_functions namespace untangle: functions
 */

/**
 * @brief Binding to a class function member.
 *
 * It returns a std::function(lambda) that wraps the function member. It may be used to provide an action for \ref connect() or \ref actuator::add().
 *
 * @remark It requires a shared pointer to the class type. A std::weak_ptr to it is captured internally in a lambda,
 * so the binding can always check whether the shared object is still alive without keeping it alive itself.
 * Therefore, it is safe to use actions provided by this binding inside an \ref actuator, and safe for the action to
 * outlive the caller's shared pointer.
 *
 * @param obj - Class object.
 * @param method - Pointer to function member. It is specified as &<class type>::<function member>
 * @return action_t - A std::function that wraps the pointer to function member.
 *
 * @remark If the class object gets invalid, invoking this binding will throw an exception of type invalid_action.
 *
 * @ingroup untangle_functions
 */
template <typename class_t, typename T, typename action_t = std::function<typename function_remove_const<T>::type>>
static action_t bind(const std::shared_ptr<class_t>& obj, T class_t::* method)
{
  return [wp = std::weak_ptr<class_t>(obj), method](auto&&... args) -> typename action_t::result_type
  {
    // lock() also keeps the object alive for the duration of the call
    if (const auto obj_ = wp.lock())
    {
      return ((*obj_).*method)(std::forward<decltype(args)>(args)...);
    }
    else
    {
      //inform the actuator about dead binding
      throw invalid_action("bind::method: invalid object");
    }
  };
}

/**
 * @brief Binding to a class method.
 *
 * @attention It is not safe to use this binding to provide actions to an \ref actuator.The class object is provided through a pointer type. This pointer is captured internally in a lambda, so it can not be checked if it gets null.
 *
 * @remark It is provided for convenience of use: within a class it is safe to create bindings through <B>this</B> pointer.
 *
 * @param obj - Pointer to class.
 * @param method - Pointer to function member. It is specified as &<class type>::<function member>
 * @return action_t - A std::function that wraps the pointer to function member.
 *
 * @ingroup untangle_functions
 */
template <typename class_t, typename T, typename action_t = std::function<typename function_remove_const<T>::type>>
static action_t bind(class_t* obj, T class_t::* method)
{
  assert(obj != nullptr);
  return [obj, method](auto&&... args) mutable -> typename action_t::result_type
  {
    return ((obj)->*method)(std::forward<decltype(args)>(args)...);
  };
}

}
