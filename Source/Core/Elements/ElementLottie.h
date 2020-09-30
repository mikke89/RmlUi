#pragma once

#include "../../../Include/RmlUi/Core/Header.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/Geometry.h"
#include "../../../Include/RmlUi/Core/Texture.h"
#include "../../../Include/RmlUi/Core/Spritesheet.h"

#include <rlottie.h>

namespace Rml
{
	class RMLUICORE_API ElementLottie : public Element
	{
	public:
		RMLUI_RTTI_DefineWithParent(ElementLottie, Element);
		ElementLottie(const String& tag);

		virtual ~ElementLottie(void);

		/// Returns the element's inherent size.
		bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

	protected:
		/// Renders the image.
		void OnRender() override;

		/// Regenerates the element's geometry.
		void OnResize() override;

		/// Checks for changes to the image's source or dimensions.
		/// @param[in] changed_attributes A list of attributes changed on the element.
		void OnAttributeChange(const ElementAttributes& changed_attributes) override;

		/// Called when properties on the element are changed.
		/// @param[in] changed_properties The properties changed on the element.
		void OnPropertyChange(const PropertyIdSet& changed_properties) override;

	private:
		// Generates the element's geometry.
		void GenerateGeometry();
		// Loads the element's texture, as specified by the 'src' attribute.
		bool LoadTexture();
		// Loads the rect value from the element's attribute, but only if we're not a sprite.
		void UpdateRect();


	private:
		bool m_is_need_recreate_texture;
		bool m_is_need_recreate_geometry;

		// The texture this element is rendering from.
		Texture texture;
 
		// The element's computed intrinsic dimensions. If either of these values are set to -1, then
		// that dimension has not been computed yet.
		Vector2f m_dimensions;

		// The rectangle extracted from the sprite or 'rect' attribute. The rect_source will be None if
		// these have not been specified or are invalid.
		Rectangle rect;

		// The geometry used to render this element.
		Geometry geometry;

		std::unique_ptr<rlottie::Animation> m_p_lottie;
	};
}