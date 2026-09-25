# actuator

An `actuator` is a generic callable that may invoke a list of `actions`(callables of type `std::function<...>`).

Its goal is to provide a light and simple mechanism to connect components that want to communicate with each other. It works similar as the "signal and slots" mechanisms from other frameworks, but much simplified.

Modern programming languages do not offer class intrinsic interfaces that would improve significantly how objects are connected and how they communicate. 
Instead, languages, like `C++`, are using external interfaces, that are not only cluttering the code, but they don't feel "natural" as they are opposed to 
what an interface is in reality. Basically the objects and interfaces are not separated, and the interface is intrinsic to the object(think on the cogs in a mechanism). 
A generic callable is an attempt to mitigate this problem. [async](https://github.com/popescun/async) implementation is such use case.

An actuator provides the results of all actions in a vector, in the same order as the actions were added.

### Example: how to use actuator instead of polymorphism

```c++
void rotate_shapes(const std::vector<shape*>& shapes, int angle)
{
  for (const auto& s : shapes)
  {
    s->rotate(angle);
  }
}

std::shared_ptr<triangle> t(new triangle);
std::shared_ptr<circle> c(new circle);
std::shared_ptr<square> s(new square);

// using polymorphism
std::vector<shape*> shapes;
shapes.push_back(t.get());
shapes.push_back(c.get());
shapes.push_back(s.get());

rotate_shapes(shapes, 10);

// using actuator
auto action1 = untangle::bind(t, &triangle::rotate);
auto action2 = untangle::bind(c, &circle::rotate);
auto action3 = untangle::bind(s, &square::rotate);
auto actuator_rotate = untangle::connect(action1, action2, action3);
actuator_rotate(20);
```

### Anonymous lambdas as actions

An actuator normally does not own its actions: it stores pointers to `std::function` objects the caller keeps alive. `add()` also has an overload that takes an action **by value**, so an anonymous lambda can be an action with no named variable to keep around, and returns a handle to pass to `remove()`:

```c++
untangle::actuator<std::function<int(int)>> actuator_scale;
auto* handle = actuator_scale.add([](int v) { return v * 2; });
actuator_scale.add("triple", [](int v) { return v * 3; });
```

`connect()` accepts them too, but it deduces the action type from its arguments, and a lambda has its own closure type -- it is not a `std::function` until something converts it. A call made *only* of anonymous lambdas therefore has nothing to deduce from, and the signature has to be named on `connect()` itself:

```c++
auto actuator_rotate = untangle::connect<std::function<void(int)>>(
    [](int angle) { /* ... */ }, [](int angle) { /* ... */ });
```

Naming the type of the variable the result is assigned to does **not** supply it -- template arguments are deduced from the call arguments alone, never from what the returned value is assigned to:

```c++
// ill formed: "no matching function for call to connect"
untangle::actuator<std::function<void(int)>> actuator_rotate =
    untangle::connect([](int angle) { /* ... */ });
```

One named action anywhere in the call deduces the type for the whole of it, and the anonymous lambdas beside it then need nothing:

```c++
auto actuator_rotate = untangle::connect(action1, [](int angle) { /* ... */ });
```

The named form of `connect()` works the same way: a pair holding a *pointer* names an action the caller owns, a pair holding the action *by value* hands it over to the actuator, and the two can be mixed. The action type is deduced from the pointer, so a call whose first pair holds a lambda has to name the signature:

```c++
auto actuator_rotate = untangle::connect<std::function<void(int)>>(
    std::make_pair("triangle", [](int angle) { /* ... */ }),
    std::make_pair("circle", [](int angle) { /* ... */ }));

// deduced from the leading pointer, so the lambda beside it needs nothing
auto actuator_mixed = untangle::connect(std::make_pair("triangle", &action1),
                                        std::make_pair("circle", [](int angle) { /* ... */ }));
```

### Tasks: an action bound to its arguments and to the callback it must notify

An action is a subscription: it lives across invocations, and it is fired with a pack the caller supplies each time. A **task** is a one-shot. It carries its own arguments, is fired once, and reports what it produced to a callback it was built with. The two live side by side in one actuator and share nothing else -- `operator()` fires the actions and leaves the tasks alone, `call_tasks()` fires the tasks and leaves the actions alone.

A task exists so that work which runs *later* than it was described can still say what became of it. That is what a queue needs, and an actuator holding only actions cannot give: the callback convention on `operator()` reads the trailing argument of the invocation, and a queued call has no invocation left to read from.

```c++
untangle::actuator<std::function<int(int)>> actuator_scale;

// the callback comes last, after the arguments the action is bound to
actuator_scale.add_task(untangle::bind_task(action, 21, [](int result) { /* result == 42 */ }));

actuator_scale.call_tasks();  // runs it, notifies it, and empties the list
```

**A task without a callback does not exist.** That is the whole difference from an action's callback, which is optional and is *recognised* by its type -- a trailing argument that does not look like a callback is simply an argument, and nothing fires. A task's callback is the **last argument** of `bind_task()`, taken by position, and `task_callback_for` only checks that it can serve. There is no answer "no" to fall through to, so a missing or unusable callback is a `static_assert` rather than silence.

It is not forwarded to the action either, so an action needs no callback parameter of its own -- and a task whose action returns `void` is a task like any other. It reports with a `void()` callback, because *finished* is the message and the result is optional:

```c++
untangle::actuator<std::function<void(int)>> actuator_notify;
actuator_notify.add_task(untangle::bind_task(action, 5, [] { /* finished */ }));
```

The arguments are **copied** when the task is built, while the caller still holds them, and handed to the action as the task's own lvalues when it runs. A task outlives the call that described it and cannot borrow what may already be gone; a move-only argument therefore does not compile.

`add_task()` answers `bool`. It refuses a task that has nothing to run or no callback to notify, and **the refusal is the whole report** -- nothing is thrown and nothing is stored. Both are the caller's own mistake, and both are caught while the caller is still on the stack rather than surfacing later from `call_tasks()` as a `std::bad_function_call` recorded against a task whose action had in fact succeeded.

**Finished does not mean failed.** A task whose action throws is *not* notified: there is no result to hand over, and for a void task no completion to report either. What it threw goes to `actuator::errors`, the tasks behind it still run, and a caller relying on the callback alone will never hear about it. A callback runs inside the same `try` as the task that owns it, so what *it* throws travels the same path -- `errors` can hold a failure for a task that succeeded.

A task's result goes to its callback and nowhere else: `actuator::results` is how an *action* hands back what it returned, and a task was built with something better. `call_tasks()` appends to `errors` rather than clearing it, while `operator()` clears both as it starts -- so an actuator fired as `one(); one.call_tasks();` reports both kinds together, and the actions go first.

Ask `has_tasks()` whether a task is waiting. `is_connected()` answers for the actions and for nothing else: an actuator holding only tasks is *not* connected, and a queue reading the wrong one would report a batch of tasks as nothing to do.

For convenience there are provided helpers methods to "connect" to an initial list of "actions", or to create bindings to class methods.

Please check the manual in _doc/refman.pdf_ for further references.
