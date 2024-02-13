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

#ifndef RMLUI_CORE_LAYOUT_FLEXFORMATTINGCONTEXT_H
#define RMLUI_CORE_LAYOUT_FLEXFORMATTINGCONTEXT_H

#include "../../../Include/RmlUi/Core/Types.h"
#include "FormattingContext.h"

namespace Rml {

class LayoutBox;
class ContainerBox;
class FlexContainer;

/*
    Formats a flex container element and its flex items according to flexible box (flexbox) layout rules.
*/
class FlexFormattingContext final : public FormattingContext {
public:
	/// Formats a flex container element and its flex items according to flexbox layout rules.
	static UniquePtr<LayoutBox> Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box);

	/// Computes max-content size for a flex container.
	static Vector2f GetMaxContentSize(Element* element);

private:
	FlexFormattingContext() = default;

	/// Format the flexbox and its children.
	/// @param[out] flex_resulting_content_size The final content size of the flex container.
	/// @param[out] flex_content_overflow_size Overflow size in case flex items or their contents overflow the container.
	/// @param[out] flex_baseline The baseline of the flex contaienr, in terms of the vertical distance from its top-left border corner.
	void Format(Vector2f& flex_resulting_content_size, Vector2f& flex_content_overflow_size, float& flex_baseline) const;

	Vector2f flex_available_content_size;
	Vector2f flex_content_containing_block;
	Vector2f flex_content_offset;
	Vector2f flex_min_size;
	Vector2f flex_max_size;

	Element* element_flex = nullptr;
	FlexContainer* flex_container_box = nullptr;
};

} // namespace Rml
#endif
