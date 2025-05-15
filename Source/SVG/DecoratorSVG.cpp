/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019- The RmlUi Team, and contributors
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

#include "DecoratorSVG.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "SVGCache.h"

namespace Rml {
namespace SVG {

	DecoratorSVG::DecoratorSVG(const String& source, const bool crop_to_content) : source_path(source), crop_to_content(crop_to_content)
	{
		RMLUI_ASSERT(!source_path.empty());
	}

	DecoratorSVG::~DecoratorSVG() {}

	DecoratorDataHandle DecoratorSVG::GenerateElementData(Element* element, BoxArea paint_area) const
	{
		SharedPtr<SVGData> handle = SVGCache::GetHandle(source_path, element, crop_to_content, paint_area);
		if (!handle)
			return {};

		Data* data = new Data{
			std::move(handle),
			paint_area,
		};

		return reinterpret_cast<DecoratorDataHandle>(data);
	}

	void DecoratorSVG::ReleaseElementData(DecoratorDataHandle element_data) const
	{
		Data* data = reinterpret_cast<Data*>(element_data);
		delete data;
	}

	void DecoratorSVG::RenderElement(Element* element, DecoratorDataHandle element_data) const
	{
		Data* data = reinterpret_cast<Data*>(element_data);
		RMLUI_ASSERT(data && data->handle);
		data->handle->geometry.Render(element->GetAbsoluteOffset(data->paint_area), data->handle->texture);
	}

	DecoratorSVGInstancer::DecoratorSVGInstancer()
	{
		source_id = RegisterProperty("source", "").AddParser("string").GetId();
		crop_id = RegisterProperty("crop", "crop-none").AddParser("keyword", "crop-none, crop-to-content").GetId();
		RegisterShorthand("decorator", "source, crop", ShorthandType::FallThrough);
	}

	DecoratorSVGInstancer::~DecoratorSVGInstancer() {}

	SharedPtr<Decorator> DecoratorSVGInstancer::InstanceDecorator(const String&, const PropertyDictionary& properties,
		const DecoratorInstancerInterface&)
	{
		String source = properties.GetProperty(source_id)->Get<String>();
		if (source.empty())
			return nullptr;

		const bool crop_to_content = properties.GetProperty(crop_id)->Get<int>() != 0;

		return MakeShared<DecoratorSVG>(source, crop_to_content);
	}

} // namespace SVG
} // namespace Rml
