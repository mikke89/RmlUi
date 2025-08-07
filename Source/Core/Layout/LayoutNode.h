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
#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;

// Determines if the element, or its descendants, have been changed in such a way as to require a new layout pass.
enum class DirtyLayoutType : uint8_t {
	None = 0,
	Self = 1 << 1,  // This element has been modified such that the layout may have changed.
	Child = 1 << 2, // One or more descendent elements within the same layout boundary have been modified.
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

// The conditions and result of the last committed layout on the element.
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
    A LayoutNode maintains the layout state and cache of an Element.

    All usage of this class assumes valid computed values, except for setting its dirty state which is always allowed.
*/
class LayoutNode {
public:
	LayoutNode(Element* element) : element(element) {}

	void SetDirty(DirtyLayoutType dirty_type);
	void ClearDirty();

	bool IsDirty() const { return dirty_flag != DirtyLayoutType::None; }
	bool IsChildDirty() const { return (dirty_flag & DirtyLayoutType::Child) != DirtyLayoutType::None; }
	bool IsSelfDirty() const { return (dirty_flag & DirtyLayoutType::Self) != DirtyLayoutType::None; }

	void PropagateDirtyToParent();

	void CommitLayout(Vector2f containing_block_size, Vector2f absolutely_positioning_containing_block_size, const Box* override_box,
		bool layout_constraint, Vector2f visible_overflow_size, float max_content_width, Optional<float> baseline_of_last_line);

	// TODO: Currently, should only be used by `Context::SetDimensions`. Ideally, we would remove this and "commit" the
	// containing block (without the layout result, see comment in `CommitLayout`).
	void ClearCommittedLayout()
	{
		committed_layout.reset();
		committed_max_content_width.reset();
		committed_max_content_height.reset();
	}

	bool HasCommittedLayout() const { return committed_layout.has_value(); }
	bool CommittedLayoutMatches(Vector2f containing_block_size, Vector2f absolutely_positioning_containing_block_size, const Box* override_box,
		bool layout_constraint) const;

	Optional<float> GetCommittedMaxContentWidth() const { return committed_max_content_width; }
	void CommitMaxContentWidth(float width) { committed_max_content_width = width; }

	Optional<float> GetCommittedMaxContentHeight() const { return committed_max_content_height; }
	void CommitMaxContentHeight(float height) { committed_max_content_height = height; }

	// TODO: Remove and replace with a better interface.
	const Optional<CommittedLayout>& GetCommittedLayout() const { return committed_layout; }

	// TODO: Can we make it private?
	bool IsLayoutBoundary() const;

private:
	Element* element;

	DirtyLayoutType dirty_flag = DirtyLayoutType::None;

	Optional<CommittedLayout> committed_layout;
	Optional<float> committed_max_content_width;
	Optional<float> committed_max_content_height;
};

} // namespace Rml
#endif
