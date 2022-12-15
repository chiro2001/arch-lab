//
// Created by chiro on 22-12-15.
//
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ctime>
#include <cstdint>
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
  size_t loop = 0x2000;
  struct timeval start{}, stop{};
  // init data
  Clear_L1_Cache();
  // for (auto i = 0; i < L2_cache_size; i++) array[i] = rand() & 0xFF;
  gettimeofday(&start, nullptr);
  // loop read
  using UNIT = WORD;
  volatile UNIT r = 0;
  auto sz_one = size / (sizeof(UNIT) / sizeof(BYTE));
  while (loop--) {
    auto sz = sz_one;
    UNIT *p = (UNIT *) array;
    while (sz--) {
      r += *(p++);
    }
  }
  // printf("r = %x\n", r);
  gettimeofday(&stop, nullptr);
  auto time_used = get_usec(start, stop);
  return time_used / (double) sz_one;
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
    printf("%12lu KiB:\t%lf\n", t.first >> 10, t.second);
  else
    printf("%12.1lf MiB:\t%lf\n", (double) (t.first) / (double) MiB(1), t.second);
}

void display_result_graph(vector<pair<size_t, double>> &vec) {
  auto max_time = max_min_time_in_vector(vec, [](double a, double b) { return a < b; });
  auto min_time = max_min_time_in_vector(vec, [](double a, double b) { return a > b; });
  for (auto &t: vec) {
    auto max_len = 20;
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
  for (auto i = 1; i <= 11; i++) {
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

int main() {
  srand(time(nullptr));
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
