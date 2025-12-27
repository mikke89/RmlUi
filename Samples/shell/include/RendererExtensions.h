#pragma once

#include <RmlUi/Core/Types.h>

namespace Rml 
{
class RenderInterface;
}

namespace RendererExtensions {

// Extensions used by the test suite
struct Image {
	int width = 0;
	int height = 0;
	int num_components = 0;
	int row_pitch = -1;
	Rml::UniquePtr<Rml::byte[]> data;
};
Image CaptureScreen(Rml::RenderInterface* p_render_interface);

} // namespace RendererExtensions
