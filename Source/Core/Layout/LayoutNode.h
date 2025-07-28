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

#ifndef RMLUI_CORE_LAYOUT_LAYOUTNODE_H
#define RMLUI_CORE_LAYOUT_LAYOUTNODE_H

#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

enum class DirtyLayoutType : uint8_t {
	None = 0,
	DOM = 1 << 0,
	PropertyForcingLayout = 1 << 1,
	TableAttribute = 1 << 2,
	ReplacedElement = 1 << 3,
	IntrinsicSize = 1 << 4, // Closely tied to ReplacedElement, combine them?
	Text = 1 << 5,
	Child = 1 << 6,
};

inline DirtyLayoutType operator|(DirtyLayoutType lhs, DirtyLayoutType rhs)
{
	using T = std::underlying_type_t<DirtyLayoutType>;
	return DirtyLayoutType(T(lhs) | T(rhs));
}
inline DirtyLayoutType operator&(DirtyLayoutType lhs, DirtyLayoutType rhs)
{
	using T = std::underlying_type_t<DirtyLayoutType>;
	return DirtyLayoutType(T(lhs) & T(rhs));
}

struct CommittedLayout {
	Vector2f containing_block_size;
	Vector2f absolutely_positioning_containing_block_size;
	Optional<Box> override_box;
	bool layout_constraint;

	Vector2f visible_overflow_size;
	float max_content_width;
	Optional<float> baseline_of_last_line;
};

/*
    A LayoutNode TODO
*/
class LayoutNode {
public:
	enum class Type { Undefined, Root, BlockContainer, InlineContainer, FlexContainer, TableWrapper, Replaced };

	LayoutNode(Element* element) : element(element) {}

	// Assumes valid computed values.
	void PropagateDirtyToParent();

	void ClearDirty();
	void SetDirty(DirtyLayoutType dirty_type);

	bool IsChildDirty() const { return (dirty_flag & DirtyLayoutType::Child) != DirtyLayoutType::None; }
	bool IsDirty() const { return dirty_flag != DirtyLayoutType::None; }
	bool IsSelfDirty() const { return !(dirty_flag == DirtyLayoutType::None || dirty_flag == DirtyLayoutType::Child); }

	void CommitLayout(Vector2f containing_block_size, Vector2f absolutely_positioning_containing_block_size, const Box* override_box,
		bool layout_constraint, Vector2f visible_overflow_size, float max_content_width, Optional<float> baseline_of_last_line);

	// TODO: Currently, should only be used by `Context::SetDimensions`. Ideally, we would remove this and "commit" the
	// containing block (without the layout result, see comment in `CommitLayout`).
	void ClearCommittedLayout()
	{
		committed_layout.reset();
		committed_max_content_width.reset();
	}

	bool CommittedLayoutMatches(Vector2f containing_block_size, Vector2f absolutely_positioning_containing_block_size, const Box* override_box,
		bool layout_constraint) const
	{
		if (IsDirty())
			return false;
		if (!committed_layout.has_value())
			return false;
		if (committed_layout->containing_block_size != containing_block_size ||
			committed_layout->absolutely_positioning_containing_block_size != absolutely_positioning_containing_block_size)
			return false;

		// Layout under a constraint may make some simplifications that requires re-evaluation under a normal formatting mode.
		if (committed_layout->layout_constraint && !layout_constraint)
			return false;

		if (!override_box)
			return !committed_layout->override_box.has_value();

		const Box& compare_box = committed_layout->override_box.has_value() ? *committed_layout->override_box : element->GetBox();

		// In some situations, if we have an indefinite size on the committed box, we could see if the layed-out size
		// matches the input override box and use the cache here. However, because of cyclic-percentage rules with
		// containing block sizes, this is only correct in certain situations, particularly when the vertical size of
		// the containing block is indefinite. In this case the used containing block size should resolve to indefinite,
		// even after it is sized. Although, we don't actually implement this behavior for now, but once we do, we could
		// implement caching here in this case. For the horizontal axis, we are always required to re-evaluate any
		// children for which this box acts as a containing block for, thus we cannot use a cache mechanism here. See:
		// https://drafts.csswg.org/css-sizing/#cyclic-percentage-contribution

		return *override_box == compare_box;
	}

	bool HasCommittedLayout() const { return committed_layout.has_value(); }

	Optional<float> GetMaxContentWidthIfCached() const { return committed_max_content_width; }
	void CommitMaxContentWidth(float width) { committed_max_content_width = width; }

	Optional<float> GetMaxContentHeightIfCached() const { return committed_max_content_height; }
	void CommitMaxContentHeight(float height) { committed_max_content_height = height; }

	// TODO: Remove and replace with a better interface.
	const Optional<CommittedLayout>& GetCommittedLayout() const { return committed_layout; }

	// A.k.a. reflow root.
	bool IsLayoutBoundary() const;

private:
	Element* element;
	Type type = Type::Undefined;

	DirtyLayoutType dirty_flag = DirtyLayoutType::None;

	// Store this for partial layout updates / reflow. If this changes, always do layout again, regardless of any cache.
	Vector2f containing_block;

	Optional<CommittedLayout> committed_layout;
	Optional<float> committed_max_content_width;
	Optional<float> committed_max_content_height;
};

} // namespace Rml
#endif
