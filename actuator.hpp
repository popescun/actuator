// Copyright (c) 2018 Nicolae Popescu. MIT License.

/**
 * @brief Interface to \ref untangle::actuator functor.
 */
#pragma once

#include <cstddef>
#include <exception>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
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
 * @brief Can \p callback_t be called as the completion callback of a task returning \p result_t?
 *
 * A task's callback is required, and it is the **last argument** given to `bind_task()` rather
 * than a trailing argument recognised by its type. So the question this asks is not *whether* the
 * argument is a callback -- it is one, by position -- but only whether it can serve as one.
 *
 * @remark That is the whole difference from an action's callback convention, which asks the
 * *whether* and is allowed to answer no: a trailing argument that is not a callback is simply an
 * argument, and nothing is invoked. A task has no such answer to fall through to, which is what
 * lets the callback be required. See the callback convention on the actuator itself, and the
 * warning that comes with it.
 *
 * @remark A task returning nothing still reports that it finished, so for \p result_t of void the
 * callback takes no argument at all. The disjunct that asks to be handed a result yields **false**
 * there rather than failing hard -- forming `void&` in a requires-parameter list is a substitution
 * failure in the immediate context -- which leaves the void disjunct to decide. Its
 * `std::is_void_v` guard is not decoration either: without it a `std::function<void()>` would
 * satisfy this for *any* result type, and a task's result would go unreported.
 *
 * @tparam callback_t Type of the callback.
 * @tparam result_t Result type of the task it reports on.
 */
template <typename callback_t, typename result_t>
concept task_callback_for =
    requires(callback_t& c, result_t& r) { requires std::is_void_v<decltype(c(r))>; } ||
    (std::is_void_v<result_t> &&
     requires(callback_t& c) { requires std::is_void_v<decltype(c())>; });

/**
 * @brief The callback type a task returning \p result_t needs.
 *
 * The caller writes the callback, so a task has to say what shape it must have.
 *
 * @remark A specialisation rather than a std::conditional_t, which cannot serve here:
 * conditional_t forms **both** branches before choosing one, and `std::function<void(result_t)>`
 * with \p result_t of void is ill formed -- a parameter of type void cannot be produced by
 * substitution, however legal `void(void)` is as literal syntax. A specialisation never forms the
 * branch it does not take.
 *
 * @tparam result_t Result type of the task.
 */
template <typename result_t>
struct task_callback_type {
  using type = std::function<void(result_t)>;  //!< Handed the result the task produced.
};

/**
 * @brief The callback of a task that produces no result: it reports only that the task finished.
 */
template <>
struct task_callback_type<void> {
  using type = std::function<void()>;
};

/**
 * @brief An action bound to its arguments, and to the callback it notifies once it has finished.
 *
 * What \ref actuator::add_task() holds and `actuator::call_tasks()` fires. One is built by
 * \ref bind_task(), which binds the arguments into \ref task::call and takes \ref task::callback
 * off the end of the same pack.
 *
 * @remark **An action is a subscription; a task is a one-shot.** An action lives across
 * invocations and is fired with a pack the caller supplies each time. A task carries its own
 * arguments, is fired once, and reports to its own callback. That is why a task is held by value
 * rather than by pointer, and why nothing removes one -- call_tasks() consumes it.
 *
 * @remark The argument types do not appear here. \ref bind_task() binds them into \ref task::call,
 * whose signature is nullary, so **one container holds tasks built from completely different
 * argument packs** -- which is what lets a queue of them exist at all.
 *
 * @tparam result_t Result type of the action the task was built from.
 */
template <typename result_t>
struct task {
  //! What a task of this result type notifies. A void task says finished and nothing else.
  using callback_t = typename task_callback_type<result_t>::type;

  //! What \ref task::call returns, as an actuator reads it off an action type.
  using result_type = result_t;

  /**
   * @brief Runs the action, with the arguments bound into it.
   *
   * @remark It does not notify. `actuator::call_tasks()` does, once this has returned -- so a
   * task that throws never reports, because there is no result and no completion to report.
   */
  result_type operator()() { return call(); }

  /**
   * @brief Is there anything to run?
   *
   * @remark actuator::operator()() tests an action before invoking it, and a task has to answer
   * that test the way a std::function does.
   */
  explicit operator bool() const { return static_cast<bool>(call); }

  std::function<result_type(void)> call;  //!< The action, with its arguments already bound.
  callback_t callback;                    //!< Notified with the result, once \ref call returned.
};

/**
 * @brief Binds an action to its arguments and to the callback it must notify, which comes last.
 *
 * What builds a \ref task. The arguments are bound into task::call, and the callback is taken
 * the end of the same pack. A task built here runs later than it was built -- that is what a queue
 * of them is for -- so the arguments are **copied** now, while the caller still holds them, and
 * handed to the action as the task's own lvalues when it runs.
 *
 * @remark **The callback is the last argument, by position.** It cannot be a parameter of its own:
 * a parameter pack cannot be followed by another parameter and still be deduced, so it arrives
 * inside \p args and is split off here. That split is paid once -- every caller above forwards
 * (action, args...) and never learns of it.
 *
 * @remark It is **not** a trailing argument recognised by its type, which is what an action's
 * callback convention does; see the convention on \ref actuator. Here the last argument *is* the
 * callback, and \ref task_callback_for only checks that it can serve as one. There is no answer
 * "no" to fall through to, which is what lets a task's callback be required.
 *
 * @remark It is **not** forwarded to the action either. The action is invoked with \p args minus
 * its last element, so an action needs no callback parameter of its own -- and a void action,
 * which could never have used one, is a task like any other.
 *
 * @attention Each argument is copied, so a move-only one does not compile. The task outlives the
 * call that built it and cannot borrow what the caller may already have destroyed.
 *
 * @tparam action_t Type of the action. Any callable naming a result_type will do; it need not be a
 * std::function.
 * @tparam Args Types of the arguments to bind, of which the last is the callback.
 * @param action - The action to bind.
 * @param args - The arguments to bind to \p action, followed by the callback to notify.
 *
 * @return The task, ready to be held by \ref actuator::add_task() and fired by
 * actuator::call_tasks().
 */
template <typename action_t, typename... Args>
auto bind_task(action_t action, Args&&... args) {
  using result_t = typename action_t::result_type;

  static_assert(sizeof...(Args) > 0,
                "bind_task: a task must be given a callback as its last argument");

  constexpr std::size_t last = sizeof...(Args) - 1;
  using callback_t = std::tuple_element_t<last, std::tuple<std::decay_t<Args>...>>;

  static_assert(task_callback_for<callback_t, result_t>,
                "bind_task: the last argument must be a callback taking the task's result and "
                "returning nothing (void() for a task that returns nothing)");

  // References into the caller's arguments. What is copied out of it is the task's own, below.
  auto pack = std::forward_as_tuple(std::forward<Args>(args)...);

  // The pack is indexed rather than unpacked, because everything but its last element is bound and
  // a fold cannot say "all but the last".
  return [&]<std::size_t... i>(std::index_sequence<i...>) {
    return task<result_t>{
        .call = [action = std::move(action), ... bound = std::decay_t<decltype(std::get<i>(pack))>(
                                                 std::get<i>(pack))]() mutable -> result_t {
          return action(bound...);
        },
        .callback = std::get<last>(pack)};
  }(std::make_index_sequence<last>{});
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
 * @remark Failure convention: an action that throws does not stop the invocation. What it threw
 * is stored in actuator::errors, one std::exception_ptr per failure, and the actions after it
 * still run. An untangle::invalid_action means a dead binding, so that action is dropped as well;
 * anything else leaves the action in place. Nothing is printed and nothing escapes the call - the
 * caller reads actuator::errors and decides.
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
  /**
   * @brief Tasks container type.
   *
   * @remark The elements are **values**, not pointers. An action is stored by pointer because
   * \ref remove() needs identity and two std::function(s) cannot be compared; a task is never
   * removed one at a time -- \ref call_tasks() consumes the whole list -- so it needs neither.
   *
   * @remark A std::list for the same reason \ref owned is one: adding never invalidates the
   * address of an element already in it, so a reference taken into the list stays good.
   */
  using tasks_t = std::list<task<typename action_t::result_type>>;
  using result_t = std::conditional<std::is_void<typename action_t::result_type>::value, int,
                                    typename action_t::result_type>;
  /**
   * @brief Results container type.
   *
   * It holds the return values of the actions that have a non-void return type.
   * Upon the actuator invocation, the returns can be extracted from \ref results.
   */
  using results_t = std::vector<typename result_t::type>;
  /**
   * @brief Errors container type.
   *
   * It holds what the actions threw, as std::exception_ptr, in the order they were invoked.
   */
  using errors_t = std::vector<std::exception_ptr>;

  actions_t actions;          //!< Actions list.
  actions_map_t actions_map;  //!< Named actions map.
  results_t results;          //!< Actions return values list.
  errors_t errors;            //!< What the actions threw. See \ref errors_t.

  /**
   * @brief Tasks waiting to be fired, in the order \ref add_task() took them.
   *
   * @remark Held beside \ref actions and sharing nothing with them: \ref operator()() fires the
   * actions and leaves these alone, \ref call_tasks() fires these and leaves the actions alone.
   */
  tasks_t tasks;

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
   * @brief Remove all actions, and any stored results and errors.
   *
   * After this call the actuator is empty: actuator::is_connected() returns false. The
   * actions it owns are destroyed, so every handle returned by \ref add(action_t&&) is
   * dangling afterwards; the actions it merely points at are left untouched.
   */
  void reset() {
    actions.clear();
    actions_map.clear();
    results.clear();
    errors.clear();
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
    // Named explicitly, like every member here: this function does not default, so anything it
    // does not copy is silently lost by every copy of an actuator.
    tasks = other.tasks;
    results = other.results;
    errors = other.errors;
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
   *
   * @remark Nothing escapes this call: what an action throws goes to actuator::errors, and the
   * actions after it still run. See the failure convention on \ref actuator.
   */
  template <typename... Args>
  void operator()(Args&&... args) {
    results.clear();
    errors.clear();

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
      } catch (const invalid_action&) {
        // A dead binding is not coming back: it is recorded and the action is dropped.
        errors.push_back(std::current_exception());
        dead_actions.push_back(action);
      } catch (...) {
        // Anything else is the action's own failure. It is recorded, the action is kept, and the
        // actions behind it still run.
        errors.push_back(std::current_exception());
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
   * @remark What the action throws is stored in actuator::errors, as it is for
   * \ref operator()(). See the failure convention on \ref actuator.
   *
   * @param name - Key associated with the action. Invoking a key that is not in the map does
   * nothing.
   * @param args - Arguments list must match the action arity. A trailing callback is invoked
   * with the action's return value; see the convention on \ref actuator.
   */
  template <typename... Args>
  void invoke_action(const std::string& name, Args&&... args) {
    results.clear();
    errors.clear();

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
    } catch (const invalid_action&) {
      errors.push_back(std::current_exception());
      const action_t* dead_action = it->second;
      actions_map.erase(name);
      release_owned(dead_action);
    } catch (...) {
      errors.push_back(std::current_exception());
    }
  }

  /**
   * @brief Fires every task held, notifies each callback, and empties the list.
   *
   * A task runs, and the callback it was built with is invoked with what it returned -- with
   * nothing at all, for a task whose action returns void, because *finished* is the message and
   * the result is optional. Tasks run in the order \ref add_task() took them.
   *
   * @remark **It does not fire the actions**, exactly as \ref operator()() does not fire the
   * tasks. The two kinds share an actuator and nothing else.
   *
   * @remark **A task's result is delivered to its callback and nowhere else.** actuator::results is
   * how an action hands back what it returned; a task was built with something better, and does not
   * need both.
   *
   * @remark **What anything throws is appended to actuator::errors, not written over it.**
   * \ref operator()() clears that list as it starts and this does not, so an actuator fired as
   * `one(); one.call_tasks();` reports both kinds together -- which means **the actions go
   * first**. The other order loses what the tasks recorded.
   *
   * @remark Failure convention, as \ref operator()() has one: a task that throws does not stop the
   * pass, and the tasks behind it still run.
   *
   * @attention **Finished does not mean failed.** A task whose action throws is *not* notified:
   * there is no result to hand over, and for a void task no completion to report either. What it
   * threw goes to actuator::errors and nothing else is said. A caller relying on the callback
   * alone will never hear about it -- read the errors.
   *
   * @attention A callback runs inside the same `try` as the task that owns it, so what **it**
   * throws travels the same path. actuator::errors can therefore hold a failure for a task whose
   * action in fact succeeded. The task is consumed either way; re-running an action that already
   * ran, to reach a callback that already threw, would be worse than losing the notification.
   */
  void call_tasks() {
    // Taken by swap, so a callback that adds a task adds it to the *next* pass. Iterating the
    // member instead would let a task that re-adds itself keep this loop from ever ending -- and
    // it is the same reason a queue built on this takes its batch by move.
    tasks_t firing;
    firing.swap(tasks);

    for (auto& one : firing) {
      try {
        if constexpr (std::is_void_v<typename action_t::result_type>) {
          one();
          one.callback();
        } else {
          // One expression, so the result is handed over as an rvalue: a result type that cannot
          // be copied is still a result a task can report.
          one.callback(one());
        }
      } catch (...) {
        // The task's own failure, or its callback's -- the two are not told apart here, and the
        // attention above says why.
        errors.push_back(std::current_exception());
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
   * @brief Adds a task, to be fired by \ref call_tasks().
   *
   * A task is built by \ref bind_task(), which binds an action to its arguments and to the callback
   * it must notify, and is stored here by value -- the actuator owns every task it holds, unlike
   * the actions it merely points at.
   *
   * @remark There is no counterpart to \ref remove(). \ref call_tasks() consumes the list, so a
   * task leaves by being fired; nothing needs to identify one afterwards.
   *
   * @remark Nor is there a named form. \ref actions_map exists so \ref invoke_action() can fire
   * one action on demand; tasks are fired as a batch, and the order they were added in is the order
   * they run in.
   *
   * @attention It refuses a task that could not do its job, and **the refusal is the whole report**
   * -- nothing is thrown and nothing is stored. Both cases are the caller's own mistake and both
   * are caught here, while the caller is still on the stack, rather than surfacing from
   * \ref call_tasks() as a std::bad_function_call recorded in actuator::errors. There it would be
   * read as the failure of a task whose action had in fact succeeded, on whatever thread happened
   * to fire it.
   *
   * @param task - The task to add. It is taken by value and moved from; a refused one is dropped.
   *
   * @return true - the task was taken, and \ref call_tasks() will fire it.
   * @return false - it was refused, because it has nothing to run or no callback to notify. A task
   * that cannot report is not a task; see the convention on \ref untangle::task.
   */
  bool add_task(task<typename action_t::result_type>&& task) {
    if (!task || !task.callback) {
      return false;
    }

    tasks.push_back(std::move(task));
    return true;
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
   * @remark **It answers for the actions and for nothing else.** Tasks are not actions and do not
   * make an actuator connected; ask \ref has_tasks() about those. Kept this way deliberately when
   * tasks arrived, because a caller that will never hold a task -- an attachment, a poll -- reads
   * this and must keep reading the answer it always read.
   *
   * @return true - if the actuator::actions list is not empty.
   * @return false - if the actuator::actions list is empty.
   */
  bool is_connected() const { return !actions.empty() || !actions_map.empty(); }

  /**
   * @brief Is there a task waiting to be fired?
   *
   * The counterpart to \ref is_connected(), and the two answer different questions: this one is
   * about actuator::tasks, that one about actuator::actions. An actuator can hold either, both, or
   * neither.
   *
   * @remark It is what a queue asks to decide whether its worker still has work. Reading
   * \ref is_connected() for that would report a batch of tasks as nothing to do, and the work
   * would sit unfired.
   *
   * @remark A refused task is not a waiting one: \ref add_task() answering false means the task was
   * dropped, so this stays false.
   *
   * @return true - a task is waiting; \ref call_tasks() has something to fire.
   * @return false - no task is waiting. True again after \ref call_tasks(), which consumes them.
   */
  bool has_tasks() const { return !tasks.empty(); }

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
