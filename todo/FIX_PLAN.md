# actuator.hpp — fix plan

**Status (2026-09-04):** 17 of 23 steps done, plus findings G (snippets) and H (doc warnings).
Everything through step 25 is committed except **step 18**, which is applied and awaiting commit
(`actuator.hpp` — two `static` keywords dropped; `doc/refman.pdf` regenerated with it).
**Tests:** 23/23 green — `cd test/build && cmake --build . && ./bin/actuator_test` (baseline was 11/11)
**Docs:** 0 doxygen warnings; `doc/refman.pdf` is 33 pages (was 21).
**Source:** findings in `todo`, verified 2026-09-02 by compiling and running probes.

## Progress

Done — steps 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 22, 23. Step 7 was folded into step 8.
Plus one unplanned change: C++17 -> C++20 and googletest v1.16.0 -> v1.18.0 (commit 626acf1),
which cleared a `char8_t` -> `char32_t` warning coming from googletest's own headers.

| Commit | Step |
|---|---|
| `f8be7a1` | 1 — reset() compiles and clears all three containers |
| `fe14277` | 2 — self-assignment guard |
| `e0d0484` | 3 — operator= copies results |
| `291ea3c` | 4 — bind() captures weak_ptr by value, not a dangling reference |
| `b0317ac` | 5 — dropped mutable on the shared_ptr lambda |
| `626acf1` | (extra) C++20 + googletest v1.18.0 |
| `523a5d1` | 6 — operator() no longer nulls the caller's std::function |
| `ae88448` | 22 + 23 — full snake_case rename |
| `4246f6a` | 7 + 8 — null / empty actions dropped instead of terminating |
| `c8cfd08` | 9 — invalid_action publicly inherits and overrides what() |
| `2c9f46b` | 10 — invoke_action mirrors operator() with `if constexpr` |
| *(uncommitted)* | 11 — deleted the dead select_actuate pair |
| *(uncommitted)* | 12 — rule of five; actuator moves instead of copying |
| *(uncommitted)* | (amends 2+3) — copy assignment defaulted |
| *(uncommitted)* | 24a — \snippet filename corrected to actuator_test.cpp |

**NEXT: step 13** — item 7, `return std::move(local)` defeats NRVO (`:294`, `:323`). Now that
step 12 has made `actuator` genuinely movable, this one is finally about NRVO rather than about
hiding a copy.

**Remaining: nothing.** Every finding in this plan is closed.

**`actuator.hpp` itself is complete** — every finding against the header is closed.

**Planned separately:** a `clang-format` pass will normalise layout across the repo, so brace style and indentation inconsistencies are deliberately not tracked as findings here.
Step 17 is optional and skippable. Item 11 stays rejected.

21 atomic steps. **One step = one commit = one concern**, and the suite must be green after
every one. Original `todo` numbering is preserved so items stay traceable; findings added
during verification are lettered.

**Granularity rule:** steps split by *concern*, not by *edit count*. Where one concern
touches several sites (item 7's two `connect` overloads, item 8's three observers, item C1's
two `bind` overloads) it stays one step — splitting those would leave the header internally
inconsistent for a commit, which is the opposite of atomic. Every such case is flagged.

---

## Step index

| # | Item | Concern | Sites | Verified |
|---|---|---|---|---|
| **Group 1 — bugs** |
| 1 ✅ | 1 | `reset()` does not compile | `:107-110` | CONFIRMED |
| 2 ✅ | 3 | self-assignment wipes the actuator | `:98` | CONFIRMED |
| 3 ✅ | E | `operator=` skips `results` | `:98-105` | read-only |
| 4 ✅ | 2 | `bind(shared_ptr)` captures a dangling reference | `:363-378` | CONFIRMED |
| 5 ✅ | 10a | dead `mutable` on the `shared_ptr` lambda | `:366` | read-only |
| 6 ✅ | 4 | `operator()` nulls the caller's `std::function` | `:139-143` | CONFIRMED |
| 7 ✅ | A | *(folded into 8)* `add(nullptr)` segfaults | `:148` (+`:292` defensive) | CONFIRMED |
| 8 ✅ | F | empty action terminates the process | `:123-145` | CONFIRMED |
| **Group 2 — exception type** |
| 9 ✅ | 5 | `invalid_action` uncatchable as `std::exception` | `:31-41,141,171` | CONFIRMED |
| **Group 3 — finish the `if constexpr` migration** |
| 10 ✅ | 6a | `invoke_action` still routes through SFINAE *(refactor)* | `:191-211` | limitation only |
| 11 ✅ | 6b | delete the dead `select_actuate` pair | `:283-303` | — |
| **Group 4 — API and performance** |
| 12 ✅ | B | `~actuator()` suppressed implicit moves *(needed rule of five)* | `:80-104` | CONFIRMED |
| 13 ✅ | 7 | `return std::move(local)` defeats NRVO | `:291,:320` | CONFIRMED |
| 14 ✅ | 8 | observers are not `const` | `:100,:253,:262` | CONFIRMED |
| 15 ✅ | 9 | wrong doc comment on the actions map *(+ rename)* | `:66,:77` | CONFIRMED |
| 16 ✅ | 10b | dead `mutable` on the raw-pointer lambda | `:397` | CONFIRMED |
| 17 ✅ | 12 | raw-pointer `bind` was assert-only | `:394-406` | CONFIRMED |
| 18 ✅ | C1 | `bind` is `static` in a header | `:363,:394` | CONFIRMED |
| 19 ✅ | C2 | `<string>` used but not included *(+ by-value params)* | `:8-18,:174,:215` | CONFIRMED |
| **Group 5 — example (unrelated, optional)** |
| 20 ✅ | D1 | example includes a nonexistent header | `example.cpp:8` | CONFIRMED |
| 21 ✅ | D2 | example pins C++14 *(bumped to 20, not 17)* | `example/CMakeLists.txt:3` | CONFIRMED |
| **Group 6 — naming alignment** |
| 22 ✅ | N1 | internal camelCase -> snake_case | locals + template params | — |
| 23 ✅ | N2 | public camelCase -> snake_case *(API break)* | members, methods, aliases | — |
| **Group 7 — documentation** |
| 24a ✅ | G | `\snippet` names a file that does not exist | `:88,201,227,275,276` | CONFIRMED |
| 24b ✅ | G | `\snippet` marker pairs missing from the test | `actuator_test.cpp` | CONFIRMED |
| 24c ✅ | G | `remove()` documented with the `add` snippet | `:227` | CONFIRMED |
| 25 ✅ | H | 8 doxygen warnings in the header's doc comments | `:25,268,355,383,388` | CONFIRMED |
| 26 ✅ | I | Doxyfile has no `INPUT`/`OUTPUT_DIRECTORY` | `Doxyfile:61,802,876,1118` | CONFIRMED |

---

## Blocking decision — needed before Step 9 only

Item 5 wants `invalid_action` to inherit publicly and replace the `std::string what` data
member with a `const char* what() const noexcept` override.

**A class cannot have a data member `what` and a member function `what()`.** The member
currently *hides* the inherited virtual `std::exception::what()` — precisely why `e.what()`
fails today. There is no non-breaking version of this fix.

| Option | `catch (const std::exception&)` | `e.what()` | `e.what` |
|---|---|---|---|
| **1. Clean break** — public inheritance + real override | matches, real message | works | **gone** |
| **2. Proxy member** — `what` gains `c_str()`, string conversion, `operator()()` | matches, prints `"std::exception"` | works | works |
| **3. Inheritance only** — keep the member | matches, prints `"std::exception"` | ill-formed | works |

**Recommendation: option 1.** `.what` appears exactly twice in the repo — `actuator.hpp:141`
and `:171`, both inside the header. No other consumers exist in tree. The break costs two
lines. Options 2 and 3 leave generic handlers printing a useless default, which is most of
what item 5 was for.

Steps 1–8 do not depend on this. Only Step 9 blocks.

---

## Group 1 — bugs

### Step 1 · item 1 — `reset()` does not compile
`actuator.hpp:107-110` · CONFIRMED: `error: no member named 'reset' in 'std::list<...>'`

`actions.reset()`; `std::list` has no `reset()`. As a member of a class template it is only
instantiated when called, so it is a latent landmine rather than a build break.

> `actions.clear(); mapActions.clear(); results.clear();`

### Step 2 · item 3 — self-assignment wipes the actuator
`actuator.hpp:98` · CONFIRMED: `actions.size()` goes 1 → 0 across `ac = ac;`

`operator=` clears, then copies from the just-cleared self.

> `if (this == &other) return *this;` at the top.

**Test:** `TEST(test_actuator, test_self_assignment)`.

### Step 3 · item E — `operator=` skips `results`
`actuator.hpp:98-105` · read-only

The implicit copy constructor copies `results`; `operator=` does not, so copy-construct and
copy-assign produce different objects from the same source. Separate concern from Step 2 —
that one is a correctness guard, this one is copy completeness.

> Add `results = other.results;`, or document the omission deliberately. Pick one.

**SUPERSEDED 2026-09-04 (applied, uncommitted).** `operator=(const actuator&)` is now
`= default`, which subsumes both this step and step 2:

- The compiler enumerates the members, so `results` cannot be skipped again — this whole class
  of bug is gone rather than patched. Ditto for any member added later.
- Step 2's self-assignment guard is unnecessary for memberwise assignment: each standard
  container's own `operator=` is self-safe. Probed on `list` + `map` + `vector`, contents
  intact after an unguarded `p = p`.

Moves are unaffected — a defaulted copy assignment is still *user-declared*, so the explicit
move members from step 12 remain necessary. Verified 23/23 green, `copy_ctor=1 copy_assign=1
nothrow_move_ctor=1 nothrow_move_assign=1`, 0 element copies on both move paths, clean under
`-Wall -Wextra -Wunused-parameter`.

The removed doc block took a `\snippet test_actuator.cpp test_assignment` reference with it,
which was **already dangling** — no such marker exists in the test file. `operator=` is now
undocumented in Doxygen; a one-line `@brief` would restore parity if wanted.

Commit separately from steps 11-12: this edits code that step 3 already committed.

### Step 4 · item 2 — `bind(shared_ptr)` captures a dangling reference
`actuator.hpp:363-378` · CONFIRMED: probe returned `1172321806` where `42` was expected

Captures `[&obj, method]` — a reference to `bind`'s own parameter, which the returned lambda
outlives. Silent use-after-free. The current tests survive only because every caller happens
to keep a named `shared_ptr` alive in the same scope.

> Capture `std::weak_ptr` **by value**; `lock()` inside the lambda; throw `invalid_action`
> when the lock fails.

Preserves the `obj.reset()` → `invalid_action` behavior that `test_invalid_action` and
`test_polymorphism_named_actions` depend on. One semantic change to document in the doxygen
`@remark`: `bind(std::make_shared<T>(), &T::m)` becomes an action that is *always dead*
rather than undefined behavior.

**Test:** `TEST(test_actuator, test_bind_temporary_shared_ptr)`.

### Step 5 · item 10a — dead `mutable` on the `shared_ptr` lambda
`actuator.hpp:366` · read-only

Nothing in the body mutates a capture. Split from Step 4: that is a safety fix, this is
removing a dead keyword. Landing them separately keeps Step 4's diff purely about lifetime.

> Drop `mutable`.

### Step 6 · item 4 — `operator()` nulls the caller's `std::function`
`actuator.hpp:139-143` · CONFIRMED: caller's `action2` goes non-empty → empty after `ac(5)`

On a dead binding the catch does `*action = nullptr;`, writing through the stored `actionT*`
into an object the **caller** owns. `invokeAction` (`:172`) handles the same situation by
erasing only its own map entry — the two paths disagree about whose state they may touch.

> Drop `*action = nullptr;`. Collect dead entries into a local `std::vector<actionT*>` during
> the loop, then erase them from `actions` after it.

**Note for Step 7:** doing this deletes the unguarded `remove_if` at `:148`, which is where
finding A actually crashes. Step 7 shrinks to a guard on the surviving loop condition.

### Step 7 · finding A — `add(nullptr)` segfaults
`actuator.hpp:148` · CONFIRMED: UBSan null-reference, then ASan SEGV
*Not in the original review.*

`operator()`'s loop guards `if (action)`, but the `remove_if` right after dereferences
unconditionally: `return (*action == nullptr);`

**Correction to an earlier claim:** `connect`'s `remove_if` at `:292` has the same *shape*
but is **not** reachable with a null — it builds from `{&A1, &An...}`, addresses of
references. Guarding it is defensive tidiness, not a bug fix.

> Guard the pointer wherever it is dereferenced: `action != nullptr && ...`. After Step 6
> that is one place.

**Test:** `TEST(test_actuator, test_add_null_action)`.

### Step 8 · finding F — an empty action terminates the process
`actuator.hpp:123-145` · CONFIRMED: uncaught `std::bad_function_call`, exit 134
*Not in the original review.*

`if (action)` tests the **pointer**, not the function. Invoking an empty `std::function`
throws `std::bad_function_call`, and the handler catches only `invalid_action`, so it escapes
`operator()` and terminates. `connect` filters empty actions at construction, which is the
only reason no existing test reaches this — `add()` applies no such filter.

> Test the function too: `if (action == nullptr || !*action) { /* mark dead */ continue; }`
> and let the removal pass from Step 6 drop it.

**Test:** `TEST(test_actuator, test_add_empty_action)`.

---

## Group 2 — exception type

### Step 9 · item 5 — `invalid_action` is uncatchable as `std::exception`
`actuator.hpp:31-41`, call sites `:141`, `:171` · CONFIRMED: probe fell through to `catch (...)`
**Blocked on the decision above.**

`struct invalid_action : private std::exception`. Private inheritance makes the base
inaccessible for handler matching, so `catch (const std::exception&)` does not match.
Separately, the `std::string what` member shadows the virtual `what()`.

> Apply the chosen option; update `:141` and `:171` in the same commit — they will not
> compile otherwise, so this cannot be split further.

---

## Group 3 — finish the `if constexpr` migration

### Step 10 ✅ · item 6a — `invoke_action` still routes through SFINAE
`actuator.hpp:191-207` · **REFACTOR, not a bug** — no in-tree behavior is wrong today

`operator()` was modernized in 47ee6db; `invoke_action` still calls `select_actuate`. Every
existing instantiation compiles and produces correct results, so nothing observable is
broken. What the SFINAE path costs is:

- **A limitation.** The non-void overload ends `return typename T::result_type();`, so
  `invoke_action` will not instantiate for a result type without a default constructor.
  Latent — it surfaces only when someone writes such a type.
- **Waste.** That return default-constructs a result which the `void`-returning caller
  discards. Observable only if the result type has a side-effecting default constructor.

> Mirror `operator()`: `if constexpr (std::is_same_v<typename action_t::result_type, void>)`.

**Test:** `TEST(test_actuator, test_invoke_action_non_default_constructible_result)` — a
*capability* test, not a regression test. It demonstrates a constraint being lifted and pins
the header to `if constexpr`; it is not evidence that a defect existed. Before the rewrite it
fails as a compile error at `actuator.hpp:297`, which takes the whole translation unit with
it, so the suite cannot run at all until the rewrite lands.

**Commit prefix:** `refactor:`, matching `ae88448` — not `fix:`.

**APPLIED 2026-09-04**, not yet committed. Five lines at `actuator.hpp:199-203` mirroring
`operator()`; diff is 6 insertions / 1 deletion, CRLF intact. Suite 21/21 -> 22/22 green.
`test_polymorphism_named_actions2` still passes, confirming the void path and the
`catch (const invalid_action&)` map-erase behaviour are unchanged.

### Step 11 ✅ · item 6b — delete the dead `select_actuate` pair
`actuator.hpp:283-303`

Step 10 removes the only caller. Separate commit: Step 10 is a behavior-preserving rewrite,
this is a pure deletion, and the deletion is only valid once Step 10 has landed.

> Delete both overloads and the now-stale `// todo: to be removed?` comment at `:161-163`.

**APPLIED 2026-09-04**, not yet committed. 26 deletions, no insertions, CRLF intact. The
`private:` label went with them — nothing else in `actuator` was private, so leaving it would
have left an empty access section. `<type_traits>` stays included: `is_same_v` is still used by
`operator()` and `invoke_action`, and `is_void` by the `result_t` alias. Suite still 22/22.

Group 3 is now complete — no `enable_if` or SFINAE remains anywhere in the header.

---

## Group 4 — API and performance

### Step 12 ✅ · finding B — `~actuator()` suppressed implicit moves
`actuator.hpp:80-104` · CONFIRMED by probe: 2 element copies per move, now 0
*Not in the original review.*

**The original prescription ("delete `~actuator()` entirely") was wrong — it fixes nothing.**
Measured with an instrumented result type across five header variants:

| variant | copy ctor | moves real? |
|---|---|---|
| before | yes | no — 2 copies |
| delete `~actuator()` *(the original advice)* | yes | **no — 2 copies** |
| delete dtor + `operator= = default` | yes | no — 2 copies |
| add move members, keep dtor | **deleted** | yes |
| **rule of five** *(applied)* | yes | yes — 0 copies |

Two facts the original missed:

1. **The destructor is not the only suppressor.** `operator=(const actuator&)` is user-declared
   and suppresses the implicit move members by itself, so removing the destructor changes
   nothing measurable. `= default` does not help either — a function defaulted on its first
   declaration is still *user-declared*.
2. **Declaring a move constructor deletes the implicit copy constructor**, which breaks
   `test_assignment`'s `constructed = source` at `actuator_test.cpp:326` — a line step 3 added.
   The copy constructor must be spelled out too.

> Applied: `actuator() = default;`, `actuator(const actuator&) = default;`,
> `actuator(actuator&&) noexcept = default;`, `~actuator() = default;`, the existing guarded
> `operator=(const actuator&)`, and `actuator& operator=(actuator&&) noexcept = default;`.
> `type()` moved below them so all six special members read as one block.

The explicit `noexcept` on the defaulted moves is safe here — if it disagreed with the implicit
spec the function would be silently *deleted*, but both report `nothrow == 1` and the moves run.

**Test:** `TEST(test_actuator, test_move_does_not_copy)`. It counts element copies, because
`std::is_move_constructible_v<actuator>` was **true even before the fix** — the copy constructor
satisfies it. `static_assert`s pin copyability so the deleted-copy-ctor trap cannot return.

### Step 13 ✅ · item 7 — `return std::move(local)` defeats NRVO
`actuator.hpp:291`, `:320` · CONFIRMED by instrumenting `actuator`'s own move ctor
· **APPLIED, uncommitted**

Returning a named local by `std::move` blocks copy elision.

> Applied: `return actuator;` in both overloads.
> *One concern, two sites — kept together; a half-applied idiom fix is worse than none.*

**But on Apple clang 21 this alone changes nothing measurable — `auto` was already blocking
NRVO.** Measured moves of the actuator on returning from `connect`:

| variant | moves |
|---|---|
| `auto` + `return std::move(actuator);` *(before)* | 1 |
| `auto` + `return actuator;` *(step 13, applied)* | **1** |
| `actuator<action_t>` + `return actuator;` | **0** |

Narrowing it down, the blocker is the **deduced return type on a function template**, not the
dependent local type:

| shape | moves |
|---|---|
| `template<T> actuator<T> f() { actuator<T> a; return a; }` | 0 |
| `template<T> auto f() { actuator<T> a; return a; }` | 1 |
| `template<T> auto f() { ac_t a; return a; }` | 1 |
| `template<T> ac_t f() { ac_t a; return a; }` | 0 |
| non-template `auto f()` | 0 |

Same at `-O0` and `-O2`. A non-template `auto f()` elides fine, so this is specifically a missed
optimization for deduced return types on templates.

Step 13 is still right — `std::move` on a returned local forecloses NRVO permanently, while
`return actuator;` lets any compiler that can elide do so. Committed as `44de039`.

### Step 13b ✅ — spell out `connect`'s return type · **APPLIED, uncommitted**

`auto` -> `untangle::actuator<action_t>` on both overloads (`:280`, `:312`), which is what makes
step 13 pay off. Measured on the real header with an instrumented move ctor: **0 moves at both
`-O0` and `-O2`**, down from 1. 23/23 green, 0 doxygen warnings, `doc/refman.pdf` regenerated.

Separate commit from step 13: that one fixes an idiom, this one changes a declaration.

### Step 14 ✅ · item 8 — observers are not `const`
`actuator.hpp:100`, `:253`, `:262` · CONFIRMED: 4 compile errors on a `const actuator`
· **APPLIED, uncommitted**

`type()`, `is_connected()`, `has_action()` could not be called through a `const actuator&`.

> Applied: all three marked `const`; `has_action(std::string)` -> `const std::string&`, which
> also stops copying the key on every lookup. *One concern, three sites — kept together.*

**Test:** `TEST(test_actuator, test_const_observers)`. It is a *compile-time* proof — before the
fix the translation unit failed with 4 errors ("`this` argument ... but function is not marked
const") covering `is_connected`, `has_action` twice, and `type`, and the whole suite could not
run. 23/23 -> 24/24 after. `doc/refman.pdf` regenerated; 0 doxygen warnings.

### Step 15 ✅ · item 9 — wrong doc comment on the actions map
`actuator.hpp:66`, `:77` · CONFIRMED by reading · **APPLIED, uncommitted**

The map was documented `//!< Actions list.` — the same text `:76` already uses for the actual
list. A copy-paste slip; no test can prove a comment, so the duplicated text was the evidence.

> Applied: `//!< Named actions map.`

**Plus a rename the user asked for in the same breath:** `map_actions` -> `actions_map` and the
alias `map_actions_t` -> `actions_map_t`. 19 occurrences across 16 lines, all inside
`actuator.hpp` — the tests never referenced the member, so nothing outside the header moved.
Public API break, consistent with step 23's naming work. 24/24 green, 0 doxygen warnings,
`doc/refman.pdf` regenerated.

**Left open — the aliases at `:66` and `:67` (`actions_map_t`, `result_t`) carry no doc comment**
while their siblings `actions_t` and `results_t` each have a full block. That is *missing* docs
rather than *wrong* docs, so it wants its own step if the user wants it at all.

### Step 16 ✅ · item 10b — dead `mutable` on the raw-pointer lambda
`actuator.hpp:397` · CONFIRMED · **APPLIED, uncommitted**

Same dead keyword as Step 5, different overload. Separate step because Step 5 rides along
with a lifetime fix and this one stands alone.

> Applied: dropped `mutable`. No `mutable` remains anywhere in the header, so the two `bind`
> overloads finally agree.

**Verified two ways.** The whole suite compiles clean against a header with the keyword removed,
so nothing in either lambda ever mutated a capture. And it was not merely inert — `mutable`
makes the closure's `operator()` non-`const`:

| closure | `operator()` const-callable |
|---|---|
| `[x](int a) { ... }` | yes |
| `[x](int a) mutable { ... }` | **no** |

Nothing in the current path hit that (`std::function::operator()` reaches its target through a
non-const path), but it was a restriction imposed for no benefit — against the grain of step 14's
const-correctness work. 24/24 green, 0 doxygen warnings, `doc/refman.pdf` regenerated.

### Step 17 ✅ · item 12 — raw-pointer `bind` was assert-only
`actuator.hpp:394-406` · CONFIRMED: the proving test aborted the whole suite
· **APPLIED, uncommitted** *(the "optional" step was taken)*

`obj` is captured **by value**, so it cannot *become* null after binding — the original `assert`
covered binding time. The gap was that `assert` compiles out under `NDEBUG`, leaving a raw
dereference. Two bad outcomes depending on build mode: **abort** (debug) or **undefined
behaviour** (release). Neither catchable.

> Applied: `assert` removed; the lambda checks `obj == nullptr` and throws `invalid_action`.
> A null binding is now an *always-dead action*, exactly what step 4 made
> `bind(std::make_shared<T>(), &T::m)` — the actuator catches it and drops the action.

**Test:** `TEST(test_actuator, test_bind_null_pointer_is_a_dead_action)`. Before the fix it did
not merely fail — the `assert` aborted the process, so no other test ran. Verified afterwards at
both `-O0` and `-O2 -DNDEBUG`: identical behaviour, throws and the actuator drops the action.
That equality across build modes is the point of the step.

**Two consequences handled with it:**

- `<cassert>` became unused and was removed — the include-what-you-use principle from step 19,
  applied in reverse.
- The `@attention` said the pointer "can not be checked if it gets null", which the fix made
  false. Reworded: a null *is* detected; a pointer to an already-destroyed object still cannot
  be distinguished from a valid one, and that remains the real hazard. Added a `@remark`
  documenting the always-dead behaviour.

25/25 green, 0 doxygen warnings, `doc/refman.pdf` regenerated.

### Step 18 ✅ · item C1 — `bind` is `static` in a header
`actuator.hpp:363`, `:394` · CONFIRMED by symbol inspection · **APPLIED, uncommitted**
*Not in the original review.*

Both overloads were `static` at namespace scope, giving them internal linkage and a separate
copy per translation unit. Templates need no such marking.

> Dropped `static` from both. *One concern, two sites — kept together.*

**This also fixed the last doxygen warning** (step 25's `:25`). `\ref bind()` was unresolvable in
every form tried — qualified, bare, and with a full signature — because a `static` namespace-scope
function is filed by doxygen as a *file-static* member, out of `\ref`'s reach. `connect` is not
`static`, which is why the identical `\ref connect()` works at `:52` and `:346`. The doc warning
was a symptom of this linkage bug, not a separate defect.

**Verified by symbol class, not by "it linked".** Two TUs both instantiating `bind` link either
way, so a successful link proves nothing — with `static` each TU simply keeps a private copy and
there is nothing to collide. `nm -C` on the same TU shows what actually changed:

| | symbol class |
|---|---|
| without `static` | `T` — external, merged across TUs |
| with `static` | `t` — local, one private copy per TU |

That is why the bug is silent: it never fails a build, it just duplicates every instantiation
into every including TU.

### Step 19 ✅ · item C2 — `<string>` used but not included
`actuator.hpp:8-18` · CONFIRMED by audit · **APPLIED, uncommitted**
*Not in the original review.*

`std::string` is used at **7 sites** (`:36`, `:46`, `:66`, `:173`, `:214`, `:242`, `:262`) and
`<string>` was never included. Separate concern from Step 18 — that is linkage, this is a
missing dependency.

> Applied: `#include <string>` after `<map>`.

**Not demonstrable on this toolchain, and that is worth recording rather than faking.** Five of
the existing includes each pull `<string>` in on libc++ — `<vector>`, `<list>`, `<map>`,
`<functional>`, `<iostream>` — so no realistic edit exposes it here. The defect is latent: it
bites on another standard library, on a libc++ that trims its internal includes, or if
`<iostream>` is ever dropped (`std::cout` in `operator()` is its only user).

**Plus a by-value fix the user asked for in the same breath.** `invoke_action` (`:173`) and
`add` (`:214`) took `std::string name` **by value**, copying the key on every call, while
`remove` and `has_action` already took `const std::string&` — step 14 had fixed the latter and
left these two inconsistent. Both now take `const std::string&`.

`invalid_action(std::string text) : message(std::move(text))` at `:36` was **left by value on
purpose** — that is the sink idiom, the parameter is consumed into the member, and `const&`
there would force a copy.

24/24 green, 0 doxygen warnings, `doc/refman.pdf` regenerated.

---

## Group 5 — example (unrelated, optional)

### Step 20 ✅ · finding D1 — example includes a nonexistent header
`example/example.cpp:8` · CONFIRMED: `fatal error: 'actuator.h' file not found`
· **APPLIED, uncommitted**

`#include <actuator.h>` — no such file; the header is `actuator.hpp`. Because compilation
stopped here, step 21 was invisible behind it.

> Applied: `#include <actuator.hpp>`.

### Step 21 ✅ · finding D2 — example pins C++14
`example/CMakeLists.txt:3` · CONFIRMED: 8 errors at C++14, 0 at C++20 · **APPLIED, uncommitted**

With only the include fixed, the C++14 pin produced 8 errors — all `if constexpr` and
`std::is_same_v`, the C++17 features steps 6 and 10 introduced.

> Applied: **bumped to 20, not the 17 the plan called for.** The repo moved to C++20 in
> `626acf1` and the test suite builds at `gnu++20`; 17 would leave the example as the only
> component on a different standard.

**Verified beyond compiling:** the example runs to completion, exit 0, and is clean under
`-fsanitize=address,undefined` (0 findings). Its output shows the current header at work —
the dead-binding path prints `bind: invalid object`, the unified message from step 17.

**`example/` is a separate project by design** — it keeps its own standalone `CMakeLists.txt`
and is built on its own, not from the test build or CI. That is the user's decision, not an
oversight; when it breaks, fix it directly, as these two steps did.

---

## Group 6 — naming alignment

The header is inconsistent today: `is_connected` and `has_action` are snake_case, while
`mapActions`, `invokeAction` and the `actionT` / `resultsT` type aliases are camelCase.
The convention going forward is **lower_case_with_underscores**. Split into two steps by
blast radius, and sequenced last so the rename does not churn every functional diff before
it.

### Step 22 · N1 — internal names
No API impact; safe to do unilaterally.

| Name | Count | Kind |
|---|---|---|
| `removableIterators` | 3 | local in `removeEmptyActions` |
| `actionT` | 28 | template parameter |
| `classT` | 7 | template parameter |
| `actuatorT` | 7 | template parameter / local alias |
| `keyT` | 2 | template parameter |

> Template parameters are internal to the header, but the rename is large and touches almost
> every line. Consider whether `actionT` -> `action_t` is worth the diff, or whether template
> parameters should be treated as a separate naming class and left alone.

### Step 23 · N2 — public names *(breaking)*
Every one of these is reachable by user code, and `actuator_test.cpp` uses several.

| Name | Count | Kind |
|---|---|---|
| `mapActions` | 18 | public data member |
| `invokeAction` | 1 | public method |
| `removeEmptyActions` | 2 | public free function in `untangle` |
| `actionsT`, `mapActionsT`, `resultsT`, `resultT` | 8 | public type aliases |

> Needs explicit sign-off, like item 5. Renaming these breaks any downstream caller. Same
> mitigating fact as item 5 though: the only in-tree consumers are `actuator_test.cpp` and
> the already-broken `example/`.

---

## Not doing

### item 11 — specialize `resultsT` for void actions · REJECTED

`std::conditional_t<is_void, std::tuple<>, std::vector<...>>` breaks the public API:
`operator()` calls `results.clear()` unconditionally, and `actuator_test.cpp:416` and `:442`
both assert `results.size() == 0` on *void* actuators. `std::tuple<>` has neither `clear()`
nor `size()`. What it saves is an empty `std::vector<int>` — 24 bytes inline, zero heap,
since nothing pushes into it on the void path. Not worth an API break.

---

## Verification appendix

Apple clang 21.0.0, `-std=c++17`, several probes under `-fsanitize=address,undefined`.

| Item | Probe | Result |
|---|---|---|
| 1 ✅ | call `a.reset()` | `error: no member named 'reset' in 'std::list<...>'` |
| 3 ✅ | `ac = ac;` | `actions.size()` 1 → 0 |
| 2 ✅ | `bind` from a `shared_ptr` local to a factory, call after return | returned `1172321806`, expected `42` |
| 4 ✅ | `q.reset(); ac(5);` then inspect caller's action | caller's `std::function` non-empty → empty |
| 5 ✅ | `throw invalid_action` / `catch (const std::exception&)` | not matched; fell to `catch (...)` |
| 6 ✅ | `invokeAction` with a non-default-constructible result | hard error at `actuator.hpp:265` |
| 8 ✅ | call observers on a `const actuator` | 2 compile errors |
| A | `add(nullptr); ac(1);` | UBSan null-reference at `:148`, then SEGV |
| F | `add(&empty_function); ac(3);` | uncaught `std::bad_function_call`, exit 134 |

---

## Group 7 — documentation

### Step 24 · finding G — every `\snippet` reference is broken
`actuator.hpp:88,201,227,275,276` · CONFIRMED by matching references against markers
*Not in the original review. Found 2026-09-04.*

Two independent defects. **24a is applied; 24b is not.**

**24a ✅ (applied, uncommitted) — wrong filename.** All five references named
`test_actuator.cpp`. The file is `test/actuator_test.cpp`, and `EXAMPLE_PATH = test` in the
Doxyfile, so every snippet resolved to nothing. Corrected to `actuator_test.cpp`.

**24b (pending) — missing marker pairs.** `\snippet` needs the region bracketed by two
`//! [name]` lines. With the filename now right, four of the five still fail:

| reference | markers in the test | test that should carry them |
|---|---|---|
| `test_assignment` (`:88`) | 0 | `actuator_test.cpp:256` |
| `test_add` (`:201`, `:227`) | 0 | `:330` |
| `test_polymorphism1` (`:275`) | 1 — opening only, at `:127` | — |
| `test_polymorphism2` (`:276`) | 0 | — |

Four *complete* pairs exist that nothing references: `test_extract_results`,
`test_polymorphism_named_actions2`, `test_void_return_and_args`, `test_void_return_no_args`.
The named tests all exist, so the markers were most likely lost in the same rename that broke
the filename; `test_polymorphism1:127` is the surviving half of one.

**24b ✅ APPLIED 2026-09-04**, not yet committed. Added the four pairs — 7 marker lines, since
`test_polymorphism1` only needed its closing half. All four references now resolve:

| snippet | wraps | lines |
|---|---|---|
| `test_polymorphism1` | the `rotate_shapes` helper — manual polymorphism | 7 |
| `test_polymorphism2` | the bind/connect/invoke block in `test_polymorphism_using_shared_pointers` | 6 |
| `test_assignment` | the whole `test_assignment` body | 18 |
| `test_add` | the whole `test_add` body | 18 |

`test_polymorphism1` + `2` are the contrast `connect()`'s doc was written for: the hand-rolled
`std::vector<shape*>` loop, then the actuator equivalent. Whole-body wrapping for the other two
matches the existing convention (`test_extract_results`, `test_void_return_*`).

**24c ✅ (applied, uncommitted) — `remove()` documented itself with an `add` example.**
`actuator.hpp:227` referenced `test_add`, the same snippet `add()` uses at `:201`. Added a
`test_remove` marker pair and repointed the reference. All five snippets now resolve, confirmed
by an actual doxygen run: zero snippet warnings, and `rotate_shapes` / `actuator_rotate.add` /
`actuator_rotate.remove` / `actuator_rotate_1` all appear in the output.

**Not verified by Doxygen** — it is not installed on this machine. The table was built by
matching marker names to references. `doxygen -q` elsewhere would confirm with
`warning: unable to resolve reference to ...`.

### Step 25 · finding H — eight doxygen warnings in the doc comments
`actuator.hpp:25,268,354,382,387` · CONFIRMED by doxygen 1.18.0
*Found 2026-09-04, once the `\snippet` fixes made real warnings visible.*

| line | warning |
|---|---|
| `:25` | `\ref bind()` unresolved — `\ref` does not take the `()` |
| `:268` | `@param A1..An` does not match the real parameters `A1` and `An`; one is left undocumented |
| `:354`, `:387` | `&<class type>::<function member>` — `<class>` and `<function>` parsed as HTML tags |
| `:382` | `\ref actuator.The` unresolved — a missing space after `\ref actuator` swallowed the next word |

`:382` is the worst: the `@attention` sentence runs together *and* the reference breaks.
**APPLIED — committed as `176fb72`** (7 of 8) **and step 18** (the 8th).

| line | fix |
|---|---|
| `:268` | split into `@param A1` and `@param An`, matching the real signature |
| `:355`, `:388` | escaped to `&\<class type\>::\<function member\>` |
| `:383` | `\ref actuator . The` — reference resolves and the fused sentence is separated |
| `:25` | needed step 18; see there — `\ref` cannot reach a `static` namespace function |

Doxygen now reports **0 real warnings** (the ~15 obsolete-tag notices remain; those are step 26).

### Step 26 · finding I — the Doxyfile cannot be run as committed
`Doxyfile:802` (`INPUT`), `:61` (`OUTPUT_DIRECTORY`) · CONFIRMED
*Found 2026-09-04.*

`INPUT` is empty with `RECURSIVE = YES`, so a bare `doxygen Doxyfile` scans the whole tree —
including `test/build/_deps/googletest-src`. `OUTPUT_DIRECTORY` is empty too, so it writes
`html/` and `latex/` into the repo root. 15 tags are obsolete for doxygen 1.18 (`doxygen -u`).

> **APPLIED, uncommitted.** Four settings: `OUTPUT_DIRECTORY = test/build/doxygen` (`:61`),
> `INPUT = actuator.hpp README.md` (`:802`), `RECURSIVE = NO` (`:876`), `GENERATE_HTML = NO`
> (`:1118`). `EXAMPLE_PATH = test` and `GENERATE_LATEX = YES` were already correct.
> **`doc/` holds only `refman.pdf`** — see the memory note.

**Verified by running `doxygen Doxyfile` bare, with nothing piped in:** 0 real warnings, output
lands in `test/build/doxygen/latex` (gitignored), and the repo root stays free of `html/` and
`latex/`. Every doc build in this session until now needed a hand-written override; it no
longer does.

The 15 obsolete-tag notices remain — the config predates doxygen 1.18. `doxygen -u Doxyfile`
would clear them but rewrites the entire file, so it was left as a separate decision.

**Toolchain (installed 2026-09-04):** `doxygen`, `tectonic`, `poppler`. There is no `makeindex`,
so `refman.ind` must be generated from `refman.idx` by script before the final compile, or the
PDF silently loses its alphabetical index.
