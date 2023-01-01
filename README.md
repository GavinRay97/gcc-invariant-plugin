# GCC Design-by-Contract [[invariant]] plugin

A work-in-progress GCC plugin to support Design-by-Contract (DbC) in C++

Inspired wholly by the D Programming Language's [invariant](https://tour.dlang.org/tour/en/gems/contract-programming) feature.

## Blogpost

See the blogpost about this project:

-   https://gavinray97.github.io/blog/adding-invariant-to-cpp-design-by-contract

## Example

Meant to be used in tandem with the new Contracts feature in C++20, recently upstreamed to GCC:

```cpp
class Stack
{
private:
  static constexpr int MAX_SIZE = 100;

  int top     = 0;
  int old_top = 0;
  int data[MAX_SIZE];

  [[demo::invariant]] [[gnu::used]]
  void check_invariants()
  {
	assert(top >= 0 && top <= 50);
  }

public:
  bool empty() const { return top == 0; }
  bool full() const { return top == MAX_SIZE; }

  void push(int value)
  [[pre: !full()]]
  [[post: top == old_top + 1]]
  {
    data[top++] = value;
    old_top = top;
  }


  int pop()
  [[pre: !empty()]]
  [[post: top == old_top - 1]]
  {
    old_top = top;
    return data[--top];
  }
};
```

## Building and Testing the Plugin

```sh-session
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build --target run-gcc
```

This will build the plugin located in `src/plugin.cpp` and test it against the contents of `test/test.cpp`.

It does this by running the following command:

```cmake
 ${GCC_COMPILER} -fplugin=./libgcc-invariant-plugin.so -c \
    -o ${CMAKE_CURRENT_SOURCE_DIR}/test-binary \
       ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp
```
