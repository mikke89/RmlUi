/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2025 The RmlUi Team, and contributors
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

#ifndef RMLUI_CORE_LAYOUT_FORMATTINGCONTEXTDEBUG_H
#define RMLUI_CORE_LAYOUT_FORMATTINGCONTEXTDEBUG_H

#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/Types.h"
#include "FormattingContext.h"

namespace Rml {
#ifdef RMLUI_DEBUG

class FormatIndependentDebugTracker {
public:
	FormatIndependentDebugTracker();
	~FormatIndependentDebugTracker();
	static FormatIndependentDebugTracker* GetIf();

	struct Entry {
		int level = 0;
		String parent_container_element;
		String absolute_positioning_containing_block_element;
		Vector2f containing_block;
		Vector2f absolute_positioning_containing_block;
		String element;
		Optional<Box> override_box;
		FormattingContextType type;
		struct LayoutResults {
			bool from_cache;
			Vector2f visible_overflow_size;
			Optional<Box> box;
		};
		Optional<LayoutResults> layout;
	};

	List<Entry> entries;
	int current_stack_level = 0;

	void Reset();

	String ToString() const;
	void LogMessage() const;
	int CountEntries() const;
	int CountCachedEntries() const;
	int CountFormattedEntries() const;
};

#endif // RMLUI_DEBUG
} // namespace Rml
#endif
