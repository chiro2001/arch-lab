//
// Created by chiro on 22-12-15.
//
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ctime>
#include <cstdint>
#include <queue>
#include <algorithm>
#include "debug_macros.h"
#include <sys/time.h>
#include <sys/types.h>

using namespace std;

#define ARRAY_SIZE (1 << 30)                                    // test array size is 2^28
#define KiB(x) ((x) * (1 << 10))
#define MiB(x) ((x) * (1 << 20))

using BYTE = uint8_t;                    // define BYTE as one-byte type
using WORD = uint32_t;                   // define WORD as quad-byte type

// const size_t loop_const = 0x40;
const size_t loop_const = 200000;
BYTE array[ARRAY_SIZE];                      // test array
const int L2_cache_size = 1 << 22;

double get_usec(const struct timeval tp0, const struct timeval tp1) {
  return (double) (1000000 * (tp1.tv_sec - tp0.tv_sec) + tp1.tv_usec - tp0.tv_usec);
}

// have access to arrays with L2 Data Cache's size to clear the L1 cache
void Clear_L1_Cache() {
  memset(array, 0, L2_cache_size);
}

// have access to arrays with ARRAY_SIZE to clear the L2 cache
void Clear_L2_Cache() {
  memset(array, 0, ARRAY_SIZE);
}

double visit_array_in_size(size_t size) {
  struct timeval tp[2];
  Clear_L2_Cache();
  register uint32_t access = 0;
  gettimeofday(&tp[0], nullptr);
  for (register int k = 0; k < 200000; k++) {
    for (register uint32_t index = 1; index < size; index += 2048) {
      array[index] = index + 1;
      access++;
    }
  }
  gettimeofday(&tp[1], nullptr);
  auto time_used = get_usec(tp[0], tp[1]);
  return time_used / (double) access;
}

template<typename F>
double max_min_time_in_vector(vector<pair<size_t, double>> &vec, F f) {
  double m = vec.front().second;
  for (auto &t: vec) {
    if (f(m, t.second)) {
      m = t.second;
    }
  }
  return m;
}

void display_pair_result(pair<size_t, double> &t) {
  if (t.first < MiB(1))
    printf("%4lu KiB:\t%.8lf us\n", t.first >> 10, t.second * 1000000 / loop_const);
  else
    printf("%4.1lf MiB:\t%.8lf us\n", (double) (t.first) / (double) MiB(1), t.second * 1000000 / loop_const);
}

void display_result_graph(vector<pair<size_t, double>> &vec) {
  auto max_time = max_min_time_in_vector(vec, [](double a, double b) { return a < b; });
  auto min_time = max_min_time_in_vector(vec, [](double a, double b) { return a > b; });
  for (auto &t: vec) {
    auto max_len = 40;
    auto pos = (t.second - min_time) * max_len / (max_time - min_time);
    printf("[");
    for (int i = 0; i < pos; i++) printf(" ");
    printf("#");
    for (int i = 0; i <= (max_len - pos); i++) printf(" ");
    printf("] ");
    display_pair_result(t);
  }
}

void Test_Cache_Size() {
  printf("**************************************************************\n");
  printf("Cache Size Test\n");

  vector<pair<size_t, double>> time;
  size_t level0 = 1;
  size_t level1 = 13;
  // warm up
  // Log("warming up");
  for (auto i = level0; i < level1; i++) {
    size_t sz = KiB(1 << i);
    visit_array_in_size(sz);
  }
  // Log("warm up done");
  for (auto i = level0; i < level1; i++) {
    size_t sz = KiB(1 << i);
    time.emplace_back(sz, visit_array_in_size(sz));
    // display_pair_result(time.back());
    sz = (size_t) ((double) (sz) * 1.5);
    time.emplace_back(sz, visit_array_in_size(sz));
    // display_pair_result(time.back());
  }
  display_result_graph(time);
}

const size_t cache_l1_size = KiB(48);

double visit_array_in_range(size_t m, size_t size) {
  Clear_L1_Cache();
  return 0;
}

double visit_array_in_range(size_t m) {
  return visit_array_in_range(m, 0x1000);
}

void Test_L1C_Block_Size() {
  printf("**************************************************************\n");
  printf("L1 DCache Block Size Test\n");

  Clear_L1_Cache();                      // Clear L1 Cache

  // TODO
}

void Test_L2C_Block_Size() {
  printf("**************************************************************\n");
  printf("L2 Cache Block Size Test\n");

  Clear_L2_Cache();                      // Clear L2 Cache

  // TODO
}

void Test_L1C_Way_Count() {
  printf("**************************************************************\n");
  printf("L1 DCache Way Count Test\n");

  // TODO
}

void Test_L2C_Way_Count() {
  printf("**************************************************************\n");
  printf("L2 Cache Way Count Test\n");

  // TODO
}

void Test_Cache_Write_Policy() {
  printf("**************************************************************\n");
  printf("Cache Write Policy Test\n");

  // TODO
}

void Test_Cache_Swap_Method() {
  printf("**************************************************************\n");
  printf("Cache Replace Method Test\n");

  // TODO
}

void Test_TLB_Size() {
  printf("**************************************************************\n");
  printf("TLB Size Test\n");

  const int page_size = 1 << 12;                // Execute "getconf PAGE_SIZE" under linux terminal

  // TODO
}

FILE *log_fp = nullptr;

int main() {
  Test_Cache_Size();
  Test_L1C_Block_Size();
  // Test_L2C_Block_Size();
  Test_L1C_Way_Count();
  // Test_L2C_Way_Count();
  // Test_Cache_Write_Policy();
  // Test_Cache_Swap_Method();
  Test_TLB_Size();

  return 0;
}
