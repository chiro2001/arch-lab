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
const size_t loop_const = 0x200;
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

double visit_array_in_size(size_t size, size_t loop2_const) {
  size_t loop2 = loop2_const;
  struct timeval start{}, stop{};
  // init data
  // Clear_L1_Cache();
  Clear_L2_Cache();
  srand(time(nullptr));
  // for (auto i = 0; i < L2_cache_size; i++) array[i] = rand() & 0xFF;
  for (auto i = 0; i < size; i++) array[i] = rand() & 0xFF;
  // loop read
  using UNIT = WORD;
  size_t min_max_count = 32;
  if (min_max_count > loop2_const) {
    min_max_count = loop2_const;
  }
  // Assert(min_max_count <= loop2_const, "loop2(0x%zx) must larger than min+max count(0x%zx)", loop2_const,
  //        min_max_count);
  vector<double> time_used_min, time_other_min;
  auto sz_one = size / (sizeof(UNIT) / sizeof(BYTE));
  // generate a random list
  const auto random_list_sz = 19;
  auto random_list = new size_t[random_list_sz];
  for (int i = 0; i < random_list_sz; i++) random_list[i] = rand();
  while (loop2--) {
    gettimeofday(&start, nullptr);
    size_t loop = loop_const;
    while (loop--) {
      size_t sz = sz_one;
      size_t s = 0;
      UNIT *p = (UNIT *) array;
      size_t r = 0x3f2f;
      size_t r1 = 1, r2 = 2;
      while (sz--) {
        auto pp = *p;
        // r1 = random_list[(r2 + r) % random_list_sz];
        // r2 = random_list[(r1 + r) % random_list_sz];
        // *(p++) = r ^ (r1 ^ r2);
        r1 = rand();
        auto t = r ^ r1;
        *(p++) = t;
        s += t;
        r = ((r << 1) | (r >> (sizeof(r) * 8 - 1)));
      }
    }
    gettimeofday(&stop, nullptr);
    auto time_used = get_usec(start, stop);
    time_used_min.emplace_back(time_used);
  }
  loop2 = loop2_const;
  while (loop2--) {
    gettimeofday(&start, nullptr);
    size_t loop = loop_const;
    while (loop--) {
      size_t sz = sz_one;
      UNIT *p = (UNIT *) array;
      size_t r = 0x3f2f;
      size_t s = 0;
      size_t r1 = 1, r2 = 2;
      while (sz--) {
        // auto pp = *p;
        // r1 = random_list[(r2 + r) % random_list_sz];
        // r2 = random_list[(r1 + r) % random_list_sz];
        // *(p++) = r ^ (r1 ^ r2);
        r1 = rand();
        auto t = r ^ r1;
        s += t;
        r = ((r << 1) | (r >> (sizeof(r) * 8 - 1)));
      }
    }
    gettimeofday(&stop, nullptr);
    // printf("r = %x\n", r);
    auto time_used = get_usec(start, stop);
    time_other_min.emplace_back(time_used);
  }
  // calculate average
  auto ave = [&](vector<double> &q) {
    double s = 0;
    for (size_t i = 0; i < min_max_count; i++) {
      // if (q.top() + 1 >= value_init) break;
      s += q[i];
      // Log("use top: %lf", q[i]);
    }
    auto r = s / (double) min_max_count;
    // Log("ave result %lf", r);
    return r;
  };
  sort(time_used_min.begin(), time_used_min.end(), less<>());
  sort(time_other_min.begin(), time_other_min.end(), less<>());
  auto time_used_min_ave = ave(time_used_min);
  auto time_other_min_ave = ave(time_other_min);
  if (time_other_min_ave > time_used_min_ave) {
    Err("no data(size=0x%zx, loop=0x%zx)... time_other_min_ave=%lf, time_used_min_ave=%lf",
        size, loop2_const,
        time_other_min_ave, time_used_min_ave);
    // let's retry
    return visit_array_in_size(size, loop2_const);
  }
  return (abs(time_used_min_ave - time_other_min_ave)) / (double) sz_one;
}

double visit_array_in_size(size_t size) {
  const size_t loop2_const = 0x4;
  return visit_array_in_size(size, loop2_const);
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
    printf("%4lu KiB:\t%.2lf us\n", t.first >> 10, t.second * 1000000 / loop_const);
  else
    printf("%4.1lf MiB:\t%.2lf us\n", (double) (t.first) / (double) MiB(1), t.second * 1000000 / loop_const);
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
  size_t level0 = 4;
  // size_t level0 = 1;
  size_t level1 = 8;
  size_t level2 = 11;
  // warm up
  Log("warming up");
  for (auto i = level0; i < level1; i++) {
    size_t sz = KiB(1 << i);
    visit_array_in_size(sz, 0x2);
  }
  Log("warm up done");
  for (auto i = level0; i < level1; i++) {
    size_t sz = KiB(1 << i);
    time.emplace_back(sz, visit_array_in_size(sz, 0x4 * (level1 - i)));
    display_pair_result(time.back());
    sz = (size_t) ((double) (sz) * 1.5);
    time.emplace_back(sz, visit_array_in_size(sz, 0x4 * (level1 - i)));
    display_pair_result(time.back());
  }
  for (auto i = level1; i < level2; i++) {
    size_t sz = KiB(1 << i);
    time.emplace_back(sz, visit_array_in_size(sz));
    display_pair_result(time.back());
    sz = (size_t) ((double) (sz) * 1.5);
    time.emplace_back(sz, visit_array_in_size(sz));
    display_pair_result(time.back());
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
