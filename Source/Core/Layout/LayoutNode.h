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

	Vector2f visible_overflow_size;
	Optional<float> baseline_of_last_line;
};

/*
    A LayoutNode TODO
*/
class LayoutNode {
public:
	enum class Type { Undefined, Root, BlockContainer, InlineContainer, FlexContainer, TableWrapper, Replaced };

	LayoutNode(Element* element) : element(element) {}

	void DirtyUpToClosestLayoutBoundary();

	void ClearDirty() { dirty_flag = DirtyLayoutType::None; }
	void SetDirty(DirtyLayoutType dirty_type) { dirty_flag = dirty_flag | dirty_type; }

	bool IsDirty() const { return dirty_flag != DirtyLayoutType::None; }
	bool IsSelfDirty() const { return !(dirty_flag == DirtyLayoutType::None || dirty_flag == DirtyLayoutType::Child); }

	void CommitLayout(Vector2f containing_block_size, Vector2f absolutely_positioning_containing_block_size, const Box* override_box,
		Vector2f visible_overflow_size, Optional<float> baseline_of_last_line)
	{
		committed_layout.emplace(CommittedLayout{
			containing_block_size,
			absolutely_positioning_containing_block_size,
			override_box ? Optional<Box>(*override_box) : Optional<Box>(),
			visible_overflow_size,
			baseline_of_last_line,
		});
		ClearDirty();
	}

	bool CommittedLayoutMatches(Vector2f containing_block_size, Vector2f absolutely_positioning_containing_block_size, const Box* override_box) const
	{
		if (IsDirty())
			return false;
		if (!committed_layout.has_value())
			return false;
		if (committed_layout->containing_block_size != containing_block_size ||
			committed_layout->absolutely_positioning_containing_block_size != absolutely_positioning_containing_block_size)
			return false;

		if (!override_box)
			return !committed_layout->override_box.has_value();

		const Box& compare_box = committed_layout->override_box.has_value() ? *committed_layout->override_box : element->GetBox();
		if (!override_box->EqualAreaEdges(*committed_layout->override_box))
			return false;

		if (override_box->GetSize() == compare_box.GetSize())
			return true;

		// Lastly, if we have an indefinite size on the committed box, see if the layed-out size matches the input override box.
		Vector2f compare_size = compare_box.GetSize();
		if (compare_size.x < 0.f)
			compare_size.x = element->GetBox().GetSize().x;
		if (compare_size.y < 0.f)
			compare_size.y = element->GetBox().GetSize().y;

		return override_box->GetSize() == compare_size;
	}

	const Optional<CommittedLayout>& GetCommittedLayout() const { return committed_layout; }

	// A.k.a. reflow root.
	bool IsLayoutBoundary() const;

private:
	Element* element;
	Type type = Type::Undefined;

	DirtyLayoutType dirty_flag = DirtyLayoutType::None;

	// TODO: Cache this after running LayoutDetails::GetShrinkToFitWidth. Clear the cache whenever any child or self is dirtied.
	float max_content_size = 0;

	// Store this for partial layout updates / reflow. If this changes, always do layout again, regardless of any cache.
	Vector2f containing_block;

	Optional<CommittedLayout> committed_layout;
};

} // namespace Rml
#endif
