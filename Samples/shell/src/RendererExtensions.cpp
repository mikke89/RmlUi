#include "../include/RendererExtensions.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/RenderInterface.h>

RendererExtensions::Image RendererExtensions::CaptureScreen(Rml::RenderInterface* p_render_interface)
{
	if (p_render_interface)
	{
		Image img;

		Rml::byte* p_image_data;
		size_t image_data_size;

		bool status = p_render_interface->CaptureScreen(img.width, img.height, img.num_components, img.row_pitch, p_image_data, image_data_size);
		RMLUI_ASSERT(status && "failed to make a screenshot (some variants why: driver failure, early calling, OS failure)");

		if (!status)
			return Image();

		img.data = Rml::UniquePtr<Rml::byte[]>(p_image_data);

		return img;
	}

	return Image();
}
