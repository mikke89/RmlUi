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

#ifndef RMLUI_CORE_ELEMENTEFFECTS_H
#define RMLUI_CORE_ELEMENTEFFECTS_H

#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Decorator;
class Element;
class Filter;

enum class RenderStage { Enter, Decoration, Exit };

/**
    Manages and renders an element's effects: decorators, filters, backdrop filters, and mask images.
 */

class ElementEffects {
public:
	ElementEffects(Element* element);
	~ElementEffects();

	void InstanceEffects();

	void RenderEffects(RenderStage render_stage);

	// Mark effects as dirty and force them to reset themselves.
	void DirtyEffects();
	// Mark the element data of effects as dirty.
	void DirtyEffectsData();

private:
	// Releases existing element data of effects, and regenerates it.
	void ReloadEffectsData();
	// Releases all existing effects and their element data.
	void ReleaseEffects();

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

	// The list of decorators and filters used by this element.
	DecoratorEntryList decorators;
	DecoratorEntryList mask_images;
	FilterEntryList filters;
	FilterEntryList backdrop_filters;

	// If set, a full reload is necessary.
	bool effects_dirty = false;
	// If set, element data of all decorators need to be regenerated.
	bool effects_data_dirty = false;
};

} // namespace Rml
#endif
