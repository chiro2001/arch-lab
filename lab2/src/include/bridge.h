//
// Created by chiro on 22-11-17.
//

#ifndef BRANCH_PREDICATION_BRIDGE_H

#include <cstdint>

// static int32_t (*rusty_cxxbridge_integer)() noexcept = nullptr;
// static int32_t (*rusty_extern_c_integer)() = nullptr;
extern "C" {
int32_t rusty_extern_c_integer();
bool rust_start();
bool rust_finish(uint64_t, uint64_t, uint64_t, uint64_t);
}

#define BRANCH_PREDICATION_BRIDGE_H

#endif //BRANCH_PREDICATION_BRIDGE_H
