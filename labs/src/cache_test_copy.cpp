#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>

// L1D_CACHE_SIZE is 32 KiB
#define L1D_CACHE_SIZE (1 << 15)
// #define L1D_CACHE_SIZE (1 << 2)
// L2_CACHE_SIZE is 512 KiB
#define L2_CACHE_SIZE (1 << 19)
#define TEST_SIZE (1 << 30)
uint8_t cache[TEST_SIZE];
uint8_t test_array[TEST_SIZE];

double get_usec(const struct timeval tp0, const struct timeval tp1) {
  return 1000000 * (tp1.tv_sec - tp0.tv_sec) + tp1.tv_usec - tp0.tv_usec;
}

void clear_cache() {
  for (uint32_t i = 0; i < TEST_SIZE; i++) {
    cache[i]++;
  }
}

void Test_Cache_Size() {
  printf("**************************************************************\n");
  printf("Cache Size Test\n");

  for (int i = 2; i < 12; i++) {
    // test memory size
    uint32_t test_size = (1 << (10 + i));
    struct timeval tp[2];
    printf("%6d KiB\t", 1 << i);

    clear_cache();

    register uint32_t access = 0;
    // then, repeat reading for 1000 times
    gettimeofday(&tp[0], NULL);
    for (register int k = 0; k < 200000; k++) {
      for (register uint32_t index = 1; index < test_size; index += 2048) {
        test_array[index] = index + 1;
        access++;
      }
    }
    gettimeofday(&tp[1], NULL);
    time_t time_used = get_usec(tp[0], tp[1]);
    printf("%.5f us\n", time_used / (float) access);
  }
}

void Test_L1C_Block_Size() {
  printf("**************************************************************\n");
  printf("L1 DCache Block Size Test\n");

  for (int i = 1; i < 9; i++) {
    printf("%6d Byte\t", 1 << i);
    struct timeval tp[2];

    register uint32_t access = 0;

    clear_cache();
    gettimeofday(&tp[0], NULL);
    for (register uint32_t index = 0; index < TEST_SIZE; index += 1 << i) {
      test_array[index] = index + 1;
      access++;
    }
    gettimeofday(&tp[1], NULL);
    time_t time_used = get_usec(tp[0], tp[1]);
    printf("%.5f us\n", time_used / (float) access);
  }
}

void Test_L1C_Way_Count() {
  printf("**************************************************************\n");
  printf("L1 DCache Way Count Test\n");

  for (int i = 1; i < 8; i++) {
    printf("%4d-way\t", 1 << i);
    struct timeval tp[2];

    clear_cache();

    gettimeofday(&tp[0], NULL);
    register uint32_t access = 0;

    for (register uint32_t cnt = 0; cnt < 100000; cnt++) {
      for (register uint32_t j = 0; j < (1 << i); j++) {
        test_array[j << 15] = j;
        access++;
      }
    }

    gettimeofday(&tp[1], NULL);
    time_t time_used = get_usec(tp[0], tp[1]);
    printf("%.5f us\n", time_used / ((float) access));

  }
}

void Test_TLB_Size() {
  printf("**************************************************************\n");
  printf("TLB Size Test\n");

  for (int i = 2; i < 9; i++) {
    printf("%4d entries\t", 1 << i);
    struct timeval tp[2];

    clear_cache();

    register uint32_t access = 0;
    gettimeofday(&tp[0], NULL);

    for (register uint32_t cnt = 0; cnt < 100000; cnt++) {
      for (register uint32_t j = 0; j < (1 << i); j++) {
        test_array[j << 16 | ((cnt ^ j) & ((1 << 12) - 1))] = j;
        access++;
      }
    }

    gettimeofday(&tp[1], NULL);
    time_t time_used = get_usec(tp[0], tp[1]);
    printf("%.5f us\n", time_used / ((float) access));
  }
}

int main() {
  // Clear cache
  for (uint32_t i = 0; i < L2_CACHE_SIZE; i++) {
    cache[i] = i;
  }

  // Fulfill test array
  memset(test_array, 5, sizeof(test_array));
  Test_Cache_Size();
  Test_L1C_Block_Size();
  Test_L1C_Way_Count();
  Test_TLB_Size();

  return 0;
}
