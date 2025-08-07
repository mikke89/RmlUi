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

class FormattingContextDebugTracker {
public:
	FormattingContextDebugTracker();
	~FormattingContextDebugTracker();
	static FormattingContextDebugTracker* GetIf();

	enum class FormatType {
		FormatIndependent,
		FormatFitContentWidth,
	};

	struct Entry {
		int level = 0;
		FormatType format_type;
		String parent_container_element;
		String absolute_positioning_containing_block_element;
		std::optional<Vector2f> containing_block;
		std::optional<Vector2f> absolute_positioning_containing_block;
		String element;
		Optional<Box> override_box;
		FormattingContextType context_type;

		struct LayoutResult {
			Vector2f visible_overflow_size;
			Optional<Box> box;
		};
		struct FitWidthResult {
			float max_content_width;
		};
		bool from_cache = false;
		Optional<LayoutResult> layout_result;
		Optional<FitWidthResult> fit_width_result;
	};

	Entry& PushEntry(FormatType format_type, ContainerBox* parent_container, Element* element, const Box* override_initial_box,
		FormattingContextType type);
	void CloseEntry(Entry& entry, LayoutBox* layout_box);
	void CloseEntry(Entry& entry, float max_content_width, bool from_cache);

	const Deque<Entry>& GetEntries() const { return entries; }
	String ToString() const;
	void LogMessage() const;
	int CountEntries() const;
	int CountCachedEntries() const;
	int CountFormattedEntries() const;

	void Reset();

private:
	Deque<Entry> entries;
	int current_stack_level = 0;
};

class ScopedFormatIndependentDebugTracker {
public:
	ScopedFormatIndependentDebugTracker(ContainerBox* parent_container, Element* element, const Box* override_initial_box, FormattingContextType type)
	{
		if (auto debug_tracker = FormattingContextDebugTracker::GetIf())
		{
			tracker_entry = &debug_tracker->PushEntry(FormattingContextDebugTracker::FormatType::FormatIndependent, parent_container, element,
				override_initial_box, type);
		}
	}

	void CloseEntry(LayoutBox* layout_box)
	{
		if (tracker_entry)
			FormattingContextDebugTracker::GetIf()->CloseEntry(*tracker_entry, layout_box);
	}

private:
	FormattingContextDebugTracker::Entry* tracker_entry = nullptr;
};

class ScopedFormatFitContentWidthDebugTracker {
public:
	ScopedFormatFitContentWidthDebugTracker(Element* element, FormattingContextType type, Box& box)
	{
		if (auto debug_tracker = FormattingContextDebugTracker::GetIf())
		{
			tracker_entry = &debug_tracker->PushEntry(FormattingContextDebugTracker::FormatType::FormatFitContentWidth, nullptr, element, &box, type);
		}
	}

	void SetCacheHit() { from_cache = true; }

	void CloseEntry(float max_content_width)
	{
		if (tracker_entry)
			FormattingContextDebugTracker::GetIf()->CloseEntry(*tracker_entry, max_content_width, from_cache);
	}

private:
	bool from_cache = false;
	FormattingContextDebugTracker::Entry* tracker_entry = nullptr;
};

void DebugLogDirtyLayoutTree(Element* root_element);

#else

class ScopedFormatIndependentDebugTracker {
public:
	ScopedFormatIndependentDebugTracker(ContainerBox* /*parent_container*/, Element* /* element*/, const Box* /* override_initial_box*/,
		FormattingContextType /* type*/)
	{}
	void CloseEntry(LayoutBox* /*layout_box*/) {}
};

class ScopedFormatFitContentWidthDebugTracker {
public:
	ScopedFormatFitContentWidthDebugTracker(Element* /*element*/, FormattingContextType /*type*/, Box& /*box*/) {}
	void SetCacheHit() {}
	void CloseEntry(float /*max_content_width*/) {}
};

inline void DebugLogDirtyLayoutTree(Element* /*root_element*/) {}

#endif // RMLUI_DEBUG

} // namespace Rml
#endif
