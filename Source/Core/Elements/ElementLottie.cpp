#include "ElementLottie.h"
#include "../TextureDatabase.h"
#include "../../../Include/RmlUi/Core/URL.h"
#include "../../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../../Include/RmlUi/Core/ElementDocument.h"
#include "../../../Include/RmlUi/Core/StyleSheet.h"

namespace Rml
{
	ElementLottie::ElementLottie(const String& tag) : Element(tag), geometry(this), m_dimensions(200.0f, 200.0f), m_is_need_recreate_texture(true), m_is_need_recreate_geometry(true)
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
		if (this->m_is_need_recreate_geometry)
			this->GenerateGeometry();

		this->LoadTexture();
		geometry.Render(GetAbsoluteOffset(Box::CONTENT).Round());
	}

	void ElementLottie::OnUpdate()
	{

	}

	void ElementLottie::OnResize()
	{
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
/*		this->m_is_need_recreate_texture = false;*/
		static float counter = 0.0f;
		if (counter > 0.995f)
			counter = 0.0f;

		counter += 0.01f;

		const String& attiribute_value_name = GetAttribute<String>("src", "C:\\Users\\lord\\RmlUi\\Samples\\assets\\lottie.json");

		if (attiribute_value_name.empty())
			return false;

		this->m_p_lottie = rlottie::Animation::loadFromFile(attiribute_value_name.c_str());

		if (this->m_p_lottie == nullptr)
		{
			Log::Message(Rml::Log::Type::LT_WARNING, "can't load lottie file properly");
			return false;
		}

		auto p_callback = [this](const String& name, UniquePtr<const byte[]>& data, Vector2i& dimensions) -> bool {
			size_t bytes_per_line = m_dimensions.x * sizeof(std::uint32_t);
			std::uint32_t* p_data = static_cast<std::uint32_t*>(calloc(bytes_per_line * m_dimensions.y, sizeof(std::uint32_t)));
			rlottie::Surface surface(p_data, m_dimensions.x, m_dimensions.y, bytes_per_line);
			m_p_lottie->renderSync(m_p_lottie->frameAtPos(counter), surface);
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
}