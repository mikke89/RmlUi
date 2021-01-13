/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include "../../Include/RmlUi/Lottie/ElementLottie.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/FileInterface.h"
#include <cmath>
#include <rlottie.h>

namespace Rml {


ElementLottie::ElementLottie(const String& tag) : Element(tag), geometry(this)
{
}

ElementLottie::~ElementLottie()
{
}

bool ElementLottie::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	if (animation_dirty)
		LoadAnimation();

	dimensions = intrinsic_dimensions;
	if (dimensions.y > 0)
		ratio = dimensions.x / dimensions.y;

	return true;
}

void ElementLottie::OnRender()
{
	if (animation)
	{
		if (geometry_dirty)
			GenerateGeometry();

		UpdateTexture();
		geometry.Render(GetAbsoluteOffset(Box::CONTENT).Round());
	}
}

void ElementLottie::OnResize()
{
	geometry_dirty = true;
	texture_size_dirty = true;
}

void ElementLottie::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.count("src"))
	{
		animation_dirty = true;
		DirtyLayout();
	}
}

void ElementLottie::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	Element::OnPropertyChange(changed_properties);

	if (changed_properties.Contains(PropertyId::ImageColor) ||
		changed_properties.Contains(PropertyId::Opacity)) {
		geometry_dirty = true;
	}
}

void ElementLottie::GenerateGeometry()
{
	geometry.Release(true);

	Vector< Vertex >& vertices = geometry.GetVertices();
	Vector< int >& indices = geometry.GetIndices();

	vertices.resize(4);
	indices.resize(6);

	Vector2f texcoords[2] = {
		{0.0f, 0.0f},
		{1.0f, 1.0f}
	};

	const ComputedValues& computed = GetComputedValues();

	const float opacity = computed.opacity;
	Colourb quad_colour = computed.image_color;
	quad_colour.alpha = (byte)(opacity * (float)quad_colour.alpha);

	const Vector2f render_dimensions_f = GetBox().GetSize(Box::CONTENT).Round();
	render_dimensions = Vector2i(render_dimensions_f);

	GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0, 0), render_dimensions_f, quad_colour, texcoords[0], texcoords[1]);

	geometry_dirty = false;
}

bool ElementLottie::LoadAnimation()
{
	animation_dirty = false;
	intrinsic_dimensions = Vector2f{};
	geometry.SetTexture(nullptr);
	animation.reset();
	prev_animation_frame = size_t(-1);
	time_animation_start = -1;

	const String attribute_src = GetAttribute<String>("src", "");

	if (attribute_src.empty())
		return false;

	String path = attribute_src;
	String directory;

	if (ElementDocument* document = GetOwnerDocument())
	{
		const String document_source_url = StringUtilities::Replace(document->GetSourceURL(), '|', ':');
		GetSystemInterface()->JoinPath(path, document_source_url, attribute_src);
		GetSystemInterface()->JoinPath(directory, document_source_url, "");
	}

	String json_data;

	if (path.empty() || !GetFileInterface()->LoadFile(path, json_data))
	{
		Log::Message(Rml::Log::Type::LT_WARNING, "Could not load lottie file %s", path.c_str());
		return false;
	}

	animation = rlottie::Animation::loadFromData(std::move(json_data), path, directory);

	if (!animation)
	{
		Log::Message(Rml::Log::Type::LT_WARNING, "Could not construct the lottie animation %s", path.c_str());
		return false;
	}

	size_t width = 0, height = 0;
	animation->size(width, height);
	intrinsic_dimensions.x = float(width);
	intrinsic_dimensions.y = float(height);

	return true;
}

void ElementLottie::UpdateTexture()
{
	if (!animation)
		return;

	const double t = GetSystemInterface()->GetElapsedTime();

	if (time_animation_start < 0.0)
		time_animation_start = t;

	// Find the next animation frame to display.
	// Here it is possible to add more logic to control playback speed, pause/resume, and more.
	double _unused;
	// Find the normalized animation progress [0, 1].
	const double pos = std::modf((t - time_animation_start) / animation->duration(), &_unused);

	const size_t next_frame = animation->frameAtPos(pos);
	if (!texture_size_dirty && next_frame == prev_animation_frame)
	{
		// No need to update the texture if we are drawing the same frame at the same size.
		return;
	}

	// Callback for generating texture.
	auto p_callback = [this, next_frame](const String& /*name*/, UniquePtr<const byte[]>& data, Vector2i& dimensions) -> bool {
		RMLUI_ASSERT(animation);

		const size_t bytes_per_line = 4 * render_dimensions.x;
		const size_t total_bytes = bytes_per_line * render_dimensions.y;

		byte* p_data = new byte[total_bytes];

		rlottie::Surface surface(reinterpret_cast<std::uint32_t*>(p_data), render_dimensions.x, render_dimensions.y, bytes_per_line);
		animation->renderSync(next_frame, surface);

		// Swizzle the channel order from rlottie's BGRA to RmlUi's RGBA, and change pre-multiplied to post-multiplied alpha.
		for (size_t i = 0; i < total_bytes; i += 4)
		{
			// Swap the RB order for correct color channels.
			std::swap(p_data[i], p_data[i + 2]);

			const byte a = p_data[i + 3];

			// The RmlUi samples shell uses post-multiplied alpha, while rlottie serves pre-multiplied alpha.
			// Here, we un-premultiply the colors.
			if (a > 0 && a < 255)
			{
				for (size_t j = 0; j < 3; j++)
					p_data[i + j] = (p_data[i + j] * 255) / a;
			}
		}

		data.reset(p_data);
		dimensions = render_dimensions;

		return true;
	};

	texture.Set("lottie", p_callback);
	geometry.SetTexture(&texture);
	prev_animation_frame = next_frame;
	texture_size_dirty = false;
}

} // namespace Rml
