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
 * @remark Argument convention: an action must not take ownership of the arguments it is
 * invoked with. \ref operator()() forwards one argument pack to every action in the list,
 * and an rvalue can be moved from only once, so an action that consumes an argument leaves
 * the actions after it holding a moved-from object -- an empty std::function for a callback
 * a later action means to call itself. The arguments are forwarded rather than copied so that
 * a single action pays nothing for the broadcast; what that buys has to be respected by the
 * actions.
 *
 * @warning An action does not have to move an argument in its body to consume it: a parameter
 * taken **by value** is move constructed from an rvalue by the call itself. Take arguments by
 * reference or const reference, or invoke with lvalues, whenever more than one action is
 * connected. The behaviour is otherwise unpredictable, and it is the caller and the action
 * signatures together that decide it -- an actuator cannot detect the violation.
 *
 * @remark \ref invoke_action() invokes one single action, so nothing follows it and the
 * convention does not constrain it.
 *
 * @remark Ownership convention: an actuator normally does not own its actions. It stores
 * pointers to std::function objects the caller keeps alive, and those have to outlive it.
 * \ref add(action_t&&) is the exception -- it moves the action into actuator::owned and
 * points at the stored copy -- which is what lets an anonymous lambda be an action, with no
 * named variable to keep around. The handle it returns is the only way to \ref remove()
 * such an action afterwards.
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

  /**
   * @brief Actions this actuator owns, as added by \ref add(action_t&&).
   *
   * @remark It is a std::list because inserting into one never invalidates the address of an
   * element already in it, so every handle already handed out stays valid as further actions
   * are added. A std::vector would reallocate and dangle all of them at once.
   */
  std::list<action_t> owned;

  actuator() = default;
  /**
   * @brief Copy constructor.
   *
   * @remark It can not be defaulted, see \ref copy_from().
   */
  actuator(const actuator& other) { copy_from(other); }
  actuator(actuator&&) noexcept = default;
  ~actuator() = default;
  /**
   * @brief Assignment operator.
   *
   * Example:
   * \snippet actuator_test.cpp test_assignment
   */
  actuator& operator=(const actuator& other) {
    if (this != &other) {
      copy_from(other);
    }
    return *this;
  }
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
   * After this call the actuator is empty: actuator::is_connected() returns false. The
   * actions it owns are destroyed, so every handle returned by \ref add(action_t&&) is
   * dangling afterwards; the actions it merely points at are left untouched.
   */
  void reset() {
    actions.clear();
    actions_map.clear();
    results.clear();
    owned.clear();
  }

  /**
   * @brief Copy another actuator, re-pointing the copied actions at this object's storage.
   *
   * @remark A defaulted copy is wrong as soon as an actuator owns anything: it duplicates
   * actuator::owned but leaves actuator::actions pointing into *other*'s copy of it, so the
   * new object dangles the moment other is destroyed. The owned actions are copied first,
   * then each pointer into other::owned is translated to the matching element here.
   *
   * @remark A pointer to an action the caller owns is external to both actuators and is
   * copied unchanged: it is not in either actuator::owned, so it is not in the translation
   * table and is left as it is.
   *
   * @param other - The actuator to copy. Self copy is the caller's to exclude.
   */
  void copy_from(const actuator& other) {
    owned = other.owned;

    // Old address -> new address, for the owned actions only.
    std::map<const action_t*, action_t*> remap;
    auto src = other.owned.begin();
    auto dst = owned.begin();
    for (; src != other.owned.end(); ++src, ++dst) {
      remap.emplace(&*src, &*dst);
    }
    const auto translate = [&remap](action_t* action) {
      const auto it = remap.find(action);
      return it != remap.end() ? it->second : action;
    };

    actions.clear();
    for (const auto& action : other.actions) {
      actions.push_back(translate(action));
    }
    actions_map.clear();
    for (const auto& entry : other.actions_map) {
      actions_map.emplace(entry.first, translate(entry.second));
    }
    results = other.results;
  }

  /**
   * @brief Destroy an owned action that nothing points at any more.
   *
   * @remark Removing an action only drops a pointer to it. When the actuator owns that
   * action the std::function itself lives in actuator::owned and has to go too, or a loop of
   * \ref add(action_t&&) and \ref remove() grows actuator::owned without bound.
   *
   * @remark The action is kept while anything still refers to it: one handle can be added to
   * both the list and the map, and removing it from one must not leave the other pointing at
   * a destroyed object. An action the actuator does not own is not in actuator::owned, so
   * nothing is found to erase and the caller's object is left alone.
   *
   * @param action - The action just removed. A null pointer is accepted and matches nothing.
   */
  void release_owned(const action_t* action) {
    for (const auto& a : actions) {
      if (a == action) {
        return;
      }
    }
    for (const auto& entry : actions_map) {
      if (entry.second == action) {
        return;
      }
    }
    owned.remove_if([action](const action_t& a) { return &a == action; });
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
   *
   * @warning The same argument pack is forwarded to every action in the list, so the argument
   * convention on \ref actuator applies here in full: an action that consumes an argument
   * leaves the actions after it with a moved-from object.
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
      release_owned(dead_action);
    }
  }

  /**
   * @brief Invokes one single action associated with a key.
   *
   * An action that can not be invoked -- a null pointer, an empty std::function, a binding
   * whose object is gone -- is dropped from actuator::actions_map instead, exactly as
   * \ref operator()() drops one from actuator::actions. Nothing is invoked and nothing
   * escapes; actuator::has_action() reports it gone afterwards.
   *
   * @param name - Key associated with the action. Invoking a key that is not in the map does
   * nothing.
   * @param args - Arguments list must match the action arity. A trailing callback is invoked
   * with the action's return value; see the convention on \ref actuator.
   */
  template <typename... Args>
  void invoke_action(const std::string& name, Args&&... args) {
    results.clear();

    // Copied up front, before the action can move the callback out of the argument pack.
    auto last = last_arg(args...);
    const auto it = actions_map.find(name);
    if (it == actions_map.end()) {
      return;
    }

    // A null pointer or an empty std::function can never be invoked, and dereferencing a
    // null one to try is undefined behaviour before a call is even made. Calling an empty
    // one throws std::bad_function_call, which is not an invalid_action and would escape
    // this method. Drop it instead, as operator()() does for the actions list.
    if (it->second == nullptr || !*it->second) {
      const action_t* dead_action = it->second;
      actions_map.erase(it);
      release_owned(dead_action);
      return;
    }

    try {
      if constexpr (std::is_same_v<typename action_t::result_type, void>) {
        (*it->second)(std::forward<Args>(args)...);
      } else {
        results.push_back((*it->second)(std::forward<Args>(args)...));
        invoke_callback(last);
      }
    } catch (const invalid_action& ia) {
      std::cout << ia.what() << std::endl;
      const action_t* dead_action = it->second;
      actions_map.erase(name);
      release_owned(dead_action);
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
   * @brief Add an action the actuator owns.
   *
   * The action is moved into actuator::owned, so it needs no named variable outliving the
   * actuator: an anonymous lambda can be passed straight in, and the call converts it to
   * action_t. See the ownership convention on \ref actuator.
   *
   * @param action - Action to be added. It is taken by value and moved from.
   * @return action_t* - A handle to the stored action, to pass to \ref remove(). It stays
   * valid until that action is removed or the actuator is destroyed. A copy of the actuator
   * owns its own copy of the action and has its own handle to it.
   *
   * Example:
   * \snippet actuator_test.cpp test_add_anonymous_lambda
   */
  action_t* add(action_t&& action) {
    owned.push_back(std::move(action));
    actions.push_back(&owned.back());
    return &owned.back();
  }

  /**
   * @brief Add action to the actions map associated with a name.
   *
   * @param name - Name of the action.
   * @param action - Action to be added.
   */
  void add(const std::string& name, action_t* action) { actions_map.emplace(name, action); }

  /**
   * @brief Add an action the actuator owns, associated with a name.
   *
   * @param name - Name of the action.
   * @param action - Action to be added. It is taken by value and moved from.
   * @return action_t* - A handle to the stored action, as \ref add(action_t&&) returns, or
   * nullptr when name is already taken. The name already in the map keeps the action it has,
   * as it does for \ref add(const std::string&, action_t*), and the action offered here is
   * dropped rather than left ownerless in actuator::owned.
   */
  action_t* add(const std::string& name, action_t&& action) {
    owned.push_back(std::move(action));
    const auto [it, inserted] = actions_map.emplace(name, &owned.back());
    if (!inserted) {
      owned.pop_back();
      return nullptr;
    }
    return &owned.back();
  }

  /**
   * @brief Remove an action from the actions list.
   *
   * An invalid action (empty std::function) is implicitly removed when operator()() is invoked.
   *
   * An action the actuator owns is destroyed by this call, so the handle to it is dangling
   * afterwards. See \ref release_owned().
   *
   * @param action - Action to be removed. For an owned action this is the handle returned by
   * \ref add(action_t&&).
   *
   * Example:
   * \snippet actuator_test.cpp test_remove
   */
  void remove(const action_t* action) {
    actions.remove_if([&action](const auto& a) { return (action == a); });
    release_owned(action);
  }

  /**
   * @brief Remove an action from actions map.
   *
   * An action the actuator owns is destroyed by this call, so the handle to it is dangling
   * afterwards. See \ref release_owned().
   *
   * @param name -  Name of the action to remove.
   */
  void remove(const std::string& name) {
    const auto it = actions_map.find(name);
    if (it == actions_map.end()) {
      return;
    }
    const action_t* action = it->second;
    actions_map.erase(it);
    release_owned(action);
  }

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
 * @brief Add one \ref connect() argument to the actuator being built.
 *
 * An argument that is an lvalue of the action type is one the caller owns: only its address
 * is stored, and it has to outlive the actuator. Anything else -- an anonymous lambda, a
 * temporary std::function, a named lambda that is not an action_t yet -- has no owner to
 * outlive the actuator, so it is converted to an action and moved into it.
 *
 * @remark Empty actions are dropped rather than stored, which is what \ref connect() has
 * always done with them. Dropping an empty owned action before it is stored is what keeps
 * actuator::owned free of entries nothing points at.
 *
 * @tparam action_t Action type of the actuator being built.
 * @tparam arg_t Deduced type of the argument, carrying its value category.
 * @param target - The actuator being built.
 * @param arg - One \ref connect() argument.
 */
template <typename action_t, typename arg_t>
void connect_one(actuator<action_t>& target, arg_t&& arg) {
  if constexpr (std::is_lvalue_reference_v<arg_t&&> &&
                std::is_same_v<std::decay_t<arg_t>, action_t>) {
    if (arg != nullptr) {
      target.add(&arg);
    }
  } else {
    action_t action(std::forward<arg_t>(arg));
    if (action != nullptr) {
      target.add(std::move(action));
    }
  }
}

/**
 * @brief Creates an actuator holding an initial list of actions.
 *
 * @remark Ownership: an action passed as an lvalue is one the caller owns, and the actuator
 * only points at it -- it must outlive the actuator. An action passed as an rvalue, an
 * anonymous lambda above all, is moved into the actuator instead and needs no named variable
 * at all. The two can be mixed in one call. See the ownership convention on \ref actuator.
 *
 * @warning A call made *only* of anonymous lambdas can not deduce action_t. A lambda has its
 * own closure type and is not a std::function until something converts it, so there is
 * nothing in such a call to deduce the action signature from, and it has to be named:
 *
 * @code
 * auto actuator_rotate = untangle::connect<std::function<void(int)>>(
 *     [](int angle) { ... }, [](int angle) { ... });
 * @endcode
 *
 * @warning Naming the type of the variable the result is assigned to does *not* supply it:
 *
 * @code
 * // still ill formed -- "no matching function for call to connect"
 * untangle::actuator<std::function<void(int)>> actuator_rotate =
 *     untangle::connect([](int angle) { ... });
 * @endcode
 *
 * Template arguments are deduced from the call arguments alone, never from what the returned
 * value is assigned to, so the explicit argument belongs on connect() itself. One named
 * action anywhere in the call deduces action_t for the whole of it, and the anonymous
 * lambdas beside it then need nothing.
 *
 * @param A1 - The first action. It is specified as std::function<...>.
 * @param An - Any number of further actions, each convertible to the type of A1.
 *
 * @return An \ref actuator.
 *
 * @ingroup untangle_functions
 *
 * Example:
 * \snippet actuator_test.cpp test_polymorphism1
 * \snippet actuator_test.cpp test_polymorphism2
 * \snippet actuator_test.cpp test_connect_anonymous_lambda
 */
template <typename action_t, typename... Actions>
actuator<action_t> connect(action_t& A1, Actions&&... An) {
  actuator<action_t> target;
  connect_one(target, A1);
  (connect_one(target, std::forward<Actions>(An)), ...);
  return target;
}

/**
 * @brief Creates an actuator owning an initial list of actions.
 *
 * This overload takes the first action by rvalue, so it is the one that accepts a leading
 * anonymous lambda. std::type_identity_t makes action_t a non-deduced context here, which is
 * what both keeps this overload out of the way of the lvalue one -- it is not a candidate at
 * all unless action_t is named -- and states the requirement described by the warning on the
 * overload taking the first action by lvalue.
 *
 * @param A1 - The first action, as an rvalue.
 * @param An - Any number of further actions, owned or pointed at by value category.
 *
 * @return An \ref actuator.
 *
 * @ingroup untangle_functions
 */
template <typename action_t, typename... Actions>
actuator<action_t> connect(std::type_identity_t<action_t>&& A1, Actions&&... An) {
  actuator<action_t> target;
  connect_one(target, std::move(A1));
  (connect_one(target, std::forward<Actions>(An)), ...);
  return target;
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

/**
 * @brief Add one named \ref connect() argument to the actuator being built.
 *
 * It is \ref connect_one() for the named overloads: the second element of the pair decides
 * ownership. A pointer to an action is one the caller owns and is only stored; anything else
 * -- an anonymous lambda, a temporary std::function -- is converted to an action and moved
 * into the actuator.
 *
 * @remark A null pointer is dropped rather than stored, which is what the named \ref
 * connect() has always done with one. An owned action that is empty is dropped for the same
 * reason the unnamed \ref connect_one() drops one: nothing would ever point at it.
 *
 * @tparam action_t Action type of the actuator being built.
 * @tparam entry_t Deduced type of the pair, carrying its value category.
 * @param target - The actuator being built.
 * @param entry - One name/action pair.
 */
template <typename action_t, typename entry_t>
void connect_named_one(actuator<action_t>& target, entry_t&& entry) {
  if constexpr (std::is_same_v<std::remove_cvref_t<decltype(entry.second)>, action_t*>) {
    if (entry.second != nullptr) {
      target.add(entry.first, entry.second);
    }
  } else {
    action_t action(std::forward<entry_t>(entry).second);
    if (action != nullptr) {
      target.add(entry.first, std::move(action));
    }
  }
}

/**
 * @brief Creates an actuator holding an initial map of named actions.
 *
 * @remark Ownership works as it does for the unnamed connect() overloads: a pair holding a
 * *pointer* names an action the caller owns, and a pair holding an action *by value* -- an
 * anonymous lambda above all -- hands it to the actuator to own. The two can be mixed in one
 * call.
 *
 * @warning The action type is deduced from the pointer in the first pair, so a call whose
 * first pair holds an anonymous lambda has nothing to deduce it from and has to name the
 * signature on connect() itself, exactly as the unnamed overload does:
 *
 * @code
 * auto actuator_rotate = untangle::connect<std::function<void(int)>>(
 *     std::make_pair("triangle", [](int angle) { ... }),
 *     std::make_pair("circle", [](int angle) { ... }));
 * @endcode
 *
 * @remark action_t leads the template parameter list, ahead of key_t, so that naming it --
 * `connect<std::function<...>>(...)` -- means the same thing on every connect() overload.
 * Both parameters are deduced from the first pair whenever it holds a pointer, so nothing
 * has to be named here in the first place.
 *
 * @param A1 - The first name/action pair. Its second element is a pointer to an action.
 * @param An - Any number of further name/action pairs, owned or pointed at according to
 * whether they hold the action itself or a pointer to it.
 *
 * @return An \ref actuator.
 *
 * @ingroup untangle_functions
 *
 * Example:
 * \snippet actuator_test.cpp test_connect_named_anonymous_lambda
 */
template <typename action_t, typename key_t, typename... Actions>
actuator<action_t> connect(std::pair<key_t, action_t*> A1, Actions&&... An) {
  actuator<action_t> target;
  connect_named_one(target, std::move(A1));
  (connect_named_one(target, std::forward<Actions>(An)), ...);
  return target;
}

/**
 * @brief Creates an actuator owning an initial map of named actions.
 *
 * This overload takes a first pair holding the action itself rather than a pointer to it, so
 * it is the one that accepts a leading anonymous lambda. action_t is never deduced here --
 * the requires clause is what keeps it from competing with the overload taking a pointer in
 * the first pair over a leading pointer pair, which both would otherwise accept once action_t
 * is named.
 *
 * @param A1 - The first name/action pair, holding the action itself.
 * @param An - Any number of further name/action pairs.
 *
 * @return An \ref actuator.
 *
 * @ingroup untangle_functions
 */
template <typename action_t, typename key_t, typename value_t, typename... Actions>
  requires(!std::is_same_v<value_t, action_t*>)
actuator<action_t> connect(std::pair<key_t, value_t> A1, Actions&&... An) {
  actuator<action_t> target;
  connect_named_one(target, std::move(A1));
  (connect_named_one(target, std::forward<Actions>(An)), ...);
  return target;
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
