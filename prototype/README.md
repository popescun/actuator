# Struct connections in C++ - prototypes

Prototypes of the ulang connection mechanism - *intrinsic interfaces*, *actuators*, *funcrefs* and
the `<intf>` operator - expressed in C++ on top of `untangle::actuator`.

Everything here is buildable:

```sh
cmake -S . -B build && cmake --build build
./build/bin/intrinsic_interface_using_macro
./build/bin/intrinsic_interface_using_slots
./build/bin/intrinsic_interface_using_nttp
./build/bin/intrinsic_interface_using_named
```

## What turned out to be possible

| ulang                                | C++ prototype                              | notes |
|--------------------------------------|--------------------------------------------|-------|
| `pos_intf =interface { set; get_x; }` | `INTRINSIC_INTERFACE(pos_intf, set, get_x)` | signatures deduced, never spelled out |
| `a <pos_intf> b;`                     | `a <pos_intf> b;`                          | identical spelling |
| `a <pos_intf> {b, c};`                | `a <pos_intf> untangle::endpoints(b, c);`  | `{b, c}` can not be deduced by an operator |
| `a <p1> b <p2> c;`                    | `a <p1> b <p2> c;`                         | identical spelling |
| `a >pos_intf< b;`                     | `a >pos_intf< b;`                          | identical spelling |
| `a.pos_intf.set_act(3, 4);`           | `a.pos_intf.set_act(3, 4);`                | identical spelling |
| `pos_intf.set_act(3, 4);` in a method | `pos_intf.set_act(3, 4);`                  | identical spelling |
| funcref field as interface member     | `std::function<...>` data member           | interchangeable with a method, as in ulang |
| `<act>.actions[i].parent`             | `<intf>.parent_of<T>(action)`              | type checked, null when not connected |
| `<intf>.is_actuator` / `.is_action`   | `<intf>.is_actuator()` / `.is_action()`    |  |

The infix operator is the part that looks impossible and is not. How it works is the subject of the
next section.

## The interface reference

One declaration is shared by every approach:

```cpp
inline constexpr auto pos_intf =
    untangle::intf([](auto& end_point) -> auto& { return end_point.pos_intf; });
```

`ENABLE_CONNECT_OPERATOR(pos_intf);` is a shorthand that expands to exactly that line - approaches
A, B and D use it, while C writes the line out so that it needs no preprocessor at all.

It exists to answer a single question: when you write `a <pos_intf> b`, how does an operator learn
*which* interface to wire? An operator only receives its operands, and `pos_intf` is a member living
inside both objects.

A member pointer can not carry that meaning. `&point::pos_intf` is bound to `point`, while the two
endpoints may be of different types - a `point` connected to a `display`. What is needed is the
interface *name*, in a form that can be passed through an operator and applied to either side
independently. The lambda is that form, and `untangle::intf()` reifies it as a type:

```cpp
template <typename accessor_t>
struct interface_ref {
  template <typename endpoint_t>
  static auto& of(endpoint_t& end_point) { return accessor_t{}(end_point); }
};
```

The projection travels in the *type*, never as data: a captureless lambda's closure type is default
constructible in C++20, so `accessor_t{}` rebuilds the accessor from nothing and the reference
stays empty. `of()` is a template, so it is duck typed - it works for any type with a member called
`pos_intf`, which is what lets one declaration serve every class declaring that interface. The
object exists so the name can appear as an operand at all; a type alone can not sit in
`a < ... > b`.

The expression `a <pos_intf> b` parses as `(a < pos_intf) > b`, and the two halves split the work:

```cpp
operator<(a, pos_intf)  ->  connect_expression<point, untangle::interface_ref<lambda>>{&a}
```

The left endpoint is captured as a pointer and the reference rides along **as a template parameter,
not as data** - the object itself is an unnamed `const ref_t&` parameter and is discarded at once.
Only its type survives, as a compile time label. The second half spends it:

```cpp
operator>(connect_expression<point, untangle::interface_ref<lambda>>{&a}, b)
    ->  ref_t::of(a).connect_to(ref_t::of(b))
    ->  a.pos_intf.connect_to(b.pos_intf)
```

So the reference carries the interface identity across the two operator calls, and `of()` converts
that identity back into a member access on each side separately. That separation is exactly why
endpoints of different types work.

Two details keep this from being reckless. Both operators are constrained on `is_interface_ref_v`,
which matches only a specialisation of `untangle::interface_ref` - every unrelated `x < y` in the
program is untouched. And because that type lives in namespace `untangle`, the namespace is
associated with the expression, so ADL finds the operators without a `using`.

Two more make it read like ulang. `operator>` returns `rhs_t&`, so `a <p1> b <p2> c` chains left to
right. And disconnection is the same trick mirrored - `a >pos_intf< b` is `(a > pos_intf) < b` -
with the second half operators constrained on `!is_interface_ref_v<rhs_t>` so the two directions
stay unambiguous.

In one line: **an interface reference turns an interface name into a first class, class agnostic
compile time value, because C++ offers no other way to pass "a member name" through an operator.**

### What the types really are

Nothing above is a metaphor - these are the types the compiler deduces for `point` in approach D:

| expression | real type |
|---|---|
| `accessor_t` | `(lambda at <file>:<line>)` - an unnamed, compiler generated class |
| `accessor_t{}` | a prvalue of that same closure type |
| `end_point` | `point&` |
| `accessor_t{}(end_point)` | `point::pos_intf_t&` |
| the declared object | `untangle::interface_ref<(lambda at ...)>` |
| `point::pos_intf_t`'s base | `untangle::interface<&point::set, &point::get_x>` |

Three things follow from that list.

**The closure type is the identity.** A lambda expression synthesises a unique unnamed class, so
each `ENABLE_CONNECT_OPERATOR` invocation produces a distinct `interface_ref` specialisation.
`pos_intf` and `notify_intf` are unrelated types, which is what lets `a <p1> b <p2> c` wire two
different interfaces in one expression - the operators discriminate on type alone.

**The call is a template instantiation.** `[](auto& end_point)` is a *generic* lambda, so its
`operator()` is a member template and `accessor_t{}(end_point)` instantiates `operator()<point>`.
That is the duck typing: one declaration serves any class with a member of that name. The `auto&`
return deduces to `point::pos_intf_t&` - a real reference, not a copy, which matters because an
interface object holds a pointer to its owner and must never be relocated.

**No lambda object ever exists at run time.** The lambda is a temporary handed to
`untangle::intf()`, which takes it by value into an *unnamed* parameter and returns `{}` - the
argument is never read. Only its type is captured, and `accessor_t{}` later rebuilds a fresh closure from nothing, which is
legal because a captureless closure is default constructible in C++20 and has no state to restore.

So the declared object is **not** the lambda: it is not even invocable. The lambda is the payload,
`interface_ref` is the carrier, and the carrier is what supplies a nominal type for the operator
constraints and for ADL. Which is also the answer to the suspicion that this is just another
artificial tag type: the projection here is a real function - one whose identity happens to live
entirely in the type system, and which therefore costs nothing at run time.

The reference must be declared at namespace scope, once per interface *name* - not per class, since
`of()` is a template. Namespace scope does not mean the global namespace: approach D's demo declares
its types and its interface reference inside an anonymous namespace, and the infix spelling still
works in `main`, because the members of an anonymous namespace are visible in the namespace
enclosing it.

It can not be folded into `INTRINSIC_INTERFACE`, which expands inside a class body: a macro emits
its tokens where it is written, and the only construct that declares a namespace scope entity from
inside a class is `friend`, which covers functions and classes but never variables. The declaration
is also optional - it buys the infix spelling, and `a.pos_intf.connect_to(b.pos_intf)` works
without it.

## The question about the macro

> not sure possible to know the params and return types at pre-processing time

Correct - and it is not needed. The preprocessor only has to emit *text*; the types are recovered by
the compiler proper from `decltype(&self_t::set)`, which is what
`untangle::member_signature<>` unpacks into `std::function<R(P...)>`. So `CONNECTION_INTERFACE(name)`
is enough, and `INTERFACE(name, R, P1, P2)` is only needed where deduction can not work
(approach B keeps it for that reason, and static_asserts the spelled out signature against the real
member, so a wrong `R` is a compile error).

Two constraints came out of the C++ rules and shaped all four approaches:

- A class body is **not** a complete-class context, so `decltype(*this)` is unavailable where the
  interface members are declared - the declaring type must arrive some other way: a CRTP base
  (approach A), a macro argument (approach B), a member pointer (approach C), or either of the first
  two (approach D).
- A nested class's *function bodies* and *default member initializers* **are** complete-class
  contexts of the enclosing class. That is what lets the generated interface object take `this` as
  `intf_name{this}` and bind the owner's methods while the owner is still incomplete.

## The four approaches

| | A - deducing macro | B - self registering slots | C - member pointer template | D - thin macro over C |
|---|---|---|---|---|
| declaration | `INTRINSIC_INTERFACE(pos_intf, set, get_x)` | `INTRINSIC_SLOT(set, void, int, int)` per member | `untangle::interface<&point::set> pos_intf{this};` | `INTRINSIC_INTERFACE(pos_intf, set, get_x)` |
| signatures | deduced | spelled out, checked | deduced | deduced |
| declaring type from | CRTP base | macro argument | member pointer | CRTP base, or a macro argument |
| dispatch | `p.pos_intf.set_act(3, 4)` | `p.pos_intf.set_act(3, 4)` | `p.pos_intf.act<&point::set>()(3, 4)` | `p.pos_intf.set_act(3, 4)` |
| endpoint pairing | by member name, compile time | by member name, run time | positional, compile time | positional, compile time |
| endpoints of different types | yes, duck typed | yes, and the peer type need not be visible | yes | yes |
| member count | 8 (raise by extending the for-each) | unlimited | unlimited | 8 (raise by extending the expansions) |
| overloaded member | needs a uniquely named forwarder | needs a uniquely named forwarder | `static_cast` the member pointer | needs a uniquely named forwarder |
| cost | one macro to read through | a vtable and a name lookup per connection | no preprocessor at all, but no generated names | a short macro, plus one reference per member |

Recommendation: **D** as the default - it is as close to ulang at the use site as A, but its macro
only emits a struct declaration and a list of references, leaving the wiring, the lifetime and the
operators in `untangle::interface<>` where they read as ordinary code. **A** where endpoints must
pair by member name rather than by position. **B** where an interface has to be connected across a
module boundary, or where a member is not usable with `&self_t::member`. **C** where macros are
unwelcome at any price.

## What is not reproduced

- **`.parent` type inference.** ulang infers the parent's type from the wiring; C++ can not, so
  `parent_of<T>()` names the type and checks it at run time with `typeid` - a wrong `T` reads as
  null, never as a reinterpreted pointer. `.field("id")` would need reflection.
- **`{b, c}` literal fan-out.** A braced list can not be deduced by an operator, hence
  `untangle::endpoints(b, c)`.
- **Generated member names without the preprocessor.** `set_act` can not be invented by a template,
  and a string template argument does not help: `"set"` is a value, and turning a value into an
  identifier is exactly what C++20 can not do. Approach D buys the name back with the smallest macro
  that will do it - one token paste per member - but the preprocessor is still what creates the
  token. C++26 static reflection (P2996) is what would close the gap properly: `^^point` to
  enumerate the methods and `define_aggregate` to declare `set_act`, at which point D's macro
  becomes a reflection function and the member limit disappears.

## Lifetime, and one rule to respect

`untangle::actuator` stores `action_t*`, so an action must outlive every actuator it is wired into.
The prototypes make that safe: each connection is a record shared by both endpoints, and whichever
endpoint is destroyed first tears it down - the action side unwires itself from the still living
actuator side, the actuator side only marks the record dead. Both destruction orders are exercised
in the demos, under `-fsanitize=address,undefined`.

Where that teardown runs matters. It must happen in the destructor of the object that *owns* the
action slots, and not in `untangle::interface_base::~interface_base()`, because by the time a base
destructor runs the derived object's slots are already gone and a peer unwiring through them is
undefined behaviour. A, B and C each call `drop_links()` from their own destructor for that reason.
Approach D needs no destructor at all: its slots stay in the `untangle::interface<>` base it derives
from, so that base's destructor already runs while they are alive.

The rule: an interface object holds a pointer to its owner, so **a connected struct must not be
copied, moved or relocated**. `untangle::interface_base` deletes its copy and move operations, which
propagates to the declaring class, so the compiler enforces it. Keep such objects in place, or hold
them through `std::shared_ptr`/`std::unique_ptr`.

## Files

| file | |
|---|---|
| `include/intrinsic_interface_core.hpp` | shared: signature traits, connection records, `.parent`, the `<intf>` operators |
| `include/1_intrinsic_interface_using_macro.hpp` | A - the deducing macro |
| `include/2_intrinsic_interface_using_slots.hpp` | B - self registering slots |
| `include/3_intrinsic_interface_using_nttp.hpp` | C - the member pointer template |
| `include/4_intrinsic_interface_using_named.hpp` | D - the thin macro over C |
| `src/<n>_intrinsic_interface_using_*_example.cpp` | one runnable demo per approach |

The headers are in `include/` and the demos in `src/`; `include/` is on the include path, so a demo
includes its approach by bare file name. The leading digit keeps the four approaches in reading
order, A to D, while the shared core carries no index because it belongs to all of them.

## clang-format

`clang-format` reads `a <pos_intf> b` as a template-id and closes the space to `a<pos_intf> b`.
It compiles either way, but the demos keep the ulang spelling with `// clang-format off` around the
connection statements.
