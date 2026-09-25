# actuator.hpp — tasks, feature plan

**A task is an action bound to its arguments, and to the callback it must notify.** The actuator
carries only actions today, and fires them with a pack the caller supplies at invocation. This plan
adds tasks beside them: `untangle::bind_task()` to build one, `actuator::add_task()` to hold one,
`actuator::call_tasks()` to fire them.

**A task without a callback does not exist.** Decided 2026-09-25, and it is what separates a task
from an action rather than a restriction laid on top: an action is fired and forgotten, a task
reports that it finished. A task returning nothing reports with a `void()` callback — *finished* is
the message, the result is optional. The callback is the **last argument of `bind_task()`**, taken
by position rather than recognised by type, and it is **not** forwarded to the action.

**Status (2026-09-25) — CLOSED. Committed as `beb5fe8`: steps 1, 2, 3, 4, 6 and 7 done, 99 of 99
green.
Four probes run; one step closed as declined, one deleted, two struck, two claims corrected. No
decision left open.**
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
| callback | trailing argument of the invocation, **optional**, recognised by its type | the last argument of `bind_task()`, **required**, taken by position, carried per task |
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
2026-09-25 kept it last but made it required and checked rather than recognised, which is strictly
easier — nothing to infer and no negative case to get right — so the probe over-tested rather than
under-tested. The one line it still carries is
that a callback travels inside a task and fires from the list, which is what the whole feature
rests on.

```c++
//! Can callback_t be called as the completion callback of a task returning result_t?
template <typename callback_t, typename result_t>
concept task_callback_for =
    (std::is_void_v<result_t> &&
     requires(callback_t& c) { requires std::is_void_v<decltype(c())>; }) ||
    requires(callback_t& c, result_t& r) { requires std::is_void_v<decltype(c(r))>; };

/**
 * @brief The callback type a task returning \p result_t needs.
 *
 * @remark A specialisation rather than a std::conditional_t, which does not work here:
 * conditional_t forms **both** branches before choosing, and std::function<void(result_t)> with
 * result_t = void is ill formed -- a parameter of type void cannot be produced by substitution,
 * however legal void(void) is as literal syntax. A specialisation never forms the branch it does
 * not take. PROBED: the conditional_t spelling fails with "argument may not have 'void' type".
 */
template <typename result_t>
struct task_callback_type {
  using type = std::function<void(result_t)>;
};
template <>
struct task_callback_type<void> {
  using type = std::function<void()>;
};

//! An action bound to its arguments and to the callback it notifies when it finishes.
template <typename result_t>
struct task {
  //! What a task of this result type notifies. A void task says finished and nothing else.
  using callback_t = typename task_callback_type<result_t>::type;

  using result_type = result_t;
  result_type operator()() { return call(); }
  explicit operator bool() const { return static_cast<bool>(call); }

  std::function<result_type(void)> call;
  callback_t callback;
};

/**
 * @brief Binds an action to its arguments and to the callback it must notify, which is last.
 *
 * @remark The pack cannot be followed by a deducible parameter, so the callback arrives inside it
 * and is split off here. Every caller above forwards (action, args...) and knows nothing of it.
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

  auto pack = std::forward_as_tuple(std::forward<Args>(args)...);

  return [&]<std::size_t... i>(std::index_sequence<i...>) {
    return task<result_t>{
        .call = [action = std::move(action),
                 ... bound = std::decay_t<decltype(std::get<i>(pack))>(
                     std::get<i>(pack))]() mutable -> result_t { return action(bound...); },
        .callback = std::get<last>(pack)};
  }(std::make_index_sequence<last>{});
}
```

**By position, not by inference — and that distinction is the whole reason it can be required.**
Both paths read the last argument, so it is worth being exact about how they differ. The actions
path asks *whether* that argument is a callback, by type: callable with `R`, returns void. It can
only ask that because the answer is allowed to be no — a trailing argument that is not a callback is
simply an argument, and nothing fires. `actuator.hpp:77-80` records the trap that comes with it, a
callback written to return a value being "silently not invoked".

A task asks nothing. The last argument **is** the callback, and `task_callback_for` only checks that
it can serve as one. There is no answer "no" to fall through: a wrong callback is a
`static_assert`, never a skip. **PROBED** — the diagnostic a caller who forgets it gets:

```
error: static assertion failed: bind_task: the last argument must be a callback taking
the task's result and returning nothing (void() for a task that returns nothing)
```

**The cost of last rather than first.** A parameter pack cannot be followed by another parameter and
still be deduced, so `bind_task` takes one pack and splits the last element off itself — an
`index_sequence` fold, about six lines. It is paid **once**: `actuator::add_task()`,
`execution::add_task()` and `executor::add_task()` all forward `(action, args...)` with the callback
inside the pack, and none of them knows a split happened. Chosen for the call shape, which then
reads the way the work does — the arguments, then what to do when it finishes.

**One wrinkle it buys.** If an action's own signature ends in a `std::function<void(R)>` parameter,
then `add_task(action, cb)` reads two ways: the compiler takes `cb` as the callback and then fails
because the action wanted an argument. It resolves as an error and never silently, but the error
will not say "you meant that as an argument". Callback-first could not produce the case at all;
this is the trade.

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
| 1 ✅ | `task_callback_for`, `task_callback_type` and `task<result_t>` | `:63-162` | CONFIRMED (7 cases) — **DONE** (`beb5fe8`) |
| 2 ✅ | `untangle::bind_task(action, args..., callback)` — the pack split | `:166-226` | CONFIRMED (8 cases) — **DONE** (`beb5fe8`) |
| 3 ✅ | `tasks` storage and `add_task()`, refusing an empty callback | `:291-301`, `:323-329`, `:697-731` | CONFIRMED (9 cases) — **DONE** (`beb5fe8`) |
| 4 ✅ | `call_tasks()` — fire, notify, record, consume | `:638-693` | CONFIRMED (8 cases) — **DONE** (`beb5fe8`) |
| 5 ✅ | `operator()()` with no arguments | — | PROBED — **DECLINED** |
| 6 ✅ | `has_tasks()`, `is_connected()` untouched | `:827-857` | CONFIRMED (4 cases) — **DONE** (`beb5fe8`) |
| 7 ✅ | `README.md`, `tools/make_doc.sh`, and the commits | `README.md`, `doc/` | **DONE** (`beb5fe8`) |

### Step 1 ✅ · `task_callback_for`, `task_callback_type`, `task<result_t>` — DONE

An earlier draft had one concept serving both paths, with `invoke_callback()` (`:327`) rewritten
onto it. **That step is gone.** Once a task's callback became required, taken by position rather
than recognised by type, and admitted `void()`, the two rules stopped being the same rule:

| | Action callback | Task callback |
|---|---|---|
| optional | yes | no |
| void results | never fires | fires `void()` |
| found by | asking whether the trailing argument is one | taking the last argument, and checking it |
| forwarded to the action | yes, it is also an argument | no |

`task_callback_for` is therefore task-only, and `invoke_callback()` is not touched by this plan at
all. Anything in the actions path that looks like this feature is coincidence.

**What landed**, at `actuator.hpp:63-162`, +101 lines and nothing removed: the concept, the
`task_callback_type` specialisation, and `task<result_t>` with its `callback_t`, `result_type`,
`operator()` and `explicit operator bool`. Seven cases at `test/actuator_test.cpp:1321-1442`. 70 of
70 green, clang-format clean, doxygen clean, `doc/refman.pdf` at 51 pages.

**`std::conditional_t` cannot express `callback_t`, and step 1's own case is what found it.**
`test_task_names_the_callback_type_its_result_needs` asserts `task<void>::callback_t` is
`std::function<void()>`, and the obvious spelling

```c++
using callback_t = std::conditional_t<std::is_void_v<result_t>,
                                      std::function<void()>, std::function<void(result_t)>>;
```

does not compile for `void` at all: `error: argument may not have 'void' type`. conditional_t forms
**both** branches before choosing one, and `std::function<void(result_t)>` with `result_t = void` is
ill formed — a parameter of type void cannot be produced by substitution, however legal `void(void)`
is as literal syntax. A specialisation never forms the branch it does not take. **The test was
written before the code and falsified the plan's own sketch**, which is the method earning its keep
rather than a near miss.

**The disjuncts are ordered for the formatter, not for logic.** clang-format rendered the void case
first with the `||` buried mid-line, which lost the parallel between the two alternatives; asking
for the result first renders each on its own line. Satisfaction is identical either way. The doc
remark therefore names the disjuncts by what they ask rather than by position, so a future reorder
cannot falsify it — and it records that the `std::is_void_v` guard is load bearing: without it a
`std::function<void()>` satisfies the concept for **any** result type, and a task's result would go
unreported.

**Three forward references are demoted, and each is a debt against a later step.** doxygen is
warnings-as-errors here, and `bind_task()`, `actuator::add_task()` and `actuator::call_tasks()` do
not exist yet, so the header names them in code font instead of `\ref`. **Step 2 owes the `\ref`
for `bind_task()`, step 3 for `add_task()`, step 4 for `call_tasks()`** — turning each into a link
as the symbol arrives. `\ref actuator` was demoted too, from the concept's own block, for what
looked at the time like a different and unexplained reason. **Step 2 explained it — see below: a
`\ref` from inside a concept's block never resolves**, so that one is not a debt and never becomes
a link.

> **`actuator.hpp` is committed with CRLF line endings and `test/actuator_test.cpp` with LF**, and
> mixing them up shows every line of the file as changed: 870 removed and 970 added, for a hundred
> lines of new code. `git diff --ignore-all-space` against the plain diff is what tells the two
> apart, and it is worth running after every edit here.
>
> **Corrected at step 2: clang-format is not the culprit, and this plan said it was.** `.clang-format`
> carries `LineEnding: DeriveCRLF`, with a comment saying the sources are CRLF and this keeps them
> that way. What flattened the file was writing it from Python in text mode, which emits LF;
> clang-format then derived from an already flattened file and kept it flat. **Write the header as
> bytes, or let clang-format finish the job** — do not blame the formatter for it again.

### Step 2 ✅ · `bind_task(action, args..., callback)` — DONE

`actuator.hpp:166-226`, +64 lines and nothing removed. Eight cases at
`test/actuator_test.cpp:1444-1579`. 78 of 78 green, clang-format clean, doxygen clean,
`doc/refman.pdf` at 53 pages.

**The pack is indexed rather than folded**, which is the one part of the implementation that is not
obvious: everything but the last element is bound, and a fold expression cannot say "all but the
last". So the arguments arrive as a `std::forward_as_tuple` of references, a templated lambda takes
`std::index_sequence<i...>` over `sizeof...(Args) - 1`, and each `std::get<i>` is **copied** into
the closure. `std::get<last>` is the callback.

**Two `static_assert`s carry what the signature cannot say**, since the callback is inside the pack:
one that there is at least one argument, one that the last satisfies \ref task_callback_for. Neither
can be a test case — a compile error is not a runtime failure — so they are contract, and their
wording is the diagnostic a caller actually meets.

**A `\ref` from inside a concept's documentation block never resolves.** Step 1 saw `\ref actuator`
fail there and left it unexplained; step 2 reproduced it exactly with `\ref bind_task()`, which
resolves from `task`'s block and from `bind_task`'s own and fails only from the concept's. Two data
points, one rule: **inside a concept, name symbols in code font.** Not a debt against a later step,
because it never becomes a link.

**And doxygen swallows a trailing colon into the symbol name.** `\ref task:` was read as a reference
to a symbol called `task:`. Ending the sentence instead of running a colon onto the reference is the
whole fix, and it is the kind of warning that reads as a missing symbol when it is really
punctuation.

**What is left to a later step by design:** nothing here fires a callback. Every case notifies by
hand and asserts that `task::operator()` alone does **not** — \ref actuator::call_tasks() is step 4,
and a case that leaned on it would be testing two steps at once.

> **A move-only argument does not compile**, because each bound argument is copied. Raised while
> reviewing the cases and left as contract rather than a case, since refusing at compile time is
> not something a runtime case can state. It is the same limitation `executor::add_task()` has
> today, so the chain does not regress; if it ever needs lifting, it is `bind_task` that lifts it.

### Step 3 ✅ · `tasks` storage and `add_task()` — DONE

`actuator.hpp:291-301` (the container type), `:323-329` (the member), `:424-426` (`copy_from`),
`:697-731` (`add_task()`). Nine cases at `test/actuator_test.cpp:1581-1702`. 87 of 87 green,
clang-format clean, doxygen clean, `doc/refman.pdf` at 58 pages.

**The empty callback: route one, decided 2026-09-25.** The parameter makes a callback impossible to
*omit*; it does not make it impossible to pass an empty `std::function`, which satisfies
`task_callback_for` and then throws `std::bad_function_call` when it is fired. Three routes were
weighed:

| Route | Cost |
|---|---|
| **Taken:** `add_task()` refuses it and answers `false` | a runtime check on a rule the signature otherwise enforces statically; the caller learns while still on their own stack |
| Let it reach `call_tasks()` and become an error | no new code; the report names the wrong culprit |
| Ignore it | a task that silently never notifies, which is the defect this whole plan exists to remove |

**What settled it was a probe of the actions path, not a preference.** Asked how an empty callback
behaves there today, the answer turned out to be neither "ignored" nor "thrown":

```
callback is empty: yes
operator() returned normally
results: 1 (front = 7)
errors:  1
  recorded: std::bad_function_call
action still connected: yes
```

The action runs, its result is collected, and the empty callback's throw is caught by
`operator()`'s `catch (...)` and filed in actuator::errors. **So the existing behaviour is route
two** — and seeing it is what made route one the right answer for tasks rather than merely the
stricter one. The two situations differ in *when* the caller finds out: an action's callback is
optional and bites during the very call the caller made, while a task's is the point and would bite
inside `call_tasks()`, on a worker thread, arbitrarily later, as a failure recorded against a task
whose action had in fact succeeded. `add_task()` is the last moment the caller is still on the
stack, and the check is one `if`.

**Both halves are refused, not just the callback.** A hand-built task can carry a callback and no
`call`, and it throws `std::bad_function_call` out of `call_tasks()` in exactly the same way. The
guard is `if (!task || !task.callback)`. If only the callback should be guarded, it is that `!task ||`
and one case.

**`copy_from()` had to be told about tasks, and would have lost them in silence otherwise.** It
copies `owned`, `actions`, `actions_map`, `results` and `errors` **by hand** rather than defaulting,
so a member it does not name is simply dropped from every copy — a copied actuator would look like
one with nothing to do. Move is `= default`, so tasks ride along for free; asserted rather than
assumed, in `test_moving_an_actuator_carries_its_tasks`.

**No `remove()`, no named form, and that is the design rather than an omission.** An action is
stored by pointer because `remove()` needs identity and two `std::function`s cannot be compared; a
task is consumed by `call_tasks()` and never identified again. `actions_map` exists so
`invoke_action()` can fire one action on demand; tasks are fired as a batch, in the order they were
added.

**The strongest case here is `test_tasks_and_actions_live_side_by_side`**, which states the whole
two-kinds design in one place: after `actuator(5)` the action has run and its result is collected,
and the task has not run, has not notified, and is still in the list. Nothing in this step may
anticipate step 4.

> **Step 4 owes six `\ref call_tasks()`.** They are referenced from `tasks_t`, from `tasks`, from
> `add_task()` and from \ref task itself, all demoted to code font because the member does not
> exist yet. A bigger debt than the earlier ones, and the last of them.

> **Asked, and recorded outside this plan: could `add()` for actions be hardened the same way?**
> It is the same argument — the caller is still on the stack — but it changes the return type of a
> public API with callers in `async.hpp` and both suites, and `add(action_t&&)`'s handle return has
> nowhere to put a `false`. Deliberately not done here; let tasks prove the pattern, then raise it
> as its own step. The empty *callback* half is the sharper one and may be worth doing alone.

### Step 4 ✅ · `call_tasks()` — DONE

`actuator.hpp:638-693`. Eight cases at `test/actuator_test.cpp:1755-1921`. 95 of 95 green,
clang-format clean, doxygen clean, `doc/refman.pdf` at 58 pages.

**Six rules, all in the reference rather than left to be inferred.** The first four were written
before the step; the last two were settled by the cases, and are the ones this plan had left
implicit.

- **The callback always fires** on a task that returns normally. There is no `if` — that is what
  required means, and it is the one promise a caller gets from a task that an action never gave.
- **Finished does not mean failed. Decided 2026-09-25.** A task whose action throws gets no
  callback: there is no result to report, and for a void task no completion either. What it threw
  goes to actuator::errors. **Documented as a choice, not an omission** — a caller who reads "a task
  always notifies" will otherwise assume it notifies here too. Revisit if a use case asks; an
  `exception_ptr` overload is the obvious shape and nothing forecloses it.
- **A throwing callback lands in `errors`**, because it runs inside the same `try` as the task that
  owns it. So `errors` can hold a failure for a task whose action in fact succeeded. **The task is
  consumed either way**: re-running an action that already ran, to reach a callback that already
  threw, would be worse than losing the notification.
- **`call_tasks()` consumes.** Each task fires once and the list is empty afterwards — the one-shot
  half of the action/task distinction, and what removes the need for `remove()`.
- **A task's result goes to its callback and nowhere else.** actuator::results is how an *action*
  hands back what it returned; a task was built with something better and does not need both. It
  also keeps `results` meaning one thing.
- **Errors are appended, not written over.** `operator()` clears both lists as it starts and this
  clears neither, so an actuator fired as `one(); one.call_tasks();` reports both kinds together —
  which means **the actions go first**. The other order loses what the tasks recorded. This is the
  order async's drain uses anyway, so the constraint costs nothing; had `call_tasks()` cleared, that
  drain would have wiped the actions' results before reporting them.

**Two implementation choices that are not obvious from the rules.**

*The list is taken by swap rather than iterated in place.* A callback that adds a task adds it to
the **next** pass. Iterating the member would let a task that re-adds itself keep the loop from
ever ending — and it is the same reason a queue built on this takes its batch by move, so the two
layers agree rather than merely coexist.

*`one.callback(one())` is one expression on purpose*, so the result is handed over as an rvalue.
Split into `auto result = one(); one.callback(result);` it would pass an lvalue to a
`std::function<void(R)>` that takes `R` by value, and a result type that cannot be copied would
stop being reportable at all.

**The cases carry what no earlier step could**, which is the two kinds in one actuator:
`test_call_tasks_leaves_the_actions_alone` mirrors step 3's `test_tasks_and_actions_live_side_by_side`,
and `test_call_tasks_appends_to_errors_rather_than_clearing_them` states the ordering rule in the
only place it is observable. Struck step 7 was reaching for exactly these.

**The six `\ref call_tasks()` debts from steps 1 to 3 are paid.** Nothing in the header now names a
symbol that does not exist.

> **One unexplained doc failure, not reproduced.** The first `tools/make_doc.sh` of this step died
> in LaTeX pass 1 — "the XeTeX engine had an unrecoverable error" — and three runs since, one
> verbose and two plain, have all written the same 58-page PDF with nothing changed in between. Not
> called fixed, because it was never diagnosed. If it recurs, `-v` prints the TeX warnings; the
> script cleans up its intermediates, which is why there is no `refman.log` to read afterwards.

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

### Step 6 ✅ · `has_tasks()`, and `is_connected()` left alone — DONE

`actuator.hpp:827-857`. Four cases at `test/actuator_test.cpp:1923-1991`. 99 of 99 green,
clang-format clean, doxygen clean, `doc/refman.pdf` at 60 pages.

**Decided 2026-09-25: add `has_tasks()`, keep `is_connected()` exactly as it is.**

| Route | Cost |
|---|---|
| Extend `is_connected()` to include tasks | arguably correct — "does this actuator hold anything" — but it is also read by the attachment paths, so the meaning changes for callers that will never hold a task |
| **Taken:** add `has_tasks()`, and let a queue read that | two predicates to keep straight; `is_connected()` keeps its current meaning exactly |

**What the second route buys is that nothing already written changes its answer.**
`async::has_pending_actions()` (`async.hpp:686`) reads `is_connected()`, and so do the attachment
paths — `attach()`, `detach()`, `execution_poll`. None of those will ever hold a task. Extending
`is_connected()` would have changed what all of them are told in order to fix one caller, and the
one caller can simply ask the right question instead.

**The failure it prevents is the one async's step 3 was blocked on**: a queue reading
`is_connected()` to decide whether its worker still has work would report a batch of tasks as
nothing to do, break out of the drain, and leave the tasks unfired with the worker parked on its
condition variable.

**The case that earns its keep is `test_has_tasks_and_is_connected_are_not_the_same_question`**,
which asserts all four combinations — neither, tasks only, actions only, both. `tasks_only` is the
one that matters: `has_tasks()` true and `is_connected()` **false**. A queue reading the wrong
predicate either parks with work queued or spins on an actuator with nothing to do, and that is the
combination where it shows. `test_is_connected_is_unmoved_by_tasks` states the decision itself:
adding, holding and firing tasks never move that answer.

**`is_connected()` gained a remark and not a line of code**, saying it answers for the actions and
nothing else and pointing at \ref has_tasks(). The distinction is only obvious once both exist.

### Step 7 ✅ · the reference and the commits — DONE

**Done: `README.md` and `doc/refman.pdf`.** The reference PDF was rebuilt at every step rather than
once at the end, so it never drifted; it is at 62 pages, from 51 when step 1 started. `README.md`
gained a tasks section, because it documents the surface and a whole new kind of thing had appeared
in it — what a task is against an action, the callback being last and required, `void()` for a void
task, arguments copied at bind time, `add_task()` answering `bool`, finished not meaning failed,
and `has_tasks()` against `is_connected()`.

**Done: the commits — as one, `beb5fe8`.** Steps 1, 2, 3, 4 and 6 landed together, with the header,
the 36 cases, `README.md` and the reference.

> **One step per commit is what the method says, and this is not that.** The question was left open
> above and has been answered by what happened: the six steps arrived as one commit, because each
> was reviewed at its **red test** rather than at its commit, so the review the rule protects had
> already taken place five times over. Splitting afterwards would have meant carving six commits out
> of one tree with `git add -p`, for a history nobody reviewed in that shape.
>
> **What it costs is bisect granularity**, and it is worth naming: a defect in the mechanism now
> bisects to one commit of 977 added lines rather than to the step that introduced it. The step
> index above is what stands in for that, since every step names its own sites and its own cases.
>
> **async and `executor` will arrive the same way** unless decided otherwise, so this is the
> precedent rather than an exception.

**The bump was never this repo's, and listing it here was the error that kept this step open.**
`async` records the actuator's commit in `async`'s own tree, so moving that pointer is a change to
`async`, made in `async`, and its plan has owned it all along — its step 7, "`tools/make_doc.sh`,
the actuator bump, and the bump `executor` takes". Struck from here rather than tracked in two
places; a step that waits on another repo's commit can never close on its own terms.

> **This plan is itself inside `beb5fe8`**, so the hashes above were written after the fact, by the
> `chore: update feature plan` that follows it — as the method says, a commit cannot record its own
> hash.
>
> **And they were written three times.** `2bd224a` became `1e52939` when the commit message gained a
> body, and `1e52939` became `beb5fe8` on the next amend; both are unreachable from any branch now,
> and each rewrite left a dozen citations pointing at a commit that no longer existed.
>
> **The loop is the lesson, not the typo.** A hash written into the plan and then amended *into* the
> commit it names can never be right: the amend changes the hash the moment the citation lands.
> That is precisely why the method puts the plan's own update in a **later** commit — it is not
> bookkeeping etiquette, it is the only shape in which the number can be true. `executor`'s
> `FIX_PLAN.md` met the same thing at `bf7739b`, which became `4c1cba6` under an amend and left
> three dangling references behind it; this is that, three times over, and broken only by letting
> `chore: update feature plan` sit on top of the commit rather than inside it.



**Withdrawn 2026-09-25.** It listed the suite as a step of its own, which contradicts the working
method this plan inherits: *a step's case travels with its own fix*. Step 1 had already demonstrated
it — seven cases in the tree, red, before a line of the concept existed — so a later step collecting
"the tests" would either duplicate them or imply the earlier steps had shipped without any.

**What it was reaching for is real and belongs elsewhere:** the cases that cannot be attached to a
single step, because they are about tasks and actions in one actuator — a batch holding both, fired
by `operator()` and then `call_tasks()`, with `results` and `errors` carrying entries from both. That
is step 4's, since `call_tasks()` is what makes the combination observable, and it is recorded there
rather than deferred to a suite step.

### Struck — "the actions path has never been tested"

**Withdrawn 2026-09-25, the day it was written.** It claimed `grep -i callback` over `test/` and
`example/` returned nothing, and that four commits had built the actions callback convention without
leaving a case behind. **The claim is false.** The convention has eight cases at
`test/actuator_test.cpp:761-900`, among them `test_action_has_callback`,
`test_named_action_has_callback`, `test_anonymous_lambda_as_callback`,
`test_callback_with_return_type_is_not_accepted`, `test_action_callback_passed_as_rvalue` and
`test_every_action_receives_a_usable_callback` — which is to say every rule the convention has,
including the two that are easy to get wrong.

**Where the error came from is the part worth keeping.** The grep ran against a checkout of this
repo on `master`, a 374-line lineage on which `a8b8b47` is not an ancestor and the callback
convention does not exist at all. The suite there is 25 cases; on `main` it is 63. The reading was
correct about the file it read and wrong about the repository.

**Method, then:** a claim about what the suite does not cover is a claim about a specific checkout,
and it is worth naming the commit before recording it. The executor's `FIX_PLAN.md` already learned
the neighbouring lesson at its own step 22 — a conclusion reached by reading an interface instead of
compiling against it was the one it had to retract. This is the same shape with a different cause:
not a reading of the wrong kind, but a reading of the wrong tree.

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
| `beb5fe8` | 1, 2, 3, 4 and 6 — the whole mechanism, 36 cases, README and the reference |

**CLOSED.** Every step is done, declined or struck, and all of it is in `beb5fe8`. 99 of 99 green,
clang-format clean, doxygen clean, `README.md` and `doc/refman.pdf` current. Nothing in this repo is
outstanding and nothing here blocks anything.

**NEXT is async's step 1**, which this repo no longer gates — its step 6 answered the predicate
question. Read async's step 5 first, on the `bind_action_and_method()` wrapper, and its step 4, on
what `is_busy()` and `on_finished` mean once a pass can run only tasks.
