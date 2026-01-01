#pragma once

#include "../Core/Element.h"
#include "../Core/Header.h"

namespace Rml {
namespace SVG {
	struct SVGData;
}

class RMLUICORE_API ElementSVG : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementSVG, Element)

	explicit ElementSVG(const String& tag);
	~ElementSVG() override;

	/// Returns the element's inherent size.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

	/// Loads the current source file if needed. This normally happens automatically during layouting.
	void EnsureSourceLoaded();

	/// Gets the SVG XML data (as text) if using inline SVG, if using a file source this will return a blank string
	/// @param[out] content The SVG XML data (as text) or blank string
	void GetInnerRML(String& content) const override;

	/// Gets the SVG XML data (as text) if using inline SVG, if using a file source this will return a blank string
	/// @param[in] content The SVG XML data (as text) or blank string
	void SetInnerRML(const String& rml) override;

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
	/// Generate unique internal ids for SVG elements using inline SVG.
	static unsigned long internal_id_counter;

	String svg_data;
	bool svg_dirty = false;

	SharedPtr<SVG::SVGData> handle;
};
} // namespace Rml
