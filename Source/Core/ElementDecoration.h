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

#ifndef RMLUI_CORE_ELEMENTDECORATION_H
#define RMLUI_CORE_ELEMENTDECORATION_H

#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/Filter.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Decorator;
class Element;
class Filter;
class CompiledFilter;

enum class RenderStage { Enter, Decoration, Exit };

/**
    Manages an elements decorator state

    @author Lloyd Weehuizen
 */

class ElementDecoration {
public:
	ElementDecoration(Element* element);
	~ElementDecoration();

	/// Instances decorators if necessary.
	void InstanceDecorators();

	/// Renders all appropriate decorators.
	void RenderDecorators(RenderStage render_stage);

	/// Mark decorators as dirty and force them to reset themselves.
	void DirtyDecorators();
	/// Mark the element data of decorators as dirty.
	void DirtyDecoratorsData();

private:
	// Releases existing element data of decorators, and regenerates it.
	void ReloadDecoratorsData();
	// Releases all existing decorators and frees their data.
	void ReleaseDecorators();

	struct DecoratorEntry {
		SharedPtr<const Decorator> decorator;
		DecoratorDataHandle decorator_data;
		BoxArea paint_area;
	};
	using DecoratorEntryList = Vector<DecoratorEntry>;

	struct FilterEntry {
		SharedPtr<const Filter> filter;
		CompiledFilter compiled;
	};
	using FilterEntryList = Vector<FilterEntry>;

	Element* element;

	// The list of decorators and filters used by this element from all style rules.
	DecoratorEntryList decorators;
	DecoratorEntryList mask_images;
	FilterEntryList filters;
	FilterEntryList backdrop_filters;

	// If set, a full reload is necessary.
	bool decorators_dirty = false;
	// If set, element data of all decorators need to be regenerated.
	bool decorators_data_dirty = false;
};

} // namespace Rml
#endif
