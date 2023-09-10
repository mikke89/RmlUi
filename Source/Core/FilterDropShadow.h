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

#ifndef RMLUI_CORE_FILTERDROPSHADOW_H
#define RMLUI_CORE_FILTERDROPSHADOW_H

#include "../../Include/RmlUi/Core/Filter.h"
#include "../../Include/RmlUi/Core/ID.h"
#include "../../Include/RmlUi/Core/NumericValue.h"

namespace Rml {

class FilterDropShadow : public Filter {
public:
	bool Initialise(Colourb color, NumericValue offset_x, NumericValue offset_y, NumericValue sigma);

	CompiledFilter CompileFilter(Element* element) const override;

	void ExtendInkOverflow(Element* element, Rectanglef& scissor_region) const override;

private:
	Colourb color;
	NumericValue value_offset_x, value_offset_y, value_sigma;
};

class FilterDropShadowInstancer : public FilterInstancer {
public:
	FilterDropShadowInstancer();

	SharedPtr<Filter> InstanceFilter(const String& name, const PropertyDictionary& properties) override;

private:
	struct PropertyIds {
		PropertyId color, offset_x, offset_y, sigma;
	};
	PropertyIds ids;
};

} // namespace Rml
#endif
