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

/*
    A LayoutNode TODO
*/
class LayoutNode {
public:
	enum class Type { Undefined, Root, BlockContainer, InlineContainer, FlexContainer, TableWrapper, Replaced };

	LayoutNode(Element* element) : element(element) {}

	LayoutNode* GetClosestLayoutBoundary() const;

	void ClearDirty() { dirty_flag = DirtyLayoutType::None; }
	void SetDirty(DirtyLayoutType dirty_type) { dirty_flag = dirty_flag | dirty_type; }

	bool IsDirty() const { return dirty_flag != DirtyLayoutType::None; }
	bool IsSelfDirty() const { return !(dirty_flag == DirtyLayoutType::None || dirty_flag == DirtyLayoutType::Child); }

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
};

} // namespace Rml
#endif
