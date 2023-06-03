#pragma once

template <typename T>
void swap(T& a, T& b) {
  T t = a;
  a = b;
  b = t;
}

/*
 * A implementation of quick sort.
 * TODO: randomize the pivot once we support random numbers in the kernel.
 */
template <typename ItrType>
void sort(ItrType begin, ItrType end) {
  if (end - begin <= 1) {
    return;
  }

  ItrType next_le = begin;
  ItrType pivot = end - 1;

  // use a middle element as the pivot
  int n = end - begin;
  swap(*(begin + (n / 2)), *pivot);

  for (ItrType cur = begin; cur < pivot; ++cur) {
    if ((*cur) < (*pivot)) {
      swap(*next_le, *cur);
      ++next_le;
    }
  }
  swap(*next_le, *pivot);
  sort(begin, next_le);
  sort(next_le + 1, end);
}
