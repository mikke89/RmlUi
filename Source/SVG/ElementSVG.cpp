#include "../../Include/RmlUi/SVG/ElementSVG.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "SVGCache.h"

namespace Rml {

unsigned long ElementSVG::internal_id_counter = 0;

ElementSVG::ElementSVG(const String& tag) : Element(tag) {}
ElementSVG::~ElementSVG()
{
	handle.reset();
}

bool ElementSVG::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	EnsureSourceLoaded();

	dimensions = handle ? handle->intrinsic_dimensions : Vector2f(0);

	if (HasAttribute("width"))
	{
		dimensions.x = GetAttribute<float>("width", 0);
	}
	if (HasAttribute("height"))
	{
		dimensions.y = GetAttribute<float>("height", 0);
	}

	if (dimensions.y > 0)
		ratio = dimensions.x / dimensions.y;

	dimensions *= ElementUtilities::GetDensityIndependentPixelRatio(this);

	return true;
}

void ElementSVG::OnRender()
{
	EnsureSourceLoaded();
	if (handle)
		handle->geometry.Render(GetAbsoluteOffset(BoxArea::Content), handle->texture);
}

void ElementSVG::OnResize()
{
	svg_dirty = true;
}

void ElementSVG::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.count("src") || changed_attributes.count("crop-to-content"))
	{
		svg_dirty = true;
		DirtyLayout();
	}

	if (changed_attributes.find("width") != changed_attributes.end() || changed_attributes.find("height") != changed_attributes.end())
	{
		DirtyLayout();
	}
}

void ElementSVG::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	Element::OnPropertyChange(changed_properties);

	if (changed_properties.Contains(PropertyId::ImageColor) || changed_properties.Contains(PropertyId::Opacity))
	{
		svg_dirty = true;
	}
}

void ElementSVG::GetInnerRML(String& content) const
{
	// If the SVG is from a file source don't add anything to the content string.
	const auto source = GetAttribute<String>("src", "");
	if (!source.empty())
		return;

	content += svg_data;
}

void ElementSVG::SetInnerRML(const String& content)
{
	// If the SVG is from a file source don't set the svg xml data on the element.
	const auto source = GetAttribute<String>("src", "");
	if (!source.empty())
		return;

	// We use CreateString instead of std::to_string to avoid having to create an extra std::string and convert it to Rml::String in case clients use
	// a non-std string.
	if (!HasAttribute("rmlui-svgdata-id"))
		SetAttribute("rmlui-svgdata-id", "svgdata:" + CreateString("%lu", internal_id_counter++));

	svg_data = "" + content;
	svg_dirty = true;
	EnsureSourceLoaded();
}

void ElementSVG::EnsureSourceLoaded()
{
	if (!svg_dirty)
		return;

	svg_dirty = false;

	const bool crop_to_content = HasAttribute("crop-to-content");
	const auto source = GetAttribute<String>("src", "");
	if (source.empty())
	{
		const auto source_id = GetAttribute<String>("rmlui-svgdata-id", "svgdata:undefined");
		if (handle)
			handle.reset(); // The old handle won't be re-used so clear it.

		// Build an svg wrapper tag, copying all but src attribute (expected attributes could be width, height, viewBox, etc.)
		String svg_element_source = "<svg ";
		ElementAttributes attrs = GetAttributes();
		for (auto& attr : attrs)
		{
			if (attr.first == "src")
				continue;
			svg_element_source.append(attr.first);
			svg_element_source.append("=\"");
			svg_element_source.append(StringUtilities::EncodeRml(attr.second.Get<String>()));
			svg_element_source.append("\" ");
		}
		svg_element_source.append(">");
		svg_element_source.append(svg_data);
		svg_element_source.append("</svg>");

		handle = SVG::SVGCache::GetHandle(source_id, svg_element_source, SVG::SVGCache::Data, this, crop_to_content, BoxArea::Content);
	}
	else
	{
		handle = SVG::SVGCache::GetHandle(source, source, SVG::SVGCache::File, this, crop_to_content, BoxArea::Content);
	}
}
} // namespace Rml
