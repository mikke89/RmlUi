#pragma once

#include "Types.h"

namespace Rml {

namespace Utilities {

	template <class T>
	inline void HashCombine(size_t& seed, const T& v)
	{
		Hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

} // namespace Utilities
} // namespace Rml
