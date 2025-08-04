/**
 * @brief Interface to \ref untangle::actuator function object.
 *
 * @file connect.h
 * @author Nicu Popescu
 * @date 2018 - 2021
 */
#pragma once

#include <utility>
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
 *         When the class object gets invalid, invoking the action will raise an exception of this type.
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
 * @brief An actuator is a function object that can trigger a dynamic list of actions (of type std::function<...>).
 *
 *@remark An actuator object can be constructed with an initial list of actions by \ref connect().
 *
 * @tparam actionT Action type. It is specified as std::function<...>.
 */
template<typename actionT>
struct actuator final
{
  /**
   * @brief Actions container type.
   *
   * @remark The elements stored are of pointer type, that is required to implement the remove() operation.
   * std::function supports only equality operator for nullptr (two std::function(s) can not compare).
   */
  using actionsT = std::list<actionT*>;
  using mapActionsT = std::map<std::string, actionT*>;
  using resultT = std::conditional<std::is_void<typename actionT::result_type>::value, int, typename actionT::result_type>;
  /**
   * @brief Results container type.
   *
   * It holds the return values of the actions that have a non void return type.
   * Upon the actuator invocation, the returns can be extracted from \ref results.
   */
  using resultsT = std::vector<typename resultT::type>;

  actionsT actions; //!< Actions list.
  mapActionsT mapActions; //!< Actions list.
  resultsT results; //!< Actions return values list.

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
   * @return actionT
   */
  actionT type()
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
    actions.clear();
    mapActions.clear();
    actions = other.actions;
    mapActions = other.mapActions;
    return *this;
  }

  void reset()
  {
    actions.reset();
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
    for (const auto& action : actions)
    {
      if (action)
      {
        try
        {
          select_actuate(action, std::forward<Args>(args)...);
        }
        catch (const invalid_action& ia)
        {
          std::cout << ia.what.c_str() << std::endl;
          *action = nullptr;
        }
      }
    }
    actions.remove_if([](const auto& action)
    {
      return (*action == nullptr);
    });
  }

  /**
   * @brief Invokes one single action associated with a key.
   *
   * @param name - Key associated with the action.
   * @param args - Arguments list must match the action arity.
   */
  template<typename ...Args>
  void invokeAction(std::string name, Args&&... args)
  {
    results.clear();
    const auto& it = mapActions.find(name);
    if (it != mapActions.end())
    {
      try
      {
        select_actuate(it->second, std::forward<Args>(args)...);
      }
      catch (const invalid_action& ia)
      {
        std::cout << ia.what.c_str() << std::endl;
        mapActions.erase(name);
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
  void add(actionT* action)
  {
    actions.push_back(action);
  }

  /**
   * @brief Add action to the actions map associated with a name.
   *
   * @param name - Name of the action.
   * @param action - Action to be added.
   */
  void add(std::string name, actionT* action)
  {
    mapActions.emplace(name, action);
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
  void remove(const actionT* action)
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
    mapActions.erase(name);
  }

  /**
   * @brief Check if this actuator is "connected" with other actions.
   *
   * @return true - if the actuator::actions list is not empty.
   * @return false - if the actuator::actions list is empty.
   */
  bool is_connected() { return !actions.empty() || !mapActions.empty(); }

  /**
   * @brief Check if there is certain named action.
   *
   * @param name - Name associated with the action.
   * @return true - if name can be found in actuator::mapActions
   * @return false - if name can not be found in actuator::mapActions
   */
  bool has_action(std::string name) { return mapActions.find(name) != mapActions.end() ? true : false; }

  private:
   /**
   * @brief SFINAE for void return.
   *
   */
  template<typename T, typename ...Args>
  typename std::enable_if_t<std::is_void<typename T::result_type>::value, typename T::result_type> select_actuate(T* action, Args&&... args)
  {
     (*action)(std::forward<Args>(args)...);
  }

  /**
   * @brief SFINAE for non void return.
   *
   */
  template<typename T, typename ...Args>
  typename std::enable_if_t<!std::is_void<typename T::result_type>::value, typename T::result_type> select_actuate(T* action, Args&&... args)
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
template<typename actionT, typename ...Actions>
auto connect(actionT& A1, Actions&... An)
{
  using actuatorT = untangle::actuator<actionT>;
  actuatorT actuator;
  actuator.actions = {&A1, &An...};

  // remove empty actions
  actuator.actions.remove_if([](const auto& action)
  {
    return (*action == nullptr);
  });
  return std::move(actuator);
}

template <typename actuatorT>
void removeEmptyActions(actuatorT& actuator)
{
  std::vector<typename actuatorT::mapActionsT::const_iterator> removableIterators;
  for (auto it = actuator.mapActions.begin(); it != actuator.mapActions.end(); ++it)
  {
    if (it->second == nullptr)
    {
      removableIterators.push_back(it);
    }
  }
  for (auto it: removableIterators)
  {
    actuator.mapActions.erase(it);
  }
}

template<typename keyT, typename actionT, typename ...Actions>
auto connect(std::pair<keyT, actionT*> A1, Actions... An)
{
  using actuatorT = untangle::actuator<actionT>;
  actuatorT actuator;
  actuator.mapActions = {A1, An...};

  // remove empty actions
  removeEmptyActions(actuator);
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
 * It returns an std::function(lambda) that wraps the function member. It may be used to provide an action for \ref connect() or \ref actuator::add().
 *
 * @remark It requires a shared pointer to the class type. This shared pointer is captured internally in a lambda, and it can be checked if the shared object is valid.
 * Therefore it is safe to use actions provided by this binding inside an \ref actuator.
 *
 * @param obj - Class object.
 * @param method - Pointer to function member. It is specified as &<class type>::<function member>
 * @return actionT - A std::function that wraps the pointer to function member.
 *
 * @remark If the class object gets invalid, invoking this binding will throw an exception of type invalid_action.
 *
 * @ingroup untangle_functions
 */
template <typename classT, typename T, typename actionT = std::function<typename function_remove_const<T>::type>>
static actionT bind(const std::shared_ptr<classT>& obj, T classT::* method)
{
  return [&obj, method](auto&&... args) mutable -> typename actionT::result_type
  {
    if (obj)
    {
      return ((*obj).*method)(std::forward<decltype(args)>(args)...);
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
 * @remark It is provided for conveniency of use: within a class it is safe to create bindings through <B>this</B> pointer.
 *
 * @param obj - Pointer to class.
 * @param method - Pointer to function member. It is specified as &<class type>::<function member>
 * @return actionT - A std::function that wraps the pointer to function member.
 *
 * @ingroup untangle_functions
 */
template <typename classT, typename T, typename actionT = std::function<typename function_remove_const<T>::type>>
static actionT bind(classT* obj, T classT::* method)
{
  assert(obj != nullptr);
  return [obj, method](auto&&... args) mutable -> typename actionT::result_type
  {
    return ((obj)->*method)(std::forward<decltype(args)>(args)...);
  };
}

}
