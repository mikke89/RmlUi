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
#include "../../../Include/RmlUi/Core/StringUtilities.h"

namespace Rml {
#ifdef RMLUI_DEBUG

static FormatIndependentDebugTracker* g_debug_format_independent_tracker = nullptr;

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

FormatIndependentDebugTracker::Entry& FormatIndependentDebugTracker::PushEntry(FormatType format_type, ContainerBox* parent_container,
	Element* element, const Box* override_initial_box, FormattingContextType type)
{
	Entry& result = entries.emplace_back(FormatIndependentDebugTracker::Entry{
		current_stack_level,
		format_type,
		parent_container && parent_container->GetElement() ? parent_container->GetElement()->GetAddress() : "",
		parent_container && parent_container->GetAbsolutePositioningContainingBlockElementForDebug()
			? parent_container->GetAbsolutePositioningContainingBlockElementForDebug()->GetAddress()
			: "",
		parent_container ? parent_container->GetContainingBlockSize(Style::Position::Static) : Optional<Vector2f>{},
		parent_container ? parent_container->GetContainingBlockSize(Style::Position::Absolute) : Optional<Vector2f>{},
		element->GetAddress(),
		override_initial_box ? Optional<Box>{*override_initial_box} : std::nullopt,
		type,
	});
	current_stack_level += 1;
	return result;
}

void FormatIndependentDebugTracker::CloseEntry(Entry& entry, LayoutBox* layout_box)
{
	current_stack_level -= 1;
	if (layout_box)
	{
		entry.from_cache = (layout_box->GetType() == LayoutBox::Type::CachedContainer);
		entry.layout_result = Optional<FormatIndependentDebugTracker::Entry::LayoutResult>({
			layout_box->GetVisibleOverflowSize(),
			layout_box->GetIfBox() ? Optional<Box>{*layout_box->GetIfBox()} : std::nullopt,
		});
	}
}
void FormatIndependentDebugTracker::CloseEntry(Entry& entry, float max_content_width, bool from_cache)
{
	current_stack_level -= 1;
	entry.from_cache = from_cache;
	entry.fit_width_result = Optional<FormatIndependentDebugTracker::Entry::FitWidthResult>({
		max_content_width,
	});
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
		auto OptionalVec2ToString = [](const Optional<Vector2f>& value) -> String {
			if (value.has_value())
				return Rml::ToString(value->x) + " x " + Rml::ToString(value->y);
			return "none";
		};
		const std::string newline = '\n' + StringUtilities::RepeatString("|   ", entry.level);
		result += '\n' + StringUtilities::RepeatString("+ - ", entry.level) + entry.element;
		result +=
			newline + "|   Format type: " + (entry.format_type == FormatType::FormatIndependent ? "FormatIndependent" : "FormatFitContentWidth");
		result += newline + "|   Containing block: " + OptionalVec2ToString(entry.containing_block) + ". " + entry.parent_container_element;
		result += newline + "|   Absolute positioning containing block: " + OptionalVec2ToString(entry.absolute_positioning_containing_block) + ". " +
			entry.absolute_positioning_containing_block_element;

		result += newline + "|   Formatting context: ";
		switch (entry.context_type)
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

		if (entry.layout_result)
		{
			result += newline + "|   Layout result" + (entry.from_cache ? " (cached)" : "") + ":";
			result += newline + "|       Visible overflow: " + Rml::ToString(entry.layout_result->visible_overflow_size.x) + " x " +
				Rml::ToString(entry.layout_result->visible_overflow_size.y);

			if (entry.layout_result->box)
			{
				const Box& box = *entry.layout_result->box;
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
		else if (entry.fit_width_result)
		{
			result += newline + "|   Fit-width result" + (entry.from_cache ? " (cached)" : "") + ":";
			result += newline + "|       Max-content width: " + Rml::ToString(entry.fit_width_result->max_content_width);
		}
		else
		{
			result += newline + "|   Layout result: none";
		}
	}

	return result;
}

void FormatIndependentDebugTracker::LogMessage() const
{
	Log::Message(Log::LT_INFO, "%s", ToString().c_str());
}

int FormatIndependentDebugTracker::CountCachedEntries() const
{
	return (int)std::count_if(entries.begin(), entries.end(), [](const auto& entry) { return entry.from_cache; });
}

int FormatIndependentDebugTracker::CountFormattedEntries() const
{
	return (int)entries.size() - CountCachedEntries();
}

int FormatIndependentDebugTracker::CountEntries() const
{
	return (int)entries.size();
}

#endif // RMLUI_DEBUG
} // namespace Rml
