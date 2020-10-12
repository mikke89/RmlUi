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

#ifndef RMLUI_ELEMENT_LOTTIE_H
#define RMLUI_ELEMENT_LOTTIE_H

#include <RmlUi/Core/Header.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Texture.h>

#include <rlottie.h>

namespace Rml
{
	class ElementLottie : public Element
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
		void OnUpdate() override;

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

		void Play(void);

	private:
		bool m_is_need_recreate_texture;
		bool m_is_need_recreate_geometry;
		std::uint32_t* m_p_raw_data;
		String m_file_name;
		// The texture this element is rendering from.
		Texture texture;
 
		// The element's computed intrinsic dimensions. If either of these values are set to -1, then
		// that dimension has not been computed yet.
		Vector2f m_dimensions;

		// The geometry used to render this element.
		Geometry geometry;

		std::unique_ptr<rlottie::Animation> m_p_lottie;
	};
}


#endif
