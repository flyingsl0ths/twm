#pragma once

#include <cstddef>
#include <string_view>

using u8 [[maybe_unused]]    = char8_t;
using u32 [[maybe_unused]]   = unsigned int;
using s32 [[maybe_unused]]   = int;
using f32 [[maybe_unused]]   = float;
using f64 [[maybe_unused]]   = double;
using usize [[maybe_unused]] = size_t;
using str [[maybe_unused]]   = std::string_view;
