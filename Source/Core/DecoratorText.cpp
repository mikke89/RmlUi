#include "DecoratorText.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/TextShapingContext.h"

namespace Rml {

DecoratorText::DecoratorText() {}

DecoratorText::~DecoratorText() {}

void DecoratorText::Initialise(String in_text, bool in_inherit_color, Colourb in_color, Vector2Numeric in_align)
{
	text = std::move(in_text);
	inherit_color = in_inherit_color;
	color = in_color;
	align = in_align;
}

DecoratorDataHandle DecoratorText::GenerateElementData(Element* element, BoxArea paint_area) const
{
	ElementData* data = new ElementData{paint_area, {}, -1};

	if (!GenerateGeometry(element, *data))
	{
		Log::Message(Log::LT_WARNING, "Could not construct text decorator with text %s on element %s", text.c_str(), element->GetAddress().c_str());
		return {};
	}

	return reinterpret_cast<DecoratorDataHandle>(data);
}

void DecoratorText::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<ElementData*>(element_data);
}

void DecoratorText::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	ElementData* data = reinterpret_cast<ElementData*>(element_data);
	if (!GenerateGeometry(element, *data))
		return;

	const Vector2f translation = element->GetAbsoluteOffset(BoxArea::Border);

	for (size_t i = 0; i < data->textured_geometry.size(); ++i)
		data->textured_geometry[i].geometry.Render(translation, data->textured_geometry[i].texture);
}

bool DecoratorText::GenerateGeometry(Element* element, ElementData& element_data) const
{
	FontFaceHandle font_face_handle = element->GetFontFaceHandle();
	if (font_face_handle == 0)
		return false;

	FontEngineInterface* font_engine_interface = GetFontEngineInterface();
	const int new_version = font_engine_interface->GetVersion(font_face_handle);
	if (new_version == element_data.font_handle_version)
		return true;

	const auto& computed = element->GetComputedValues();
	const TextShapingContext text_shaping_context{computed.language(), computed.direction(), computed.font_kerning(), computed.letter_spacing()};

	const int string_width = font_engine_interface->GetStringWidth(font_face_handle, text, text_shaping_context);

	const FontMetrics& metrics = font_engine_interface->GetFontMetrics(font_face_handle);
	const RenderBox render_box = element->GetRenderBox(element_data.paint_area);
	const Vector2f text_size = {float(string_width), metrics.ascent + metrics.descent};
	const Vector2f offset_to_align_area = render_box.GetFillOffset() + Vector2f{0, metrics.ascent};
	const Vector2f size_of_align_area = render_box.GetFillSize() - text_size;
	const Vector2f offset_within_align_area =
		Vector2f{element->ResolveNumericValue(align.x, size_of_align_area.x), element->ResolveNumericValue(align.y, size_of_align_area.y)};

	const Vector2f offset = offset_to_align_area + offset_within_align_area;
	const float opacity = computed.opacity();
	const ColourbPremultiplied text_color = (inherit_color ? computed.color() : color).ToPremultiplied(opacity);

	RenderManager& render_manager = element->GetContext()->GetRenderManager();
	TexturedMeshList mesh_list;
	font_engine_interface->GenerateString(render_manager, font_face_handle, {}, text, offset, text_color, opacity, text_shaping_context, mesh_list);

	if (mesh_list.empty())
		return false;

	Vector<TexturedGeometry> textured_geometry(mesh_list.size());
	for (size_t i = 0; i < textured_geometry.size(); i++)
	{
		textured_geometry[i].geometry = render_manager.MakeGeometry(std::move(mesh_list[i].mesh));
		textured_geometry[i].texture = mesh_list[i].texture;
	}

	element_data = ElementData{
		element_data.paint_area,
		std::move(textured_geometry),
		font_engine_interface->GetVersion(font_face_handle),
	};

	return true;
}

DecoratorTextInstancer::DecoratorTextInstancer()
{
	ids = {};
	ids.text = RegisterProperty("text", "").AddParser("string").GetId();
	ids.color = RegisterProperty("color", "inherit-color").AddParser("keyword", "inherit-color").AddParser("color").GetId();
	ids.align_x = RegisterProperty("align-x", "center").AddParser("keyword", "left, center, right").AddParser("length_percent").GetId();
	ids.align_y = RegisterProperty("align-y", "center").AddParser("keyword", "top, center, bottom").AddParser("length_percent").GetId();

	RegisterShorthand("decorator", "text, color, align-x, align-y, align-x", ShorthandType::FallThrough);
}

DecoratorTextInstancer::~DecoratorTextInstancer() {}

SharedPtr<Decorator> DecoratorTextInstancer::InstanceDecorator(const String& /*name*/, const PropertyDictionary& properties,
	const DecoratorInstancerInterface& /*instancer_interface*/)
{
	const Property* p_text = properties.GetProperty(ids.text);
	const Property* p_color = properties.GetProperty(ids.color);
	Array<const Property*, 2> p_align = {properties.GetProperty(ids.align_x), properties.GetProperty(ids.align_y)};
	if (!p_text || !p_color || !p_align[0] || !p_align[1])
		return nullptr;

	String text = StringUtilities::DecodeRml(p_text->Get<String>());
	if (text.empty())
		return nullptr;

	const bool inherit_color = (p_color->unit == Unit::KEYWORD);
	const Colourb color = (p_color->unit == Unit::COLOUR ? p_color->Get<Colourb>() : Colourb{});
	const Vector2Numeric align = ComputePosition(p_align);

	auto decorator = MakeShared<DecoratorText>();
	decorator->Initialise(std::move(text), inherit_color, color, align);
	return decorator;
}

} // namespace Rml
