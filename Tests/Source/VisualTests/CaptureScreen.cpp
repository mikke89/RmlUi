#include "CaptureScreen.h"
#include "TestConfig.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RendererExtensions.h>
#include <Shell.h>
#include <cmath>

#define LODEPNG_NO_COMPILE_CPP
#include <lodepng.h>

bool CaptureScreenshot(const Rml::String& filename, int clip_width)
{
	using Image = RendererExtensions::Image;

	Image image_orig = RendererExtensions::CaptureScreen();

	if (!image_orig.data)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screenshot of window.");
		return false;
	}

	if (clip_width == 0)
		clip_width = image_orig.width;

	// Create a new image flipped vertically, and clipped to the given clip width.
	Image image;
	image.width = clip_width;
	image.height = image_orig.height;
	image.num_components = image_orig.num_components;
	image.data = Rml::UniquePtr<Rml::byte[]>(new Rml::byte[image.width * image.height * image.num_components]);

	const int c = image.num_components;
	for (int y = 0; y < image.height; y++)
	{
		const int flipped_y = image_orig.height - y - 1;

		const int yb = y * image.width * c;
		const int yb_orig = flipped_y * image_orig.width * c;
		const int wb = image.width * c;

		for (int xb = 0; xb < wb; xb++)
		{
			image.data[yb + xb] = image_orig.data[yb_orig + xb];
		}
	}

	const Rml::String output_path = GetCaptureOutputDirectory() + "/" + filename;
	unsigned int lodepng_result = lodepng_encode24_file(output_path.c_str(), image.data.get(), image.width, image.height);
	if (lodepng_result)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not write the captured screenshot to %s: %s", output_path.c_str(),
			lodepng_error_text(lodepng_result));
		return false;
	}

	return true;
}

struct DeferFree {
	unsigned char* ptr = nullptr;
	~DeferFree() { free(ptr); }
};

ComparisonResult CompareScreenToPreviousCapture(Rml::RenderInterface* render_interface, const Rml::String& filename, TextureGeometry* out_reference,
	TextureGeometry* out_highlight)
{
	using Image = RendererExtensions::Image;

	const Rml::String input_path = GetCompareInputDirectory() + "/" + filename;

	unsigned char* data_ref = nullptr;
	unsigned int w_ref = 0, h_ref = 0;

	unsigned int lodepng_result = lodepng_decode32_file(&data_ref, &w_ref, &h_ref, input_path.c_str());
	DeferFree defer_free{data_ref};

	if (lodepng_result)
	{
		ComparisonResult result;
		result.success = false;
		result.error_msg =
			Rml::CreateString("Could not read the captured screenshot from %s: %s", input_path.c_str(), lodepng_error_text(lodepng_result));
		return result;
	}
	RMLUI_ASSERT(w_ref > 0 && h_ref > 0 && data_ref);

	Image screen = RendererExtensions::CaptureScreen();
	if (!screen.data)
	{
		ComparisonResult result;
		result.success = false;
		result.error_msg = "Could not capture screenshot of window.";
		return result;
	}
	RMLUI_ASSERT(screen.num_components == 3);

	const size_t image_ref_diff_byte_size = w_ref * h_ref * 4;

	Image diff;
	diff.width = w_ref;
	diff.height = h_ref;
	diff.num_components = 4;
	diff.data = Rml::UniquePtr<Rml::byte[]>(new Rml::byte[image_ref_diff_byte_size]);

	// So we have both images now, compare them! Also create a diff image.
	// In case they are not the same size, we require that the reference image size is smaller or equal to the screen
	// in both dimensions, and we compare them at the top-left corner.
	// Note that the loaded image is flipped vertically compared to the OpenGL capture!

	if (screen.width < (int)w_ref || screen.height < (int)h_ref)
	{
		ComparisonResult result;
		result.success = false;
		result.error_msg = "Test comparison failed. The screen is smaller than the reference image in one or both dimensions.";
		return result;
	}

	const Rml::Colourb highlight_color(255, 0, 255, 255);
	size_t sum_diff = 0;
	size_t max_pixel_diff = 0;
	for (int y = 0; y < (int)h_ref; y++)
	{
		const int y_flipped_screen = screen.height - y - 1;
		for (int x = 0; x < (int)w_ref; x++)
		{
			const int i0_screen = (y_flipped_screen * screen.width + x) * 3;
			const int i0_ref = (y * w_ref + x) * 4;
			const int i0_diff = (y * diff.width + x) * 4;

			int pixel_diff = 0;
			for (int z = 0; z < 3; z++)
			{
				const Rml::byte pix_ref = data_ref[i0_ref + z];
				const Rml::byte pix_screen = screen.data[i0_screen + z];
				pixel_diff += Rml::Math::Absolute((int)pix_ref - (int)pix_screen);
			}

			diff.data[i0_diff + 0] = (pixel_diff ? highlight_color[0] : screen.data[i0_screen + 0]);
			diff.data[i0_diff + 1] = (pixel_diff ? highlight_color[1] : screen.data[i0_screen + 1]);
			diff.data[i0_diff + 2] = (pixel_diff ? highlight_color[2] : screen.data[i0_screen + 2]);
			diff.data[i0_diff + 3] = highlight_color[3];
			sum_diff += (size_t)pixel_diff;
			max_pixel_diff = Rml::Math::Max(max_pixel_diff, (size_t)pixel_diff);
		}
	}

	ComparisonResult result;
	result.skipped = false;
	result.success = true;
	result.is_equal = (sum_diff == 0);
	result.absolute_difference_sum = sum_diff;
	result.max_absolute_difference_single_pixel = max_pixel_diff;

	const size_t max_diff = size_t(3 * 255) * size_t(w_ref) * size_t(h_ref);
	result.similarity_score = (sum_diff == 0 ? 1.0 : 1.0 - std::log(double(sum_diff)) / std::log(double(max_diff)));

	// Optionally render the screen capture or diff to a texture.
	auto GenerateGeometry = [&](TextureGeometry& geometry, Rml::Span<const Rml::byte> data, Rml::Vector2i dimensions) -> bool {
		ReleaseTextureGeometry(render_interface, geometry);
		const Rml::ColourbPremultiplied colour = {255, 255, 255, 255};
		const Rml::Vector2f uv_top_left = {0, 0};
		const Rml::Vector2f uv_bottom_right = {1, 1};
		Rml::MeshUtilities::GenerateQuad(geometry.mesh, Rml::Vector2f(0, 0), Rml::Vector2f((float)w_ref, (float)h_ref), colour, uv_top_left,
			uv_bottom_right);
		geometry.texture_handle = render_interface->GenerateTexture(data, dimensions);
		geometry.geometry_handle = render_interface->CompileGeometry(geometry.mesh.vertices, geometry.mesh.indices);
		return geometry.texture_handle && geometry.geometry_handle;
	};

	if (out_reference && result.success)
		result.success = GenerateGeometry(*out_reference, {data_ref, image_ref_diff_byte_size}, {(int)w_ref, (int)h_ref});

	if (out_highlight && result.success)
		result.success = GenerateGeometry(*out_highlight, {diff.data.get(), image_ref_diff_byte_size}, {diff.width, diff.height});

	if (!result.success)
		result.error_msg = Rml::CreateString("Could not generate texture from file %s", input_path.c_str());

	return result;
}

void RenderTextureGeometry(Rml::RenderInterface* render_interface, TextureGeometry& geometry)
{
	if (geometry.geometry_handle && geometry.texture_handle)
	{
		render_interface->RenderGeometry(geometry.geometry_handle, Rml::Vector2f(0, 0), geometry.texture_handle);
	}
}

void ReleaseTextureGeometry(Rml::RenderInterface* render_interface, TextureGeometry& geometry)
{
	if (geometry.geometry_handle)
	{
		render_interface->ReleaseGeometry(geometry.geometry_handle);
		geometry.geometry_handle = 0;
	}
	if (geometry.texture_handle)
	{
		render_interface->ReleaseTexture(geometry.texture_handle);
		geometry.texture_handle = 0;
	}
}

// Suppress warnings emitted by lodepng
#if defined RMLUI_PLATFORM_WIN32_NATIVE
	#pragma warning(disable : 4334)
	#pragma warning(disable : 4267)
#endif

#include <lodepng.cpp>
