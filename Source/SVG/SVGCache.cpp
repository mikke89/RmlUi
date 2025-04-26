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

#include "SVGCache.h"
#include "../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/FileInterface.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/Texture.h"
#include "../../Include/RmlUi/Core/Utilities.h"
#include "../Core/ControlledLifetimeResource.h"
#include <algorithm>
#include <lunasvg.h>

#ifdef RMLUI_SVG_DEBUG
	#define RMLUI_SVG_DEBUG_LOG(...) Rml::Log::Message(Rml::Log::LT_DEBUG, __VA_ARGS__)
#else
	#define RMLUI_SVG_DEBUG_LOG(...)
#endif

namespace Rml {
namespace SVG {
	struct SVGKey {
		String path;
		Vector2i dimensions;
		bool crop_to_content;
		ColourbPremultiplied colour;

		friend bool operator==(const SVGKey& lhs, const SVGKey& rhs)
		{
			return lhs.path == rhs.path && lhs.dimensions == rhs.dimensions && lhs.crop_to_content == rhs.crop_to_content && lhs.colour == rhs.colour;
		}
	};
} // namespace SVG
} // namespace Rml

namespace std {
template <>
struct hash<::Rml::SVG::SVGKey> {
	size_t operator()(const ::Rml::SVG::SVGKey& key) const noexcept
	{
		size_t hash = 0;
		Rml::Utilities::HashCombine(hash, key.path);
		Rml::Utilities::HashCombine(hash, key.dimensions.x);
		Rml::Utilities::HashCombine(hash, key.dimensions.y);
		Rml::Utilities::HashCombine(hash, key.crop_to_content);
		static_assert(sizeof(uint32_t) == sizeof(key.colour), "Expecting color to be 4 bytes");
		Rml::Utilities::HashCombine(hash, *reinterpret_cast<const uint32_t*>(&key.colour[0]));
		return hash;
	}
};
} // namespace std

namespace Rml {
namespace SVG {

	static SharedPtr<SVGData> GetHandle(RenderManager& render_manager, String path, Vector2i dimensions, bool crop_to_content,
		ColourbPremultiplied colour);
	static void ReleaseHandle(SVGData* handle);

	struct SVGGeometry {
		ColourbPremultiplied colour;
		UniquePtr<Geometry> geometry;
	};

	struct SVGTexture {
		Vector2i render_dimensions;
		bool crop_to_content;
		CallbackTexture texture;
		// List of geometries using this texture, one entry for each unique color.
		Vector<SVGGeometry> geometries;
	};

	struct SVGDocument {
		Vector2f intrinsic_dimensions;
		UniquePtr<lunasvg::Document> svg_document;
		// List of textures using this document, one entry for each unique render dimension plus meta-data.
		Vector<SVGTexture> textures;
	};

	struct SVGCacheData {
		// A list of SVG documents mapped by their path. Owns all geometry and textures needed for rendering.
		UnorderedMap<String, SVGDocument> documents;

		// Handles are reference-counted lookup keys and views into the SVG document resources. Handles are responsible
		// for cleaning up the resources in the documents, when nothing refers to them any longer. When a handle is
		// destroyed, it also removes itself from the handle map.
		StableUnorderedMap<SVGKey, WeakPtr<SVGData>> handles;
	};

	static ControlledLifetimeResource<SVGCacheData> svg_cache_data;

	SVGData::SVGData(Geometry& geometry, Texture texture, Vector2f intrinsic_dimensions, const SVGKey& cache_key) :
		geometry(geometry), texture(texture), intrinsic_dimensions(intrinsic_dimensions), cache_key(cache_key)
	{}

	SVGData::~SVGData()
	{
		ReleaseHandle(this);
	}

	static Vector<SVGTexture>::iterator FindSVGTexture(SVGDocument& doc, Vector2i dimensions, bool crop_to_content)
	{
		return std::find_if(doc.textures.begin(), doc.textures.end(),
			[&](const auto& entry) { return entry.render_dimensions == dimensions && entry.crop_to_content == crop_to_content; });
	}

	static Vector<SVGGeometry>::iterator FindSVGGeometry(SVGTexture& per_size_data, const ColourbPremultiplied colour)
	{
		return std::find_if(per_size_data.geometries.begin(), per_size_data.geometries.end(),
			[&](const SVGGeometry& data) { return data.colour == colour; });
	}

	static const std::string& GetSourceOr(const lunasvg::Document* svg_document, const std::string& default_value)
	{
		const auto& documents = svg_cache_data->documents;
		auto it = std::find_if(documents.begin(), documents.end(),
			[svg_document](const auto& pair) { return pair.second.svg_document.get() == svg_document; });
		if (it != documents.end())
			return it->first;
		return default_value;
	}

	static SharedPtr<SVGData> GetHandle(RenderManager& render_manager, String move_from_path, const Vector2i dimensions, const bool crop_to_content,
		const ColourbPremultiplied colour)
	{
		SVGKey key{std::move(move_from_path), dimensions, crop_to_content, colour};
		const String& path = key.path;
		auto& documents = svg_cache_data->documents;
		auto& handles = svg_cache_data->handles;

		const auto it_handle = handles.find(key);
		if (it_handle != handles.cend())
		{
			RMLUI_SVG_DEBUG_LOG("Found handle, reusing: %s, (%d, %d), %s, %#x", path.c_str(), dimensions.x, dimensions.y,
				crop_to_content ? "crop_to_content" : "crop_none", *reinterpret_cast<const uint32_t*>(&colour[0]));
			SharedPtr<SVGData> result = it_handle->second.lock();
			RMLUI_ASSERTMSG(result, "Failed to lock handle in SVG cache");
			return result;
		}

		RMLUI_SVG_DEBUG_LOG("Making new handle: %s, (%d, %d), %s, %#x", path.c_str(), dimensions.x, dimensions.y,
			crop_to_content ? "crop_to_content" : "crop_none", *reinterpret_cast<const uint32_t*>(&colour[0]));

		// Find or create a document
		auto it_svg_document = documents.find(path);
		if (it_svg_document == documents.cend())
		{
			RMLUI_SVG_DEBUG_LOG("Loading SVG document from file %s", path.c_str());
			SVGDocument doc;

			String svg_data;
			if (path.empty() || !GetFileInterface()->LoadFile(path, svg_data))
			{
				Log::Message(Rml::Log::Type::LT_WARNING, "Could not load SVG file %s", path.c_str());
				return {};
			}

			// We use a reset-release approach here in case clients use a non-std unique_ptr (lunasvg uses std::unique_ptr)
			doc.svg_document.reset(lunasvg::Document::loadFromData(svg_data).release());

			if (!doc.svg_document)
			{
				Log::Message(Rml::Log::Type::LT_WARNING, "Could not load SVG data from file %s", path.c_str());
				return {};
			}

			doc.intrinsic_dimensions.x = Math::Max(float(doc.svg_document->width()), 1.0f);
			doc.intrinsic_dimensions.y = Math::Max(float(doc.svg_document->height()), 1.0f);

			const auto it_inserted = documents.insert_or_assign(path, std::move(doc));
			RMLUI_ASSERT(it_inserted.second);

			it_svg_document = it_inserted.first;
		}

		SVGDocument& doc = it_svg_document->second;

		Vector2f intrinsic_dimensions = doc.intrinsic_dimensions;
		if (crop_to_content)
		{
			const lunasvg::Box smallest_fit = doc.svg_document->boundingBox();
			intrinsic_dimensions.x = static_cast<float>(smallest_fit.w);
			intrinsic_dimensions.y = static_cast<float>(smallest_fit.h);
		}

		// Find or create texture
		auto it_size = FindSVGTexture(doc, dimensions, crop_to_content);
		if (it_size == doc.textures.cend())
		{
			RMLUI_SVG_DEBUG_LOG("Creating per-size data for (%d, %d), %s", dimensions.x, dimensions.y,
				crop_to_content ? "crop_to_content" : "crop_none");
			SVGTexture svg_texture;
			svg_texture.render_dimensions = dimensions;
			svg_texture.crop_to_content = crop_to_content;
			svg_texture.texture = {};

			// Callback for generating texture.
			auto texture_callback = [svg_document = doc.svg_document.get(), dimensions, crop_to_content](
										const CallbackTextureInterface& texture_interface) -> bool {
				RMLUI_ASSERT(svg_document);
				RMLUI_SVG_DEBUG_LOG("Generating texture: %s, (%d, %d), %s", GetSourceOr(svg_document, "").c_str(), dimensions.x, dimensions.y,
					crop_to_content ? "crop_to_content" : "crop_none");

				if (dimensions.x == 0 || dimensions.y == 0)
					return false;

				lunasvg::Bitmap bitmap;
				if (crop_to_content)
				{
					const lunasvg::Box smallest_fit = svg_document->boundingBox();

					lunasvg::Matrix matrix(dimensions.x / svg_document->width(), 0, 0, dimensions.y / svg_document->height(), 0, 0);
					matrix.scale(svg_document->width() / smallest_fit.w, svg_document->height() / smallest_fit.h);
					matrix.translate(-smallest_fit.x, -smallest_fit.y);

					bitmap = lunasvg::Bitmap(dimensions.x, dimensions.y);
					bitmap.clear(0x00000000);
					svg_document->render(bitmap, matrix);
				}
				else
				{
					bitmap = svg_document->renderToBitmap(dimensions.x, dimensions.y);
				}

				if (!bitmap.valid() || !bitmap.data())
				{
					Log::Message(Rml::Log::Type::LT_WARNING, "Could not render SVG to bitmap: %s", GetSourceOr(svg_document, "").c_str());
					return false;
				}

				// Swap red and blue channels, assuming LunaSVG v2.3.2 or newer, to convert to RmlUi's expected RGBA-ordering.
				const size_t bitmap_byte_size = bitmap.width() * bitmap.height() * 4;
				uint8_t* bitmap_data = bitmap.data();
				for (size_t i = 0; i < bitmap_byte_size; i += 4)
					std::swap(bitmap_data[i], bitmap_data[i + 2]);

				if (!texture_interface.GenerateTexture({reinterpret_cast<const Rml::byte*>(bitmap.data()), bitmap_byte_size},
						Vector2i{bitmap.width(), bitmap.height()}))
				{
					Log::Message(Rml::Log::Type::LT_WARNING, "Could not generate texture for SVG: %s", GetSourceOr(svg_document, "").c_str());
					return false;
				}

				return true;
			};

			svg_texture.texture = render_manager.MakeCallbackTexture(std::move(texture_callback));

			doc.textures.push_back(std::move(svg_texture));
			it_size = std::prev(doc.textures.end());
		}

		// Construct and insert per-color geometry
		SVGTexture& size_data = *it_size;
		RMLUI_ASSERTMSG(FindSVGGeometry(size_data, colour) == size_data.geometries.end(),
			"We found an existing color entry in the SVG document cache, this should have been found as a cache key map entry instead.");
		SVGGeometry colour_data;
		colour_data.colour = colour;
		Mesh mesh;
		MeshUtilities::GenerateQuad(mesh, Vector2f(0), Vector2f(size_data.render_dimensions), colour, Vector2f(0), Vector2f(1));
		colour_data.geometry = MakeUnique<Geometry>(render_manager.MakeGeometry(std::move(mesh)));
		size_data.geometries.push_back(std::move(colour_data));

		// Create and insert the handle
		const auto iterator_inserted = handles.emplace(std::move(key), WeakPtr<SVGData>());
		RMLUI_ASSERTMSG(iterator_inserted.second, "Could not insert entry into the SVG cache handle map, duplicate key.");
		const SVGKey& inserted_key = iterator_inserted.first->first;
		WeakPtr<SVGData>& inserted_weak_data_pointer = iterator_inserted.first->second;

		auto svg_handle = MakeShared<SVGData>(*size_data.geometries.back().geometry.get(), size_data.texture, intrinsic_dimensions, inserted_key);
		inserted_weak_data_pointer = svg_handle;

		return svg_handle;
	}

	static void ReleaseHandle(SVGData* handle)
	{
		// There are no longer any users of the cache entry uniquely identified by the handle address. Start from the
		// tip (i.e. per-color data) and remove that entry from its parent. Move up the cache ancestry and erase any
		// entries that no longer have any children.
		auto& documents = svg_cache_data->documents;
		auto& handles = svg_cache_data->handles;
		const SVGKey& key = handle->cache_key;

		auto it_handle = handles.find(key);
		RMLUI_ASSERT(it_handle != handles.cend());

		const auto it_document = documents.find(key.path);
		RMLUI_ASSERT(it_document != documents.cend());
		SVGDocument& svg_document = it_document->second;

		RMLUI_SVG_DEBUG_LOG("Releasing handle: %s, (%d, %d), %s, %#x", key.path.c_str(), key.dimensions.x, key.dimensions.y,
			key.crop_to_content ? "crop_to_content" : "crop_none", *reinterpret_cast<const uint32_t*>(&key.colour[0]));

		auto it_texture = FindSVGTexture(svg_document, key.dimensions, key.crop_to_content);
		RMLUI_ASSERT(it_texture != svg_document.textures.cend());
		SVGTexture& svg_texture = *it_texture;

		auto it_geometry = FindSVGGeometry(svg_texture, key.colour);
		RMLUI_ASSERT(it_geometry != svg_texture.geometries.cend());

		if (svg_texture.geometries.size() > 1)
		{
			RMLUI_SVG_DEBUG_LOG("Releasing handle from geometries, size: %zu", svg_texture.geometries.size());
			std::iter_swap(it_geometry, std::prev(svg_texture.geometries.end()));
			svg_texture.geometries.pop_back();
		}
		else if (svg_document.textures.size() > 1)
		{
			RMLUI_SVG_DEBUG_LOG("Releasing handle from textures, size: %zu", svg_document.textures.size());
			std::iter_swap(it_texture, std::prev(svg_document.textures.end()));
			svg_document.textures.pop_back();
		}
		else
		{
			RMLUI_SVG_DEBUG_LOG("Releasing document");
			documents.erase(it_document);
		}

		handles.erase(it_handle);

#ifdef RMLUI_DEBUG
		size_t count_unique_entries = 0;
		for (auto& document : documents)
		{
			RMLUI_ASSERT(!document.second.textures.empty());
			for (auto& size_data : document.second.textures)
			{
				RMLUI_ASSERT(!size_data.geometries.empty());
				count_unique_entries += size_data.geometries.size();
			}
		}
		RMLUI_ASSERT(count_unique_entries == handles.size());
#endif
	}

	void SVGCache::Initialize()
	{
		svg_cache_data.Initialize();
	}

	void SVGCache::Shutdown()
	{
		svg_cache_data.Shutdown();
	}

	SharedPtr<SVGData> SVGCache::GetHandle(const String& source, Element* element, const bool crop_to_content, const BoxArea area)
	{
		RenderManager* render_manager = element->GetRenderManager();
		if (!render_manager)
			return {};

		const ComputedValues& computed = element->GetComputedValues();
		const ColourbPremultiplied colour = computed.image_color().ToPremultiplied(computed.opacity());
		Vector2i dimensions(element->GetBox().GetSize(area).Round());
		if (dimensions.x == 0 || dimensions.y == 0)
			dimensions = {0, 0};

		String path;
		if (ElementDocument* document = element->GetOwnerDocument())
		{
			const String document_source_url = StringUtilities::Replace(document->GetSourceURL(), '|', ':');
			GetSystemInterface()->JoinPath(path, document_source_url, source);
		}

		return Rml::SVG::GetHandle(*render_manager, std::move(path), dimensions, crop_to_content, colour);
	}

} // namespace SVG
} // namespace Rml
