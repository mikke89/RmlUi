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

#include "DecoratorShader.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"

namespace Rml {

Pool<ShaderElementData>& GetShaderElementDataPool()
{
	static Pool<ShaderElementData> gradient_element_data_pool(20, true);
	return gradient_element_data_pool;
}

DecoratorShader::DecoratorShader() {}

DecoratorShader::~DecoratorShader() {}

bool DecoratorShader::Initialise(String&& in_value)
{
	value = std::move(in_value);
	return true;
}

DecoratorDataHandle DecoratorShader::GenerateElementData(Element* element, BoxArea render_area) const
{
	RenderInterface* render_interface = GetRenderInterface();
	if (!render_interface)
		return INVALID_DECORATORDATAHANDLE;

	const Box& box = element->GetBox();
	const Vector2f dimensions = box.GetSize(render_area);
	CompiledShaderHandle effect_handle =
		render_interface->CompileShader("shader", Dictionary{{"value", Variant(value)}, {"dimensions", Variant(dimensions)}});

	Geometry geometry;

	const ComputedValues& computed = element->GetComputedValues();
	const byte alpha = byte(computed.opacity() * 255.f);
	GeometryUtilities::GenerateBackground(&geometry, box, Vector2f(), computed.border_radius(), Colourb(255, alpha), render_area);

	const Vector2f offset = box.GetPosition(render_area);
	for (Vertex& vertex : geometry.GetVertices())
		vertex.tex_coord = (vertex.position - offset) / dimensions;

	ShaderElementData* element_data = GetShaderElementDataPool().AllocateAndConstruct(std::move(geometry), effect_handle);

	return reinterpret_cast<DecoratorDataHandle>(element_data);
}

void DecoratorShader::ReleaseElementData(DecoratorDataHandle handle) const
{
	ShaderElementData* element_data = reinterpret_cast<ShaderElementData*>(handle);
	GetRenderInterface()->ReleaseCompiledShader(element_data->shader);
	GetShaderElementDataPool().DestroyAndDeallocate(element_data);
}

void DecoratorShader::RenderElement(Element* element, DecoratorDataHandle handle) const
{
	ShaderElementData* element_data = reinterpret_cast<ShaderElementData*>(handle);
	element_data->geometry.RenderWithShader(element_data->shader, element->GetAbsoluteOffset(BoxArea::Border));
}

DecoratorShaderInstancer::DecoratorShaderInstancer()
{
	ids.value = RegisterProperty("value", String()).AddParser("string").GetId();
	RegisterShorthand("decorator", "value", ShorthandType::FallThrough);
}

DecoratorShaderInstancer::~DecoratorShaderInstancer() {}

SharedPtr<Decorator> DecoratorShaderInstancer::InstanceDecorator(const String& /*name*/, const PropertyDictionary& properties_,
	const DecoratorInstancerInterface& /*interface_*/)
{
	const Property* p_value = properties_.GetProperty(ids.value);
	if (!p_value)
		return nullptr;

	String value = p_value->Get<String>();

	auto decorator = MakeShared<DecoratorShader>();
	if (decorator->Initialise(std::move(value)))
		return decorator;

	return nullptr;
}

} // namespace Rml
