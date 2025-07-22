/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#ifndef RMLUI_CORE_LAYOUT_CONTAINERBOX_H
#define RMLUI_CORE_LAYOUT_CONTAINERBOX_H

#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "../../../Include/RmlUi/Core/Types.h"
#include "LayoutBox.h"

namespace Rml {

struct FormattingMode {
	enum class Constraint : uint8_t {
		None,
		MaxContent,
	};

	Constraint constraint = Constraint::None;
	bool allow_format_independent_cache = true;
	bool allow_max_content_cache = true;
};

/**
    Abstraction for layout boxes that can act as a containing block.

    Implements functionality for catching overflow, and for handling positioned boxes that the box acts as a containing block for.
*/
class ContainerBox : public LayoutBox {
public:
	/// Enable or disable scrollbars for the element we represent, preparing it for the first round of layouting, according to our properties.
	void ResetScrollbars(const Box& box);

	/// Adds an absolutely positioned element located within this container. The element is added to the absolute
	/// positioning containing block box, and will be formatted and positioned when closing that box, see
	/// 'ClosePositionedElements'.
	void AddAbsoluteElement(Element* element, Vector2f static_position, Element* static_relative_offset_parent);
	/// Adds a relatively positioned element which this container acts as a containing block for.
	void AddRelativeElement(Element* element);

	Element* GetElement() { return element; }

	/// Returns the size of the containing block for a box taking part in this container.
	/// @param[in] position The position property of the current box.
	/// @return The containing size, possibly indefinite (represented by negative size) along one or both axes.
	Vector2f GetContainingBlockSize(Style::Position position) const;

	/// Returns true if this container can have scrollbars enabled, as determined by its overflow properties.
	bool IsScrollContainer() const;
	/// Returns the currently active formatting mode for this container tree.
	const FormattingMode& GetFormattingMode() const { return formatting_mode; }

	void AssertMatchesParentContainer(ContainerBox* container_box) const
	{
		RMLUI_ASSERTMSG(container_box == parent_container, "Mismatched parent box.");
		(void)container_box;
	}
	Element* GetAbsolutePositioningContainingBlockElementForDebug() const { return absolute_positioning_containing_block->element; }

protected:
	ContainerBox(Type type, Element* element, ContainerBox* parent_container, const FormattingMode& formatting_mode);

	/// Checks if we have a new overflow on an auto-scrolling element. If so, our vertical scrollbar will be enabled and
	/// our block boxes will be destroyed. All content will need to be re-formatted.
	/// @param[in] content_overflow_size The size of the visible content, relative to our content area.
	/// @param[in] box The box built for the element, possibly with a non-determinate height.
	/// @param[in] max_height Maximum height of the content area, if any.
	/// @returns True if no overflow occurred, false if it did.
	bool CatchOverflow(const Vector2f content_overflow_size, const Box& box, const float max_height) const;

	/// Set the box and scrollable area on our element, possibly catching any overflow.
	/// @param[in] content_overflow_size The size of the visible content, relative to our content area.
	/// @param[in] box The box to be set on the element.
	/// @param[in] max_height Maximum height of the content area, if any.
	/// @returns True if no overflow occurred, false if it did.
	bool SubmitBox(const Vector2f content_overflow_size, const Box& box, const float max_height);

	/// Formats, sizes, and positions all absolute elements whose containing block is this, and offsets relative elements.
	void ClosePositionedElements();

	// Set the element's baseline (proxy for private access to Element).
	void SetElementBaseline(float element_baseline);

	int CountAbsolutelyPositionedBoxes() const { return (int)absolute_elements.size(); }

	// The element this box represents, if any.
	Element* const element;

private:
	struct AbsoluteElement {
		Vector2f static_position;               // The hypothetical position of the element as if it was placed in normal flow.
		Element* static_position_offset_parent; // The element for which the static position is offset from.
	};
	using AbsoluteElementMap = SmallUnorderedMap<Element*, AbsoluteElement>;

	AbsoluteElementMap absolute_elements; // List of absolutely positioned elements that we act as a containing block for.
	ElementList relative_elements;        // List of relatively positioned elements that we act as a containing block for.

	Style::Overflow overflow_x = Style::Overflow::Visible;
	Style::Overflow overflow_y = Style::Overflow::Visible;

	FormattingMode formatting_mode;

	ContainerBox* parent_container = nullptr;

	// For absolutely positioned boxes we use the first positioned ancestor. We deviate from the CSS specs where they
	// use a separate containing block for fixed boxes. In RCSS, we use the same rules on fixed boxes, as this is
	// particularly helpful on handles and other widgets that should not scroll with the window. This is a common design
	// pattern in target applications for this library.
	ContainerBox* absolute_positioning_containing_block = nullptr; // [not null]
};

/**
    The root of a tree of layout boxes, usually represents the root element or viewport.
*/
class RootBox final : public ContainerBox {
public:
	RootBox(const Box& box, FormattingMode formatting_mode) : ContainerBox(Type::Root, nullptr, nullptr, formatting_mode), box(box) {}
	RootBox(const Box& box, RootBox* absolute_root) : ContainerBox(Type::Root, nullptr, absolute_root, absolute_root->GetFormattingMode()), box(box)
	{}

	const Box* GetIfBox() const override { return &box; }
	String DebugDumpTree(int depth) const override;

	using ContainerBox::CountAbsolutelyPositionedBoxes;

private:
	Box box;
};

/**
    A box where flexbox layout occurs.
*/
class FlexContainer final : public ContainerBox {
public:
	FlexContainer(Element* element, ContainerBox* parent_container, const Box& box);

	// Submits the formatted box to the flex container element, and propagates any uncaught overflow to this box.
	// @returns True if it succeeds, otherwise false if it needs to be formatted again because scrollbars were enabled.
	bool Close(const Vector2f content_overflow_size, const Box& box, float element_baseline);

	float GetShrinkToFitWidth() const override;

	const Box* GetIfBox() const override { return &box; }
	String DebugDumpTree(int depth) const override;

	Box& GetBox() { return box; }

private:
	Box box;
};

/**
    A box where table formatting occurs.

    As opposed to a flex container, the table wrapper cannot contain overflow or produce scrollbars.
*/
class TableWrapper final : public ContainerBox {
public:
	TableWrapper(Element* element, ContainerBox* parent_container, const Box& initial_box);

	// Submits the formatted box to the table element, and propagates any uncaught overflow to this box.
	void Close(const Vector2f content_overflow_size, const Box& box, float element_baseline);

	float GetShrinkToFitWidth() const override;

	const Box* GetIfBox() const override { return &box; }
	String DebugDumpTree(int depth) const override;

	Box& GetBox() { return box; }

private:
	Box box;
};

/**
    A box which is produced after matching the existing layout.
*/
class CachedContainer final : public ContainerBox {
public:
	CachedContainer(Element* element, ContainerBox* parent_container, const Box& box, Vector2f visible_overflow_size,
		Optional<float> baseline_of_last_line) :
		ContainerBox(Type::CachedContainer, element, parent_container, parent_container->GetFormattingMode()), box(box),
		baseline_of_last_line(baseline_of_last_line)
	{
		SetVisibleOverflowSize(visible_overflow_size);
	}

	const Box* GetIfBox() const override { return &box; }
	bool GetBaselineOfLastLine(float& out_baseline) const override;
	float GetShrinkToFitWidth() const override;

	String DebugDumpTree(int depth) const override;

private:
	const Box& box;
	Optional<float> baseline_of_last_line;
};

} // namespace Rml
#endif
