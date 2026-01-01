#pragma once

#include "../Core/CallbackTexture.h"
#include "../Core/Element.h"
#include "../Core/Geometry.h"
#include "../Core/Header.h"

namespace rlottie {
class Animation;
}

namespace Rml {

class RMLUICORE_API ElementLottie : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementLottie, Element)

	ElementLottie(const String& tag);
	virtual ~ElementLottie();

	/// Returns the element's inherent size.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

	/// Loads the current source file if needed. This normally happens automatically during layouting.
	void EnsureSourceLoaded();

protected:
	/// Updates the animation.
	void OnUpdate() override;

	/// Renders the animation.
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
	// Loads the element's animation, as specified by the 'src' attribute.
	bool LoadAnimation();
	// Update the texture for the next animation frame when necessary.
	void UpdateTexture();

	bool animation_dirty = false;
	bool geometry_dirty = false;
	bool texture_size_dirty = false;

	// The texture this element is rendering from.
	CallbackTexture texture;
	// The texture data buffer.
	size_t texture_data_size = 0;
	UniquePtr<byte[]> texture_data;

	// The animation's intrinsic dimensions.
	Vector2f intrinsic_dimensions;
	// The element's size for rendering.
	Vector2i render_dimensions;

	// The geometry used to render this element.
	Geometry geometry;

	// The absolute time when the current animation was first displayed.
	double time_animation_start = -1;
	// The previous animation frame displayed.
	size_t prev_animation_frame = size_t(-1);

	UniquePtr<rlottie::Animation> animation;
};

} // namespace Rml
