#pragma once

#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>

struct ComparisonResult {
	bool skipped = true;
	bool success = false;
	bool is_equal = false;
	double similarity_score = 0;
	size_t absolute_difference_sum = 0;
	size_t max_absolute_difference_single_pixel = 0;
	Rml::String error_msg;
};

struct TextureGeometry {
	Rml::TextureHandle texture_handle = 0;
	Rml::CompiledGeometryHandle geometry_handle = 0;
	Rml::Mesh mesh;
};

bool CaptureScreenshot(const Rml::String& filename, int clip_width);

ComparisonResult CompareScreenToPreviousCapture(Rml::RenderInterface* render_interface, const Rml::String& filename, TextureGeometry* out_reference,
	TextureGeometry* out_highlight);

void RenderTextureGeometry(Rml::RenderInterface* render_interface, TextureGeometry& geometry);

void ReleaseTextureGeometry(Rml::RenderInterface* render_interface, TextureGeometry& geometry);
