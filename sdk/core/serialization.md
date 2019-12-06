# Serialization/Deserialization

There are two main options to make request/response strongly typed:

1. Strongly typed iterators.
2. Serialize/Deserialize into structures.

Implementation may use hybrid solutions internally.

## 1. Strongly Typed Iterators

There are two types of iterators:

1. A pull iterator (a reader).
2. A push iterator (a writer).

It's hard to implement a complex pull iterator in C without allocating.
A push iterator is using stack and usually doesn't require allocations.
Usually, the stack usage is `O(1)` or `O(log(n))`, where `n` is a size of input/output data.
Also, a push iterator is much safer for C. It has very clear life-time of objects and can
use RAII. Also it provides safe typing for such things as
[tagged unions](https://en.wikipedia.org/wiki/Tagged_union),
in particular [option type](https://en.wikipedia.org/wiki/Option_type).

## 1.1. Pull Iterators

**Advantages:**

- Easier to use than a push iterator. At least, it looks like easy.

**Disadvantages:**

- Harder to implement compare to a push iterator.
- Unclear object life-time without dynamic memory management environment (GC, smart).

## 1.2. Push Iterators

**Advantages:**

- Different serializer/deserializer can be built on top of a push iterator. A push iterator implementation can be shared between different serializers. For example, we can have C++ data structure that serialized/desrialized by using a C push iterator.
- It doesn't require a memory allocator.
- It doesn't require a buffer for reading/writing complete JSON.
- It requires only limited stack memory, `O(1)` or `O(log(n))`.

**Disadvantages:**

- Users of Azure SDK for C may have a difficult time to understand the push iterator programming model.

## 2. Serialize/Deserialize Into Structures

**Advantages:**

- Deserialized structures are easy to read. Note: it's true for languages like C++ (with generics) but it's not so good for C.

**Disadvantages:**

- Most likely, C structures will not be reused in C++.
- Requires a memory allocator.
- Occupies `O(n)` memory.

## 3. Proposal

To use advantages from both options. We can implement a push iterator on C as a core functionality and it can be used by advanced programmers for memory/time restricted environments. In the same time, we can provide another serialization/deserialization layer which will use dynamic memory to store data. We may implement this layer for C++ only.

## 4. Example

### 4.1. A Push Iterator

[az_keyvault_key_bundle_builder.h](../keyvault/keyvault/inc/az_keyvault_key_bundle_builder.h)

### 4.2. A Pull Iterator

### 4.3. Lazy Getters

### 4.3. Deserializing Using Lists

### 4.4. Multipass Deserialization