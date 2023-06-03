#pragma once

template <typename S, typename T>
class pair {
 public:
  explicit pair(const S& s, const T& t) : first(s), second(t) { }
 public:
  S first;
  T second;
};

template <typename S, typename T>
pair<S, T> make_pair(const S& s, const T& t) {
  return pair(s, t);
}
