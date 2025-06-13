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
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/RenderManager.h"

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

DecoratorDataHandle DecoratorShader::GenerateElementData(Element* element, BoxArea paint_area) const
{
	RenderManager* render_manager = element->GetRenderManager();
	if (!render_manager)
		return INVALID_DECORATORDATAHANDLE;

	const RenderBox render_box = element->GetRenderBox(paint_area);
	const Vector2f dimensions = render_box.GetFillSize();
	CompiledShader shader = render_manager->CompileShader("shader", Dictionary{{"value", Variant(value)}, {"dimensions", Variant(dimensions)}});
	if (!shader)
		return INVALID_DECORATORDATAHANDLE;

	Mesh mesh;

	const ComputedValues& computed = element->GetComputedValues();
	const byte alpha = byte(computed.opacity() * 255.f);
	MeshUtilities::GenerateBackground(mesh, render_box, ColourbPremultiplied(alpha, alpha));

	const Vector2f offset = render_box.GetFillOffset();
	for (Vertex& vertex : mesh.vertices)
		vertex.tex_coord = (vertex.position - offset) / dimensions;

	ShaderElementData* element_data =
		GetShaderElementDataPool().AllocateAndConstruct(render_manager->MakeGeometry(std::move(mesh)), std::move(shader));

	return reinterpret_cast<DecoratorDataHandle>(element_data);
}

void DecoratorShader::ReleaseElementData(DecoratorDataHandle handle) const
{
	ShaderElementData* element_data = reinterpret_cast<ShaderElementData*>(handle);
	GetShaderElementDataPool().DestroyAndDeallocate(element_data);
}

void DecoratorShader::RenderElement(Element* element, DecoratorDataHandle handle) const
{
	ShaderElementData* element_data = reinterpret_cast<ShaderElementData*>(handle);
	element_data->geometry.Render(element->GetAbsoluteOffset(BoxArea::Border), {}, element_data->shader);
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
