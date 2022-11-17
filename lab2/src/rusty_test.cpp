//
// Created by chiro on 22-11-17.
//
#include "rusty_bridge/lib.h"
#include <iostream>

using namespace std;

extern "C" {
int32_t rusty_extern_c_integer();
}

int main() {
  std::cout << "A value given via generated cxxbridge "
            << rusty_cxxbridge_integer() << "\n";
  std::cout << "A value given directly by extern c function "
            << rusty_extern_c_integer() << "\n";
  return 0;
}