#pragma once

#include "../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

struct BoxShadowRenderable;

class ElementBackgroundBorder {
public:
	ElementBackgroundBorder();
	void Render(Element* element);

	void DirtyBackground();
	void DirtyBorder();

	Geometry* GetClipGeometry(Element* element, BoxArea clip_area);

private:
	enum class BackgroundType { BackgroundBorder, BoxShadowAndBackgroundBorder, ClipBorder, ClipPadding, ClipContent, Count };
	struct Background {
		Geometry geometry;
		Texture texture;
		SharedPtr<BoxShadowRenderable> box_shadow_and_background_border;
	};

	Background* GetBackground(BackgroundType type);
	Background& GetOrCreateBackground(BackgroundType type);
	void EraseBackground(BackgroundType type);

	void GenerateGeometry(Element* element);

	bool background_dirty = false;
	bool border_dirty = false;

	StableMap<BackgroundType, Background> backgrounds;
};

} // namespace Rml
