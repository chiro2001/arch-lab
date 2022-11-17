//
// Created by chiro on 22-11-17.
//

#ifndef BRANCH_PREDICATION_BRIDGE_H

#include <cstdint>

static int32_t (*rusty_cxxbridge_integer)() noexcept = nullptr;
static int32_t (*rusty_cbridge_integer)() = nullptr;

#define BRANCH_PREDICATION_BRIDGE_H

#endif //BRANCH_PREDICATION_BRIDGE_H
