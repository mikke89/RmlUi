/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

#ifndef RMLUI_CORE_CONTROLLEDLIFETIMERESOURCE_H
#define RMLUI_CORE_CONTROLLEDLIFETIMERESOURCE_H

#include "../../Include/RmlUi/Core/Debug.h"
#include "../../Include/RmlUi/Core/Traits.h"

namespace Rml {

template <typename T>
class ControlledLifetimeResource : NonCopyMoveable {
public:
	ControlledLifetimeResource() = default;
	~ControlledLifetimeResource() noexcept { RMLUI_ASSERTMSG(!pointer || intentionally_leaked, "Resource was not properly shut down."); }

	explicit operator bool() const noexcept { return pointer != nullptr; }

	void Initialize()
	{
		RMLUI_ASSERTMSG(!pointer, "Resource already initialized.");
		pointer = new T();
	}

	void InitializeIfEmpty()
	{
		if (!pointer)
			Initialize();
		else
			SetIntentionallyLeaked(false);
	}

	void Leak() { SetIntentionallyLeaked(true); }

	void Shutdown()
	{
		RMLUI_ASSERTMSG(pointer, "Shutting down resource that was not initialized, or has been shut down already.");
		RMLUI_ASSERTMSG(!intentionally_leaked, "Shutting down resource that was marked as leaked.");
		delete pointer;
		pointer = nullptr;
	}

	T* operator->()
	{
		RMLUI_ASSERTMSG(pointer, "Resource used before it was initialized, or after it was shut down.");
		return pointer;
	}

private:
#ifdef RMLUI_DEBUG
	void SetIntentionallyLeaked(bool leaked) { intentionally_leaked = leaked; }
	bool intentionally_leaked = false;
#else
	void SetIntentionallyLeaked(bool /*leaked*/) {}
#endif

	T* pointer = nullptr;
};

} // namespace Rml
#endif
