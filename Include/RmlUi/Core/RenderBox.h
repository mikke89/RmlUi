#pragma once

#include "Types.h"

namespace Rml {

// Ordered by top, right, bottom, left.
using EdgeSizes = Array<float, 4>;
// Ordered by top-left, top-right, bottom-right, bottom-left.
using CornerSizes = Array<float, 4>;

/**
    Provides the data needed to generate a mesh for a given element's box.
 */

class RenderBox {
public:
	RenderBox(Vector2f fill_size, Vector2f border_offset, EdgeSizes border_widths, CornerSizes border_radius) :
		fill_size(fill_size), border_offset(border_offset), border_widths(border_widths), border_radius(border_radius)
	{}

	/// Returns the size of the fill area of the box.
	Vector2f GetFillSize() const { return fill_size; }
	/// Sets the size of the fill area of the box.
	void SetFillSize(Vector2f value) { fill_size = value; }
	/// Returns the offset from the border area to the fill area of the box.
	Vector2f GetFillOffset() const { return {border_widths[3], border_widths[0]}; }

	/// Returns the offset to the border area of the box.
	Vector2f GetBorderOffset() const { return border_offset; }
	/// Sets the border offset.
	void SetBorderOffset(Vector2f value) { border_offset = value; }

	/// Returns the border widths of the box.
	EdgeSizes GetBorderWidths() const { return border_widths; }
	/// Sets the border widths of the box.
	void SetBorderWidths(EdgeSizes value) { border_widths = value; }

	/// Returns the border radius of the box.
	CornerSizes GetBorderRadius() const { return border_radius; }
	/// Sets the border radius of the box.
	void SetBorderRadius(CornerSizes value) { border_radius = value; }

private:
	Vector2f fill_size;
	Vector2f border_offset;
	EdgeSizes border_widths;
	CornerSizes border_radius;
};
inline bool operator==(const RenderBox& a, const RenderBox& b)
{
	return a.GetFillSize() == b.GetFillSize() && a.GetBorderOffset() == b.GetBorderOffset() && a.GetBorderWidths() == b.GetBorderWidths() &&
		a.GetBorderRadius() == b.GetBorderRadius();
}
inline bool operator!=(const RenderBox& a, const RenderBox& b)
{
	return !(a == b);
}
} // namespace Rml
