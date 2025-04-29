# Data Binding

Data binding in Brisk allows seamless data flow between objects, automatically synchronizing the application’s state with minimal configuration. This approach, especially useful in GUI contexts but applicable universally, aligns with the principles of reactive programming, where state changes propagate through the system, allowing components to respond immediately and maintain data synchronization.

Brisk's data binding operates using memory address ranges as binding keys. 

To inform the binding system of a value change, the program should call a designated function and pass a memory range that specifies the changed data. This approach leverages memory addresses as keys, allowing the use of field or variable addresses without requiring registration of each variable.

The `Property<>` class simplifies notifications by automating them. See [Property](#property) for more details.

> [!note]
> Memory ranges used as keys must be registered in the binding system using one of the available methods. See [Memory Ranges](#memory-ranges) for more information.

The core abstraction in the binding system is the `Value<T>` type, which combines data getter and setter functionality with associated memory ranges. To create a `Value` from a variable, use the syntax `Value{ &variable }`. The variable's address is used as the single argument in the `Value` constructor.

You can subscribe to value changes using a custom lambda callback. The callback is triggered whenever the specified value changes, provided the subscribed and changed memory ranges intersect. See the example below.

## Binding Example

Consider a struct `S` containing two fields, `a` and `b`:

```c++
#include <brisk/core/Binding.hpp>
...
S s{};
// Range registration not shown here; see below.
// Subscribe to changes to individual fields `a`, `b`, and `S` as a whole.
bindings->listen(Value{&s.a}, [](){ fmt::println("a triggered"); });
bindings->listen(Value{&s.b}, [](){ fmt::println("b triggered"); });
bindings->listen(Value{&s}, [](){ fmt::println("S triggered"); });

bindings->notify(&s.a); // Output: "a triggered", "S triggered"
bindings->notify(&s.b); // Output: "b triggered", "S triggered"
bindings->notify(&s);   // Output: "a triggered", "b triggered", "S triggered"
```

This mechanism allows batch notifications about changes to an entire object, provided its data is laid out linearly in memory. Non-pointer fields in `struct`s and `class`es are contiguous in memory, making them suitable for batch notifications. Field order, memory layout, and padding are irrelevant to the binding algorithm.

## Connecting Values

`bindings->connect` links two values so that changes in the source value propagate to the target value:

```c++
int src = 123;
float tgt = 0.f;
bindings->connect(Value{ &tgt }, Value{ &src });
// `connect` also copies the current source value to the target
// unless `updateNow` is set to `false`.
BRISK_ASSERT(tgt == 123);
```

Type compatibility is flexible as long as a conversion exists.

`connectBidir` establishes a bidirectional connection:

```c++
int one = 111;
uint64_t two = 222;
bindings->connectBidir(Value{ &one }, Value{ &two });
bindings->assign(two) = 3; // Updates `two` to 3 and notifies of the change
// `one` is also updated:
BRISK_ASSERT(one == 3);

// In the opposite direction:
bindings->assign(one) = 7; // Updates `one` to 7 and notifies of the change
// `two` is also updated:
BRISK_ASSERT(two == 7);
```

Connections persist as long as both values are in scope. If either value goes out of scope or is destroyed on the heap (causing its memory range to be unregistered), the connection is automatically removed.

## Memory Ranges

Only registered memory ranges may be used in `notify`, `listen` and `connect` calls.

### Manual Registration

To register a memory range, use the following function:
```c++
void Bindings::registerRegion(BindingAddress region, Rc<Scheduler> scheduler);
```

To create a `BindingAddress` from a variable, use `toBindingAddress`, passing the variable’s address, as shown below:

```c++
bindings->registerRegion(toBindingAddress(&src), nullptr);
```

The second argument is the scheduler. See [Scheduler](#scheduler) for more details.

### `BindingRegistration`

Memory ranges can also be registered using the RAII class `BindingRegistration`. Ensure that the `BindingRegistration` instance has the same lifetime as the registered object, as shown in this example:

```c++
SomeClass instance;
BindingRegistration instance_reg{ &instance, nullptr };
```

Alternatively, embed the `BindingRegistration` object directly within a class to match the object's lifetime:

```c++
class SomeClass {
public:
    BindingRegistration m_lt{ this, nullptr /* Scheduler */ };
};
```

### Deriving from `BindableObject<Derived>`

Another way to ensure correct lifetime management is by deriving from `BindableObject`, passing the class itself as the first template argument and an address to a `Rc<Scheduler>`-convertible variable or `nullptr`:

Example:
```c++
class Component : public BindableObject<Component, &uiScheduler> {
public:
    virtual ~Component();
};
```

The `uiScheduler` variable is defined in `window/WindowApplication.hpp`:

```c++
extern Rc<TaskQueue> uiScheduler;
```

The `BindableObject` class also inherits from `std::enable_shared_from_this`, allowing `shared_from_this` to be used.

## Value Expressions

*Section under construction.*

## Edge Cases

### Notifying About Non-Memory Changes

The Brisk binding system requires a unique address for binding to function. Use a static variable (e.g., a `uint8_t`) if the value’s lifetime is static, or create a `uint8_t` on the heap and register it as described above. Then, use this address as the key.

## Lifetime

*Section under construction.*

## `Scheduler`

By default, all callbacks are invoked by the binding system within the `notify` call, on the same thread that triggers the variable change.

This behavior can be modified by associating a `Scheduler` with a memory range. A `Scheduler` is an interface used to enqueue a lambda, potentially executing it on another thread. `TaskQueue` is the main implementation of the `Scheduler` interface and can queue tasks for later execution on a target thread.

> [!warning]
> Ensure that `notify` is called (or a `Property` is changed) only on the associated task queue’s thread.

## `Property`

*Section under construction.*
