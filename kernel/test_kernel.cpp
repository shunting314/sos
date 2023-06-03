/*
 * Some code that tests kernel logic. It's called directly after all the initialization
 * is done in the kernel while before falling into kernel shell.
 *
 * The testing code is guarded by preprocessing flags. It should be NOP by default.
 */

#include <assert.h>
#include <vector.h>
#include <algorithm.h>

#define TEST_VECTOR 0
#if TEST_VECTOR
void test_vector() {
  vector<int> vec;
  vec.push_back(2);
  vec.push_back(3);
  assert(vec.size() == 2);
  assert(vec[0] == 2);
  assert(vec[1] == 3);
  vec.destruct();
}
#else
void test_vector() { }
#endif

#define TEST_MALLOC 1
#if TEST_MALLOC
void test_malloc() {
  int* ar1 = (int*) malloc(16);
  ar1[0] = 53;
  int* ar2 = (int*) realloc(ar1, 32);
  assert(ar2 != ar1);
  assert(ar2[0] == 53);
  free(ar2);

  // exaust the available heap memory
  void* huge = malloc((1 << 20) - 24);
  free(huge);
}
#else
void test_malloc() { }
#endif

#define TEST_SORT 1
#if TEST_SORT
void test_sort() {
  vector<int> ar;
  ar.push_back(0);
  for (int i = 1; i <= 5; ++i) {
    ar.push_back(i);
    ar.push_back(-i);
  }
  sort(ar.begin(), ar.end());
  assert(ar.size() == 11);
  for (int i = 0; i < ar.size(); ++i) {
    assert(ar[i] == i - 5);
  }
}
#else
void test_sort() { }
#endif

void test_kernel() {
  test_vector();
  test_malloc();
  test_sort();
}
