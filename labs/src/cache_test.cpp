//
// Created by chiro on 22-12-15.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#define ARRAY_SIZE (1 << 30)                                    // test array size is 2^28

typedef unsigned char BYTE;                    // define BYTE as one-byte type

BYTE array[ARRAY_SIZE];                      // test array
const int L2_cache_size = 1 << 18;

double get_usec(const struct timeval tp0, const struct timeval tp1) {
  return 1000000 * (tp1.tv_sec - tp0.tv_sec) + tp1.tv_usec - tp0.tv_usec;
}

// have an access to arrays with L2 Data Cache'size to clear the L1 cache
void Clear_L1_Cache() {
  memset(array, 0, L2_cache_size);
}

// have an access to arrays with ARRAY_SIZE to clear the L2 cache
void Clear_L2_Cache() {
  memset(array, 0, ARRAY_SIZE);
}

void Test_Cache_Size() {
  printf("**************************************************************\n");
  printf("Cache Size Test\n");

  // TODO

  /**
   * struct timeval tp[2];
   * gettimeofday(&tp[0], NULL);
   * func();
   * gettimeofday(&tp[1], NULL);
   * time_used = getusec(tp[0], tp[1]);
   */
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
