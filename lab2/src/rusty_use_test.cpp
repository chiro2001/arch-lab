//
// Created by chiro on 22-11-18.
//
#include <dlfcn.h>
#include <iostream>
#include "bridge.h"
using namespace std;
int main(int argc, char *argv[]) {
  void *handler = dlopen("/home/chiro/programs/arch-lab/lab2/build/librusty.so", RTLD_LAZY);
  // void *handler = dlopen("/home/chiro/programs/arch-lab/lab2/build/libbp.so", RTLD_LAZY);
  if (!handler) {
    std::cerr << dlerror() << "\n";
    return 1;
  }
  rusty_cxxbridge_integer = (int (*)() noexcept) dlsym(handler, "rusty_cxxbridge_integer");
  if (rusty_cxxbridge_integer) {
    std::cout << "A value given via generated cxxbridge "
              << rusty_cxxbridge_integer() << "\n";
  } else {
    std::cerr << "Cannot open rusty_cxxbridge_integer!" << "\n";
  }

  rusty_extern_c_integer = (int (*)() noexcept) dlsym(handler, "rusty_extern_c_integer");
  if (rusty_extern_c_integer) {
    std::cout << "A value given via generated cbridge "
              << rusty_extern_c_integer() << "\n";
  } else {
    std::cerr << "Cannot open rusty_extern_c_integer!" << "\n";
  }
}