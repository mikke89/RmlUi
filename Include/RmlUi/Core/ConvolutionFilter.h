#pragma once

#include "Header.h"
#include "Types.h"

namespace Rml {

enum class FilterOperation {
	// The result is the sum of all the filtered pixels.
	Sum,
	// The result is the largest value of all filtered pixels.
	Dilation,
};

/**
    A programmable convolution filter, designed to aid in the generation of texture data by custom
    FontEffect types.
 */

class RMLUICORE_API ConvolutionFilter {
public:
	ConvolutionFilter();
	~ConvolutionFilter();

	/// Initialises a square kernel filter with the given radius.
	bool Initialise(int kernel_radius, FilterOperation operation);

	/// Initialises the filter. A filter must be initialised and populated with values before use.
	/// @param[in] kernel_radii The size of the filter's kernel on each side of the origin along both axes. So, for example, a filter initialised with
	/// radii (1,1) will store 9 values.
	/// @param[in] operation The operation the filter conducts to determine the result.
	bool Initialise(Vector2i kernel_radii, FilterOperation operation);

	/// Returns a reference to one of the rows of the filter kernel.
	/// @param[in] kernel_y_index The index of the desired row.
	/// @return Pointer to the first value in the kernel row.
	float* operator[](int kernel_y_index);

	/// Runs the convolution filter. The filter will operate on each pixel in the destination
	/// surface, setting its opacity to the result the filter on the source opacity values. The
	/// colour values will remain unchanged.
	/// @param[in] destination The RGBA-encoded destination buffer.
	/// @param[in] destination_dimensions The size of the destination region (in pixels).
	/// @param[in] destination_stride The stride (in bytes) of the destination region.
	/// @param[in] destination_color_format Determines the representation of the bytes in the destination texture, only the alpha channel will be
	/// written to.
	/// @param[in] source The opacity information for the source buffer.
	/// @param[in] source_dimensions The size of the source region (in pixels). The stride is assumed to be equivalent to the horizontal width.
	/// @param[in] source_offset The offset of the source region from the destination region. This is usually the same as the kernel size.
	/// @param[in] source_color_format Determines the representation of the bytes in the source texture, only the alpha channel will be used.
	void Run(byte* destination, Vector2i destination_dimensions, int destination_stride, ColorFormat destination_color_format, const byte* source,
		Vector2i source_dimensions, Vector2i source_offset, ColorFormat source_color_format) const;

private:
	Vector2i kernel_size;
	UniquePtr<float[]> kernel;

	FilterOperation operation = FilterOperation::Sum;
};

} // namespace Rml
