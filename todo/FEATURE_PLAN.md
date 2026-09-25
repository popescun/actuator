# actuator.hpp — tasks, feature plan

**A task is an action bound to its arguments, and to the callback it must notify.** The actuator
carries only actions today, and fires them with a pack the caller supplies at invocation. This plan
adds tasks beside them: `untangle::bind_task()` to build one, `actuator::add_task()` to hold one,
`actuator::call_tasks()` to fire them.

**A task without a callback does not exist.** Decided 2026-09-25, and it is what separates a task
from an action rather than a restriction laid on top: an action is fired and forgotten, a task
reports that it finished. A task returning nothing reports with a `void()` callback — *finished* is
the message, the result is optional. The callback is a **named parameter of `bind_task()`**, not a
trailing argument the actuator infers, and it is **not** forwarded to the action.

**Status (2026-09-25) — nothing written, three probes run, one step closed as declined, one
deleted.**
This is a feature plan, not a fix plan: no step below is a defect in code meant to do something
else. It has one defect at its root all the same — the callback convention does not survive being
queued — and that is why the feature is worth having rather than a tidier spelling of what exists.
Claims marked **PROBED** were compiled and run on 2026-09-24/25; the rest are read-only and say so.

**This is the first of three.** `async` and `executor` have their own, in the same shape:
`async/todo/FEATURE_PLAN.md` holds the queue, `todo/FEATURE_PLAN.md` in `executor` holds the pool
and the two cases that close the whole chain. Steps are numbered per repo, as in `FIX_PLAN.md`;
cross-repo references are qualified ("async's step 5"). **Nothing in the other two can start until
this repo is green and bumped.**

**Baseline.** `a8b8b47`. Sites are line numbers at that commit and move with every step that lands.

## Why — the defect at the root

`invoke_callback()` (`:327`) fires a trailing `std::function<void(R)>` with the action's result. It
reads that callback out of the **invocation** argument pack, copied at `:354` and `:416`. Every
queueing layer above binds the pack into a nullary callable long before this actuator sees it, so
the pack read here is empty and the callback is never fired:

| Path | What the actuator is invoked with | Callback |
|---|---|---|
| `connect(action)` then `a(21, cb)` | `(21, cb)` | **fired** — 42 |
| `execution::add_action(action, 21, cb)` | `()` — `std::bind` sealed the pack | lost |
| `executor::add_task(task, 21, cb)` | `()` — sealed a second time | lost |

**PROBED.** The action runs and the callback is silently skipped; nothing warns, because under the
convention the callback is also a legitimate argument of the action, so every `static_assert` and
overload is satisfied.

**Two constraints bound every fix, and they are what shaped this plan.**

1. **Extraction at the binding site is unavoidable.** `std::bind` returns an opaque object; once the
   pack is inside it, nothing downstream can find the callback again. Only *who fires it* is
   negotiable.
2. **This actuator cannot fire it from its own invocation pack.** A batch holds many queued calls,
   each added by a different caller with a different callback, while `operator()` broadcasts one
   pack to all of them. There is no pack that would be right for all. So the callback must travel
   **with its action** — which is what a task is.

## What a task is, and why it needs less machinery than an action

**An action is a subscription; a task is a one-shot.** That is the whole distinction, and it pays
for the split on its own:

| | Action | Task |
|---|---|---|
| lives | across invocations | fired once, then gone |
| stored as | `std::list<action_t*>` (`:123`) + `std::list<action_t> owned` (`:153`) | `std::list<task<R>>` — values, no indirection |
| needs | `remove()`, `release_owned()`, the dead-action sweep, `translate()` in `copy_from` | none of it |
| arguments | supplied by the caller at invocation | bound into it at `bind_task()` |
| callback | trailing argument of the invocation, **optional**, inferred from its type | a named parameter, **required**, carried per task |
| a void one | fires nothing — no result to report | fires `void()` — *finished* is the message |

An action is stored by pointer because `remove()` needs identity and two `std::function`s cannot be
compared (`:118-122`). A task is never removed individually — `call_tasks()` consumes the list — so
it is stored by value and that entire apparatus is not needed. A dead binding throws
`invalid_action` into the existing `catch`, lands in `errors`, and is dropped because everything is
dropped.

## The shape, proven

**PROBED, 2026-09-24/25**: a tasks list carrying its own callbacks, fired in a loop, notifying
correctly — with a `std::function`, with a bare lambda, and leaving a value-returning trailing
callable alone.

```
results   = [42, 105]
transform task carries a callback? false (want false)
callback  = 42 (want 42)
lambda callback = 21 (want 21)
```

**What the probe validated is the mechanism, not the signature below.** It ran the earlier shape,
where the callback was the trailing argument and `bind_task` inferred it. The decision of
2026-09-25 made it a named parameter, which is strictly easier — no inference, no negative case to
get right — so the probe over-tested rather than under-tested. The one line it still carries is
that a callback travels inside a task and fires from the list, which is what the whole feature
rests on.

```c++
//! Can callback_t be called as the completion callback of a task returning result_t?
template <typename callback_t, typename result_t>
concept task_callback_for =
    (std::is_void_v<result_t> &&
     requires(callback_t& c) { requires std::is_void_v<decltype(c())>; }) ||
    requires(callback_t& c, result_t& r) { requires std::is_void_v<decltype(c(r))>; };

//! An action bound to its arguments and to the callback it notifies when it finishes.
template <typename result_t>
struct task {
  //! What a task of this result type notifies. A void task says finished and nothing else.
  using callback_t = std::conditional_t<std::is_void_v<result_t>, std::function<void()>,
                                        std::function<void(result_t)>>;

  using result_type = result_t;
  result_type operator()() { return call(); }
  explicit operator bool() const { return static_cast<bool>(call); }

  std::function<result_type(void)> call;
  callback_t callback;
};

template <typename action_t, typename callback_t, typename... Args>
  requires task_callback_for<callback_t, typename action_t::result_type>
auto bind_task(action_t action, callback_t callback, Args&&... args) {
  using result_t = typename action_t::result_type;

  return task<result_t>{
      .call = [action = std::move(action), ... args = std::forward<Args>(args)]() mutable
              -> result_t { return action(args...); },
      .callback = std::move(callback)};
}
```

**The callback is a parameter, not an inference, and that is the whole reason it can be required.**
The actions path finds its callback by inspecting the last argument — callable with `R`, returns
void — which works only because it is optional: a trailing argument that is not one simply fires
nothing. `actuator.hpp:77-80` records the trap that comes with it, a callback written to return a
value being "silently not invoked". A mandatory callback cannot be found that way. Declared as a
parameter it cannot be omitted, cannot be mistyped into silence, and needs no `static_assert` to
enforce — the signature does it.

**It is not forwarded to the action**, which is the second dividend. Under the old shape every task
action had to carry a callback parameter it ignored, and a void task's signature had to grow one
for a callback it could never use. Now `std::function<int(int)>` is a task action, and so is
`std::function<void(int)>`.

**`call_tasks()` therefore has no test to make** — there is always a callback, so the fire is
unconditional:

```c++
if constexpr (std::is_void_v<result_t>) { one(); one.callback(); }
else { results.push_back(one()); one.callback(results.back()); }
```

**`bind_task` is not new code.** It is `executor::bind_task` (`executor.hpp:304`) line for line —
the pool invented it privately because it needed it. Moving it here is what lets the pool delete its
copy, and what lets async delete `queued_action_t`.

**The void disjunct is safe. PROBED:** with `result_t = void` the second `requires` yields `false`
rather than a hard error, because `void&` in a requires-parameter list is a substitution failure in
the immediate context. So the first disjunct decides, and a void task is constrained to a `void()`
callback.

**An empty `std::function` still gets past the signature.** `bind_task(action, {}, ...)` compiles
and satisfies the concept. Step 4 decides whether `add_task()` refuses it — which is a runtime
check, not a type one — and that is the one hole the parameter cannot close by itself.

## Step index

| # | Step | Sites | Evidence |
|---|---|---|---|
| 1 | `task_callback_for`, and `task<result_t>` with its `callback_t` | new | PROBED (void case) |
| 2 | `untangle::bind_task()` — action, callback, arguments | new, beside `last_arg` `:54` | PROBED |
| 3 | `tasks` storage and `add_task()`, and what it does with an empty callback | new, beside `:141-153` | read-only |
| 4 | `call_tasks()` — fire, notify, record, consume | new, mirrors `:349-390` | read-only |
| 5 ✅ | `operator()()` with no arguments | — | PROBED — **DECLINED** |
| 6 | `is_connected()` vs a new `has_tasks()` | `:552` | **OPEN, decision** |
| 7 | the suite gains tasks | `test/actuator_test.cpp` | — |
| 8 | the actions path gains the callback cases it never had | `test/actuator_test.cpp` | **independent, see below** |
| 9 | `tools/make_doc.sh`, and the bump async takes | `doc/` | — |

### Step 1 · the two rules are different, so they do not share a concept

An earlier draft had one concept serving both paths, with `invoke_callback()` (`:327`) rewritten
onto it. **That step is gone.** Once a task's callback became required, took a named parameter and
admitted `void()`, the two rules stopped being the same rule:

| | Action callback | Task callback |
|---|---|---|
| optional | yes | no |
| void results | never fires | fires `void()` |
| found by | inspecting the trailing argument | declared as a parameter |
| forwarded to the action | yes, it is also an argument | no |

`task_callback_for` is therefore task-only, and `invoke_callback()` is not touched by this plan at
all. Anything in the actions path that looks like this feature is coincidence.

### Step 3 · the hole the signature cannot close

The parameter makes a callback impossible to *omit*. It does not make it impossible to pass an
empty `std::function`, which satisfies the concept and then throws `std::bad_function_call` when
`call_tasks()` fires it — inside the actuator's `try`, so it would surface as a task failure on the
`errors` path, blaming the task for the caller's mistake.

| Route | Cost |
|---|---|
| `add_task()` refuses an empty callback, returning `false` or throwing | a runtime check on a rule the signature otherwise enforces statically; the caller learns at the right moment |
| Let it reach `call_tasks()` and become an error | no new code; the report names the wrong culprit |
| Ignore it | a task that silently never notifies, which is the defect this whole plan exists to remove |

Not decided. It is the only place the required-callback rule is not enforced by the type system.

### Step 4 · what `call_tasks()` promises

Four rules, all worth stating in the reference rather than leaving to be inferred:

- **The callback always fires** on a task that returns normally. There is no `if` — that is what
  required means, and it is the one promise a caller gets from a task that an action never gave.
- **Finished does not mean failed. Decided 2026-09-25.** A task whose action throws gets no
  callback: there is no result to report, and for a void task there is no completion to report
  either. What it threw travels the existing `errors` path to `on_error`, unchanged. **Document it
  as a choice, not an omission** — a caller who reads "a task always notifies" will otherwise
  assume it notifies here too. Revisit if a use case asks for it; an `exception_ptr` overload is
  the obvious shape and nothing here forecloses it.
- **A throwing callback lands in `errors`**, because it runs inside the actuator's existing `try`.
  It is the same path a failing action takes, which means a handler upstream can fire for a task
  whose body succeeded. Uniform, and surprising if unsaid.
- **`call_tasks()` consumes.** Each task fires once and the list is empty afterwards. That is the
  one-shot half of the action/task distinction, and it is what removes the need for `remove()`.

### Step 5 ✅ · `operator()()` with no arguments — DECLINED

Asked for as "it would be nice to be able to overload the operator". It cannot be had. **PROBED, and
the result is worse than the ambiguity that was expected:**

```
a();        -> fired TASKS      // wanted ACTIONS
a(1, 2);    -> fired ACTIONS
```

It is never ambiguous. `action_t` is a *class* template parameter, fixed before overload resolution
runs, so the arity of the actions stored plays no part; the choice is made from the call expression
alone, and a non-template beats a template specialization with an empty pack. `a()` resolves to the
new overload every time.

**An ambiguity would have been the safe outcome** — the call sites would fail to compile, and be
fixed. Instead they compile and change meaning in silence. There are four, all in async:
`batch()` (`async.hpp:750`), `actuator_execute_()` (`:784`), `actuator_stop_()` (`:469`),
`actuator_is_running_()` (`:172`).

**The nullary case is the worst one, not the safest. PROBED:**

```
a()  with one nullary action connected:
  -> fired TASKS (0 held)
action ran 0 time(s) -- want 1
```

Three of those four actuators hold nullary actions — `actuator_execute_` and `actuator_stop_` are
`std::function<void(void)>`, `actuator_is_running_` is `std::function<bool(void)>`. For them `a()`
is the *only* natural spelling, so the overload does not shadow one of two ways in, it takes the
only way in. What is left is `a.operator()<>()`, forcing the template with an explicit empty
argument list, which nobody would write.

**Taken: `call_tasks()`, named.** If the operator spelling is wanted later, the only safe meaning is
"fire the actions, **then** the tasks" — a superset that leaves those four sites doing what they do
today. Not "fire the tasks".

### Step 6 · `is_connected()` or `has_tasks()` — OPEN

`is_connected()` (`:552`) answers from `actions` and `actions_map`. `async::has_pending_actions()`
(`async.hpp:686`) reads it, and after async's step 1 the queue holds tasks and no actions — so left
alone, a queue full of tasks reports itself empty and the drain breaks.

| Route | Cost |
|---|---|
| Extend `is_connected()` to include tasks | arguably correct — "does this actuator hold anything" — but it is also read by the attachment paths, so the meaning changes for callers that will never hold a task |
| Add `has_tasks()`, point `has_pending_actions()` at it | two predicates to keep straight; `is_connected()` keeps its current meaning exactly |

Not decided. One line either way, and a deliberate one rather than a drive-by. **It gates async's
step 4.**

### Step 8 · the actions path has never been tested — and that is not this feature

`grep -i callback` over `test/` and `example/` returns nothing. Four commits built the actions
callback convention — `f001d12` (named actions), `d731129` (lambdas qualify), `5816237` (a callback
must return void) — and not one of them left a case behind. The baseline is 25 of 25 green and none
of the 25 touch it.

It is recorded here because this is where it was found, not because the feature depends on it. With
step 1 gone, nothing in this plan modifies `invoke_callback()`, so no safety net is owed before the
work starts. **Land it separately**, on its own commit, whenever it suits: callback fired with each
action's result, a lambda qualifying, a value-returning trailing argument left alone,
`invoke_action()` firing it too, and a void action getting none.

## Order

1 and 2 are the mechanism and can land in one review — the type and the helper that builds it. 3
and 4 are the storage and the firing, and 3 carries the one open question the signature cannot
answer. 5 is closed. **6 must be settled before this repo is bumped**, because async cannot write
its own step against the queue predicate without it. 8 is independent of all of them.

**Nothing downstream constrains this plan any more.** An earlier draft flagged async's
`bind_action_and_method()` / `_function()` as able to send it back: they hold their action in a
`std::shared_ptr<const actionT>` while `bind_task()` takes it by value, so routing
`execution::add_action()` through `bind_task()` would have needed a wrapper whose shape could force
`bind_task`'s signature to change after step 2 had landed. The decision of 2026-09-25 removed it —
`add_action()` stays callback-free and keeps its own `std::bind`, so those two bindings are not
touched. **Steps 1 and 2 are free to be written on their own terms.**

## Working method

Inherited from `FIX_PLAN.md`, unchanged. **Each step is a test first**: a case that shows what is
missing, put up for review on its own, and the code written only once the case is agreed. The test
is the unit of review, not the implementation. **One step per commit**, and a step's case travels
with its own fix. The plan's own updates are their own commit, and always a later one.

**Every header change is followed by `tools/make_doc.sh`.**

## Progress

| Commit | Step |
|---|---|
| — | nothing landed |

**NEXT: step 1.** Step 5 is already answered. Two decisions are open: step 3 — what `add_task()`
does with an empty callback — and step 6, which gates async.
