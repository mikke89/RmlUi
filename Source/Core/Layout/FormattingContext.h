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

#ifndef RMLUI_CORE_LAYOUT_FORMATTINGCONTEXT_H
#define RMLUI_CORE_LAYOUT_FORMATTINGCONTEXT_H

#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Box;
class ContainerBox;
class LayoutBox;

enum class FormattingContextType {
	Block,
	Table,
	Flex,
	None,
};

/*
    An environment in which related boxes are layed out.
*/
class FormattingContext {
public:
	/// Format the element in an independent formatting context, generating a new layout box.
	/// @param[in] parent_container The container box which should act as the new box's parent.
	/// @param[in] element The element to be formatted.
	/// @param[in] override_initial_box Optionally set the initial box dimensions, otherwise one will be generated based on the element's properties.
	/// @param[in] backup_context If a formatting context can not be determined from the element's properties, use this context.
	/// @return A new, fully formatted layout box, or nullptr if its formatting context could not be determined, or if formatting was unsuccessful.
	static UniquePtr<LayoutBox> FormatIndependent(ContainerBox* parent_container, Element* element, const Box* override_initial_box,
		FormattingContextType backup_context);

protected:
	FormattingContext() = default;
	~FormattingContext() = default;
};

} // namespace Rml
#endif
