// PR c++/35316
// { dg-options "-std=c++0x" }

template<int> struct A
{
  int i : 2;

  void foo()
  {
    decltype(i) j;
  }
};
