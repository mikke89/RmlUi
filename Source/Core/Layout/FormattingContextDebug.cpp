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

#include "FormattingContextDebug.h"
#include "../../../Include/RmlUi/Core/Element.h"

namespace Rml {
#ifdef RMLUI_DEBUG

static FormatIndependentDebugTracker* g_debug_format_independent_tracker = nullptr;

static std::string repeat(const std::string& input, int count)
{
	if (count <= 0)
		return {};
	std::string result;
	result.reserve(input.size() * size_t(count));
	for (size_t i = 0; i < size_t(count); ++i)
		result += input;
	return result;
}

FormatIndependentDebugTracker::FormatIndependentDebugTracker()
{
	RMLUI_ASSERTMSG(!g_debug_format_independent_tracker, "An instance of FormatIndependentDebugTracker already exists");
	g_debug_format_independent_tracker = this;
}

FormatIndependentDebugTracker::~FormatIndependentDebugTracker()
{
	RMLUI_ASSERT(g_debug_format_independent_tracker == this);
	RMLUI_ASSERT(current_stack_level == 0);
	g_debug_format_independent_tracker = nullptr;
}

FormatIndependentDebugTracker* FormatIndependentDebugTracker::GetIf()
{
	return g_debug_format_independent_tracker;
}

void FormatIndependentDebugTracker::Reset()
{
	*this = {};
}

String FormatIndependentDebugTracker::ToString() const
{
	String result;

	for (const auto& entry : entries)
	{
		const std::string newline = '\n' + repeat("|   ", entry.level);
		result += '\n' + repeat("+ - ", entry.level) + entry.element;
		result += newline + "|   Containing block: " + Rml::ToString(entry.containing_block.x) + " x " + Rml::ToString(entry.containing_block.y) +
			". " + entry.parent_container_element;
		result += newline + "|   Absolute positioning containing block: " + Rml::ToString(entry.absolute_positioning_containing_block.x) + " x " +
			Rml::ToString(entry.absolute_positioning_containing_block.y) + ". " + entry.absolute_positioning_containing_block_element;

		result += newline + "|   Formatting context: ";
		switch (entry.type)
		{
		case FormattingContextType::Block: result += "Block"; break;
		case FormattingContextType::Table: result += "Table"; break;
		case FormattingContextType::Flex: result += "Flex"; break;
		case FormattingContextType::Replaced: result += "Replaced"; break;
		case FormattingContextType::None: result += "None"; break;
		default: result += "Unknown"; break;
		}

		if (entry.override_box)
		{
			const Box& box = *entry.override_box;
			result += newline + "|   Override box: ";
			result += Rml::ToString(box.GetSize().x) + " x " + Rml::ToString(box.GetSize().y);
			result += " (outer size: " + Rml::ToString(box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin)) + " x " +
				Rml::ToString(box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin)) + ")";
		}
		else
		{
			result += newline + "|   Override box: none";
		}

		if (entry.layout)
		{
			result += newline + "|   Layout results:";
			result += newline + "|       Visible overflow: " + Rml::ToString(entry.layout->visible_overflow_size.x) + " x " +
				Rml::ToString(entry.layout->visible_overflow_size.y);

			if (entry.layout->box)
			{
				const Box& box = *entry.layout->box;
				result += newline + "|       Layout box: ";
				result += Rml::ToString(box.GetSize().x) + " x " + Rml::ToString(box.GetSize().y);
				result += " (outer size: " + Rml::ToString(box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin)) + " x " +
					Rml::ToString(box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin)) + ")";
			}
			else
			{
				result += newline + "|       Layout box: none";
			}
		}
		else
		{
			result += newline + "|   Layout results: none";
		}
	}

	return result;
}

void FormatIndependentDebugTracker::LogMessage() const
{
	Log::Message(Log::LT_INFO, "%s", ToString().c_str());
}

int FormatIndependentDebugTracker::CountEntries() const
{
	return (int)entries.size();
}

#endif // RMLUI_DEBUG
} // namespace Rml
