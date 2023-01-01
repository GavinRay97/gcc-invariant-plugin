#include <cassert>
#include <iostream>

// clang-format off
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

// clang-format on

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