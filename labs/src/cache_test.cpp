//
// Created by chiro on 22-12-15.
//
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ctime>
#include <cstdint>
#include "debug_macros.h"
#include <sys/time.h>
#include <sys/types.h>

using namespace std;

#define ARRAY_SIZE (1 << 30)                                    // test array size is 2^28
#define KiB(x) ((x) * (1 << 10))
#define MiB(x) ((x) * (1 << 20))

using BYTE = uint8_t;                    // define BYTE as one-byte type
using WORD = uint32_t;                   // define WORD as quad-byte type

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
  const size_t loop2_const = 0x100;
  const size_t loop_const = 0x200;
  size_t loop2 = loop2_const;
  struct timeval start{}, stop{};
  // init data
  // Clear_L1_Cache();
  Clear_L2_Cache();
  srand(time(nullptr));
  // for (auto i = 0; i < L2_cache_size; i++) array[i] = rand() & 0xFF;
  // loop read
  using UNIT = WORD;
  double time_used_min = 0xffffff;
  double time_other_min = 0xffffff;
  auto sz_one = size / (sizeof(UNIT) / sizeof(BYTE));
  while (loop2--) {
    gettimeofday(&start, nullptr);
    size_t loop = loop_const;
    while (loop--) {
      auto sz = sz_one;
      volatile UNIT *p = (UNIT *) array;
      while (sz--) {
        // auto p2 = *(p++);
        // r = (r ^ p2) + p2;
        *(p++) = rand();
      }
    }
    gettimeofday(&stop, nullptr);
    // printf("r = %x\n", r);
    auto time_used = get_usec(start, stop);
    if (time_used_min > time_used) time_used_min = time_used;
  }
  loop2 = loop2_const;
  while (loop2--) {
    gettimeofday(&start, nullptr);
    size_t loop = loop_const;
    while (loop--) {
      auto sz = sz_one;
      volatile UNIT *p = (UNIT *) array;
      while (sz--) {
        volatile auto r = rand();
      }
    }
    gettimeofday(&stop, nullptr);
    // printf("r = %x\n", r);
    auto time_used = get_usec(start, stop);
    if (time_other_min > time_used) time_other_min = time_used;
  }
  if (time_other_min > time_used_min) {
    Err("no data longer...");
  }
  return (abs(time_used_min - time_other_min)) / (double) sz_one;
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
    printf("%4lu KiB:\t%lf\n", t.first >> 10, t.second);
  else
    printf("%4.1lf MiB:\t%lf\n", (double) (t.first) / (double) MiB(1), t.second);
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
  // for (auto i = 5; i <= 11; i++) {
  for (auto i = 5; i <= 7; i++) {
    size_t sz = KiB(1 << i);
    time.emplace_back(sz, visit_array_in_size(sz));
    display_pair_result(time.back());
    sz = (size_t) ((double) (sz) * 1.5);
    time.emplace_back(sz, visit_array_in_size(sz));
    display_pair_result(time.back());
  }
  display_result_graph(time);
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
