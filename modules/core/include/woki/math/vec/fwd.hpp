#pragma once

#include "../detail/arithmetic.hpp"
#include <cstddef>

namespace woki::math {
template<std::size_t N, arithmetic T>
requires (N >= 2)
class vec;

} // namespace woki::math
