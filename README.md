# GCC Design-by-Contract [[invariant]] plugin

A work-in-progress GCC plugin to support Design-by-Contract (DbC) in C++

Status: **Functional ✅** (though edge cases may exist)

Inspired wholly by the D Programming Language's [invariant](https://tour.dlang.org/tour/en/gems/contract-programming) feature.

You can find a pre-compiled version of the plugin for Linux x86_64 in the root of this repository (`libgcc-invariant-plugin.so`)

## Blogpost

See the blogpost about this project:

-   https://gavinray97.github.io/blog/adding-invariant-to-cpp-design-by-contract

## Example

Meant to be used in tandem with the new Contracts feature in C++20, recently upstreamed to GCC 13 development branch:

-   https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=ea63396f6b08f88f1cde827e6cab94cd488f7fa7

Compiling the below code with the plugin enabled will result in the following error:

```sh-session
$ g++ -fplugin=./libgcc-invariant-plugin.so -c test.cpp -o test-binary
$ ./test-binary
test.cpp:17: void Stack::check_invariants(): Assertion `top >= 0 && top <= 50' failed.
Aborted
```

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

int
main()
{
  Stack stack;

  for (int i = 0; i < 100; i++)
    stack.push(i);

  for (int i = 0; i < 100; i++)
    stack.pop();

  return 0;
}
```

## Building and Testing the Plugin

This plugin is built using CMake. Usage is straightforward, though it does require GCC 13 development branch to be installed.

If you're on Linux, you can install GCC 13 from Jonathan Wakely's development snapshots probably:

-   https://jwakely.github.io/pkg-gcc-latest/

If you don't want to do that and you want to build it yourself, the following is the magic incantation I use to build GCC on my machine. Adapt it for your package manager and system:

```sh-session
$ sudo dnf install flex
$
$ git clone git://gcc.gnu.org/git/gcc.git gcc-dev
$ cd gcc-dev
$
$ ./contrib/download_prerequisites
$ mkdir build && cd build
$
$ ../configure -v \
	--build=x86_64-linux-gnu \
	--host=x86_64-linux-gnu \
	--target=x86_64-linux-gnu \
	--prefix=/usr/local/gcc-dev \
	--enable-checking=release \
	--enable-languages=c,c++ \
	--disable-multilib \
	--disable-nls \
	--with-system-zlib
$
$ make -j 16
$ sudo make install-strip
```

Now with GCC 13 installed, you can build the plugin using CMake. Go into the CMakelists.txt and edit the definitions of:

```cmake
set(G++_COMPILER /usr/local/gcc-dev/bin/g++)
set(GCC_PLUGIN_DIR /usr/local/gcc-dev/lib/gcc/x86_64-linux-gnu/13.0.0/plugin)
```

Then you can invoke the following commands:

```sh-session
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build --target run-gcc
```

This will build the plugin located in `src/plugin.cpp` and test it against the contents of `test/test.cpp`.
