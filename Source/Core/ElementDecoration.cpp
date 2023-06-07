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

#include "ElementDecoration.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Filter.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"

namespace Rml {

ElementDecoration::ElementDecoration(Element* _element) : element(_element) {}

ElementDecoration::~ElementDecoration()
{
	ReleaseDecorators();
}

void ElementDecoration::InstanceDecorators()
{
	if (!decorators_dirty)
		return;

	decorators_dirty = false;
	decorators_data_dirty = true;

	RMLUI_ZoneScopedC(0xB22222);
	ReleaseDecorators();

	const ComputedValues& computed = element->GetComputedValues();

	if (computed.has_decorator())
	{
		const Property* property = element->GetLocalProperty(PropertyId::Decorator);
		if (!property || property->unit != Unit::DECORATOR)
			return;

		DecoratorsPtr decorators_ptr = property->Get<DecoratorsPtr>();
		if (!decorators_ptr)
			return;

		const StyleSheet* style_sheet = element->GetStyleSheet();
		if (!style_sheet)
			return;

		PropertySource document_source("", 0, "");
		const PropertySource* source = property->source.get();

		if (!source)
		{
			if (ElementDocument* document = element->GetOwnerDocument())
			{
				document_source.path = document->GetSourceURL();
				source = &document_source;
			}
		}

		const DecoratorPtrList& decorator_list = style_sheet->InstanceDecorators(*decorators_ptr, source);
		RMLUI_ASSERT(decorator_list.empty() || decorator_list.size() == decorators_ptr->list.size());

		for (size_t i = 0; i < decorator_list.size() && i < decorators_ptr->list.size(); i++)
		{
			const SharedPtr<const Decorator>& decorator = decorator_list[i];
			if (decorator)
			{
				DecoratorEntry entry;
				entry.decorator_data = 0;
				entry.decorator = decorator;
				entry.paint_area = decorators_ptr->list[i].paint_area;
				if (entry.paint_area == BoxArea::Auto)
					entry.paint_area = BoxArea::Padding;

				RMLUI_ASSERT(entry.paint_area >= BoxArea::Border && entry.paint_area <= BoxArea::Content);
				decorators.push_back(std::move(entry));
			}
		}
	}

	if (computed.has_filter() || computed.has_backdrop_filter())
	{
		for (const auto id : {PropertyId::Filter, PropertyId::BackdropFilter})
		{
			const Property* property = element->GetLocalProperty(id);
			if (!property || property->unit != Unit::FILTER)
				continue;

			FiltersPtr filters_ptr = property->Get<FiltersPtr>();
			if (!filters_ptr)
				continue;

			FilterEntryList& list = (id == PropertyId::Filter ? filters : backdrop_filters);
			list.reserve(filters_ptr->list.size());

			for (const FilterDeclaration& declaration : filters_ptr->list)
			{
				SharedPtr<const Filter> filter = declaration.instancer->InstanceFilter(declaration.type, declaration.properties);
				if (filter)
				{
					list.push_back({std::move(filter), CompiledFilterHandle{}});
				}
				else
				{
					const auto& source = property->source;
					Log::Message(Log::LT_WARNING, "Filter '%s' in '%s' could not be instanced, declared at %s:%d", declaration.type.c_str(),
						filters_ptr->value.c_str(), source ? source->path.c_str() : "", source ? source->line_number : -1);
				}
			}
		}
	}
}

void ElementDecoration::ReloadDecoratorsData()
{
	if (decorators_data_dirty)
	{
		decorators_data_dirty = false;

		for (DecoratorEntry& decorator : decorators)
		{
			if (decorator.decorator_data)
				decorator.decorator->ReleaseElementData(decorator.decorator_data);

			decorator.decorator_data = decorator.decorator->GenerateElementData(element, decorator.paint_area);
		}

		for (FilterEntryList* list : {&filters, &backdrop_filters})
		{
			for (FilterEntry& filter : *list)
			{
				if (filter.handle)
					filter.filter->ReleaseCompiledFilter(element, filter.handle);

				filter.handle = filter.filter->CompileFilter(element);
			}
		}
	}
}

void ElementDecoration::ReleaseDecorators()
{
	for (DecoratorEntry& decorator : decorators)
	{
		if (decorator.decorator_data)
			decorator.decorator->ReleaseElementData(decorator.decorator_data);
	}
	decorators.clear();

	for (FilterEntryList* list : {&filters, &backdrop_filters})
	{
		for (FilterEntry& filter : *list)
		{
			if (filter.handle)
				filter.filter->ReleaseCompiledFilter(element, filter.handle);
		}
		list->clear();
	}
}

void ElementDecoration::RenderDecorators(RenderStage render_stage)
{
	InstanceDecorators();
	ReloadDecoratorsData();

	if (!decorators.empty())
	{
		if (render_stage == RenderStage::Decoration)
		{
			// Render the decorators attached to this element in its current state.
			// Render from back to front for correct render order.
			for (int i = (int)decorators.size() - 1; i >= 0; i--)
			{
				DecoratorEntry& decorator = decorators[i];
				decorator.decorator->RenderElement(element, decorator.decorator_data);
			}
		}
	}

	if (filters.empty() && backdrop_filters.empty())
		return;

	RenderInterface* render_interface = ::Rml::GetRenderInterface();
	Context* context = element->GetContext();
	if (!render_interface || !context)
		return;

	RenderManager& render_manager = context->GetRenderManager();
	Rectanglei initial_scissor_region = render_manager.GetState().scissor_region;

	auto ApplyClippingRegion = [this, &render_manager, initial_scissor_region](PropertyId filter_id) {
		RMLUI_ASSERT(filter_id == PropertyId::Filter || filter_id == PropertyId::BackdropFilter);

		const bool force_clip_to_self_border_box = (filter_id == PropertyId::BackdropFilter);
		ElementUtilities::SetClippingRegion(element, force_clip_to_self_border_box);

		// Find the region being affected by the active filters and apply it as a scissor.
		Rectanglef filter_region = Rectanglef::MakeInvalid();
		ElementUtilities::GetBoundingBox(filter_region, element, BoxArea::Auto);

		// The filter property may draw outside our normal clipping region due to ink overflow.
		if (filter_id == PropertyId::Filter)
		{
			for (const auto& filter : filters)
				filter.filter->ExtendInkOverflow(element, filter_region);
		}

		Math::ExpandToPixelGrid(filter_region);

		Rectanglei scissor_region = Rectanglei(filter_region);
		scissor_region.IntersectIfValid(initial_scissor_region);
		render_manager.SetScissorRegion(scissor_region);
	};

	if (!backdrop_filters.empty())
	{
		if (render_stage == RenderStage::Enter)
		{
			ApplyClippingRegion(PropertyId::BackdropFilter);

			render_interface->PushLayer(LayerFill::Clone);

			FilterHandleList filter_handles;
			for (auto& filter : backdrop_filters)
			{
				if (filter.handle)
					filter_handles.push_back(filter.handle);
			}

			render_interface->PopLayer(BlendMode::Replace, filter_handles);

			render_manager.SetScissorRegion(initial_scissor_region);
		}
	}

	if (!filters.empty())
	{
		if (render_stage == RenderStage::Enter)
		{
			render_interface->PushLayer(LayerFill::Clear);
		}
		else if (render_stage == RenderStage::Exit)
		{
			ApplyClippingRegion(PropertyId::Filter);

			FilterHandleList filter_handles;
			for (auto& filter : filters)
			{
				if (filter.handle)
					filter_handles.push_back(filter.handle);
			}

			render_interface->PopLayer(BlendMode::Blend, filter_handles);

			render_manager.SetScissorRegion(initial_scissor_region);
		}
	}
}

void ElementDecoration::DirtyDecorators()
{
	decorators_dirty = true;
}

void ElementDecoration::DirtyDecoratorsData()
{
	decorators_data_dirty = true;
}

} // namespace Rml
