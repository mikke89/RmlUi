/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUICORETRAITS_H
#define RMLUICORETRAITS_H

#include "Header.h"
#include <type_traits>

namespace Rml {
namespace Core {


class RMLUICORE_API NonCopyMoveable {
public:
	NonCopyMoveable() {}
	~NonCopyMoveable() {}
	NonCopyMoveable(const NonCopyMoveable&) = delete;
	NonCopyMoveable& operator=(const NonCopyMoveable&) = delete;
	NonCopyMoveable(NonCopyMoveable&&) = delete;
	NonCopyMoveable& operator=(NonCopyMoveable&&) = delete;
};


class ReleaserBase;

class RMLUICORE_API Releasable : public NonCopyMoveable {
protected:
	virtual void Release() = 0;
	friend class ReleaserBase;
};

class RMLUICORE_API ReleaserBase {
protected:
	void Release(Releasable* target) const {
		target->Release();
	}
};

template<typename T>
class RMLUICORE_API Releaser final : public ReleaserBase {
public:
	void operator()(T* target) const {
		static_assert(std::is_base_of<Releasable, T>::value, "Rml::Core::Releaser can only operate with classes derived from Rml::Core::Releasable.");
		Release(static_cast<Releasable*>(target));
	}
};


}
}

#endif
