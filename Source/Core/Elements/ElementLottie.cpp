#include "ElementLottie.h"
#include "../TextureDatabase.h"
#include "../../../Include/RmlUi/Core/URL.h"
#include "../../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../../Include/RmlUi/Core/ElementDocument.h"
#include "../../../Include/RmlUi/Core/StyleSheet.h"

namespace Rml
{
	ElementLottie::ElementLottie(const String& tag) : Element(tag), geometry(this), m_dimensions(200.0f, 200.0f), m_is_need_recreate_texture(true), m_is_need_recreate_geometry(true), m_p_raw_data(nullptr)
	{
	}

	ElementLottie::~ElementLottie(void)
	{
	}

	bool ElementLottie::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
	{
		if (this->m_is_need_recreate_texture)
			this->LoadTexture();

		dimensions = this->m_dimensions;
		
		return true;
	}

	void ElementLottie::OnRender()
	{

		geometry.Render(GetAbsoluteOffset(Box::CONTENT).Round());
		this->Play();
	}

	void ElementLottie::OnUpdate()
	{

	}

	void ElementLottie::OnResize()
	{
		this->GenerateGeometry();
	}

	void ElementLottie::OnAttributeChange(const ElementAttributes& changed_attributes)
	{
	}

	void ElementLottie::OnPropertyChange(const PropertyIdSet& changed_properties)
	{
	}

	void ElementLottie::GenerateGeometry()
	{
		geometry.Release(true);

		Vector< Vertex >& vertices = geometry.GetVertices();
		Vector< int >& indices = geometry.GetIndices();

		vertices.resize(4);
		indices.resize(6);

		// Generate the texture coordinates.
		Vector2f texcoords[2];
/*
		if (rect_source != RectSource::None)
		{
			Vector2f texture_dimensions((float)texture.GetDimensions(GetRenderInterface()).x, (float)texture.GetDimensions(GetRenderInterface()).y);
			if (texture_dimensions.x == 0.0f)
				texture_dimensions.x = 1.0f;

			if (texture_dimensions.y == 0.0f)
				texture_dimensions.y = 1.0f;

			texcoords[0].x = rect.x / texture_dimensions.x;
			texcoords[0].y = rect.y / texture_dimensions.y;

			texcoords[1].x = (rect.x + rect.width) / texture_dimensions.x;
			texcoords[1].y = (rect.y + rect.height) / texture_dimensions.y;
		}
		else
		{*/
			texcoords[0] = Vector2f(0.0f, 0.0f);
			texcoords[1] = Vector2f(1.0f, 1.0f);
/*
		}
*/

		const ComputedValues& computed = GetComputedValues();

		float opacity = computed.opacity;
		Colourb quad_colour = computed.image_color;
		quad_colour.alpha = (byte)(opacity * (float)quad_colour.alpha);

		Vector2f quad_size = GetBox().GetSize(Box::CONTENT).Round();

		GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0, 0), quad_size, quad_colour, texcoords[0], texcoords[1]);

		this->m_is_need_recreate_geometry = false;
	}

	bool ElementLottie::LoadTexture()
	{
		this->m_is_need_recreate_texture = false;

		const String& attiribute_value_name = GetAttribute<String>("src", "C:\\Users\\lord\\RmlUi\\Samples\\assets\\lottie.json");

		if (attiribute_value_name.empty())
			return false;

		this->m_file_name = attiribute_value_name;
		this->m_p_lottie = rlottie::Animation::loadFromFile(attiribute_value_name.c_str());

		if (this->m_p_lottie == nullptr)
		{
			Log::Message(Rml::Log::Type::LT_WARNING, "can't load lottie file properly");
			return false;
		}

		auto p_callback = [this](const String& name, UniquePtr<const byte[]>& data, Vector2i& dimensions) -> bool {
			size_t bytes_per_line = m_dimensions.x * sizeof(std::uint32_t);
			std::uint32_t* p_data = new std::uint32_t[bytes_per_line * m_dimensions.y];
			this->m_p_raw_data = p_data;
/*			rlottie::Surface surface(p_data, m_dimensions.x, m_dimensions.y, bytes_per_line);*/
/*			m_p_lottie->renderSync(m_p_lottie->frameAtPos(counter), surface);*/
			const Rml::byte* p_result = reinterpret_cast<Rml::byte*>(p_data);
			data.reset(p_result);

			dimensions.x = m_dimensions.x;
			dimensions.y = m_dimensions.y;


			return true;
		};

		this->texture.Set(attiribute_value_name, p_callback);

		this->geometry.SetTexture(&this->texture);

		return true;
	}

	void ElementLottie::UpdateRect()
	{
	}

	void ElementLottie::Play(void)
	{
		if (this->m_p_raw_data == nullptr)
			return;

		static std::uint32_t current_frame = 0;
		++current_frame;
		current_frame = current_frame % this->m_p_lottie->totalFrame();
		static float pos = 0.0f;
		pos += 0.1f / this->m_p_lottie->frameRate();
		if (pos >= 1.0f)
			pos = 0.0f;

		auto p_callback = [this](const String& name, UniquePtr<const byte[]>& data, Vector2i& dimensions) -> bool {
			size_t bytes_per_line = m_dimensions.x * sizeof(std::uint32_t);
			std::uint32_t* p_data = new std::uint32_t[bytes_per_line * m_dimensions.y];
			this->m_p_raw_data = p_data;
			rlottie::Surface surface(p_data, m_dimensions.x, m_dimensions.y, bytes_per_line);
			m_p_lottie->renderSync(this->m_p_lottie->frameAtPos(pos), surface);

			size_t total_bytes = m_dimensions.x * m_dimensions.y * 4;

/*

			for (int i = 0; i < total_bytes; ++i)
			{
				p_data[i] = p_data[i] << 8 | p_data[i] >> 24;
			}
*/
			// temporary convertation, but need future fixes
			for (int i = 0; i < total_bytes; ++i)
			{
				p_data[i] = p_data[i] << 8 | p_data[i] >> 24;
			}

/* not working well
			for (int i = 0; i < total_bytes; i += 4) {
				auto a = p_data[i + 3];
				// compute only if alpha is non zero
				if (a) {
					auto r = p_data[i + 2];
					auto g = p_data[i + 1];
					auto b = p_data[i];

					if (a != 255) {  // un premultiply
						r = (r * 255) / a;
						g = (g * 255) / a;
						b = (b * 255) / a;

						p_data[i] = r;
						p_data[i + 1] = g;
						p_data[i + 2] = b;

					}
					else {
						// only swizzle r and b
						p_data[i] = r;
						p_data[i + 2] = b;
					}
				}
			}*/
			const Rml::byte* p_result = reinterpret_cast<Rml::byte*>(p_data);
			data.reset(p_result);

			dimensions.x = m_dimensions.x;
			dimensions.y = m_dimensions.y;


			return true;
		};

		this->texture.Set(this->m_file_name, p_callback);

		this->geometry.SetTexture(&this->texture);
	}
}