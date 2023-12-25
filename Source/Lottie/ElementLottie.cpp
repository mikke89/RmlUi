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

#include "../../Include/RmlUi/Lottie/ElementLottie.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/FileInterface.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include <cmath>
#include <rlottie.h>

namespace Rml {

ElementLottie::ElementLottie(const String& tag) : Element(tag) {}

ElementLottie::~ElementLottie() {}

bool ElementLottie::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	if (animation_dirty)
		LoadAnimation();

	dimensions = intrinsic_dimensions;
	if (dimensions.y > 0)
		ratio = dimensions.x / dimensions.y;

	return true;
}

void ElementLottie::OnUpdate()
{
	if (animation_dirty)
		LoadAnimation();

	if (!animation)
		return;

	const auto t = GetSystemInterface()->GetElapsedTime();

	if (time_animation_start < 0.0)
		time_animation_start = t;

	double _unused;
	const double frame_duration = 1.0 / animation->frameRate();
	const double delay = std::modf((t - time_animation_start) / frame_duration, &_unused) * frame_duration;
	if (IsVisible(true))
	{
		if (Context* ctx = GetContext())
			ctx->RequestNextUpdate(delay);
	}
}

void ElementLottie::OnRender()
{
	if (animation)
	{
		if (geometry_dirty)
			GenerateGeometry();

		UpdateTexture();
		geometry.Render(GetAbsoluteOffset(BoxArea::Content).Round(), texture);
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

	if (changed_properties.Contains(PropertyId::ImageColor) || changed_properties.Contains(PropertyId::Opacity))
	{
		geometry_dirty = true;
	}
}

void ElementLottie::GenerateGeometry()
{
	const ComputedValues& computed = GetComputedValues();
	ColourbPremultiplied quad_colour = computed.image_color().ToPremultiplied(computed.opacity());

	const Vector2f render_dimensions_f = GetBox().GetSize(BoxArea::Content).Round();
	render_dimensions = Vector2i(render_dimensions_f);

	Mesh mesh = geometry.Release(Geometry::ReleaseMode::ClearMesh);
	MeshUtilities::GenerateQuad(mesh, Vector2f(0), render_dimensions_f, quad_colour, Vector2f(0), Vector2f(1));
	geometry = GetRenderManager()->MakeGeometry(std::move(mesh));

	geometry_dirty = false;
}

bool ElementLottie::LoadAnimation()
{
	animation_dirty = false;
	intrinsic_dimensions = Vector2f{};
	texture = {};
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

	RenderManager* render_manager = GetRenderManager();
	if (!render_manager)
		return;

	const double t = GetSystemInterface()->GetElapsedTime();

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

	// Resize the texture buffer if necessary.
	const size_t new_texture_data_size = 4 * render_dimensions.x * render_dimensions.y;
	if (new_texture_data_size > texture_data_size)
	{
		texture_data.reset(new byte[new_texture_data_size]);
		texture_data_size = new_texture_data_size;
	}

	// Callback for generating texture.
	auto texture_callback = [this, next_frame](const CallbackTextureInterface& texture_interface) -> bool {
		RMLUI_ASSERT(animation);

		const size_t bytes_per_line = 4 * render_dimensions.x;
		const size_t total_bytes = bytes_per_line * render_dimensions.y;
		byte* p_data = texture_data.get();

		rlottie::Surface surface(reinterpret_cast<uint32_t*>(p_data), render_dimensions.x, render_dimensions.y, bytes_per_line);
		animation->renderSync(next_frame, surface);

		// Swizzle the channel order from rlottie's BGRA to RmlUi's RGBA.
		for (size_t i = 0; i < total_bytes; i += 4)
		{
			// Swap the RB order for correct color channels.
			std::swap(p_data[i], p_data[i + 2]);

#ifdef RMLUI_DEBUG
			const byte alpha = p_data[i + 3];
			for (int c = 0; c < 3; c++)
				RMLUI_ASSERTMSG(p_data[i + c] <= alpha, "Glyph data is assumed to be encoded in premultiplied alpha, but that is not the case.");
#endif
		}

		if (!texture_interface.GenerateTexture({p_data, total_bytes}, render_dimensions))
			return false;
		return true;
	};

	texture = render_manager->MakeCallbackTexture(std::move(texture_callback));

	prev_animation_frame = next_frame;
	texture_size_dirty = false;
}

} // namespace Rml
