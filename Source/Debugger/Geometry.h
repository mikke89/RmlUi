#pragma once

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Context;

namespace Debugger {

/**
    Helper class for generating geometry for the debugger.
 */

class Geometry {
public:
	// Set the context to render through.
	static void SetContext(Context* context);

	// Renders a one-pixel rectangular outline.
	static void RenderOutline(Vector2f origin, Vector2f dimensions, Colourb colour, float width);
	// Renders a box.
	static void RenderBox(Vector2f origin, Vector2f dimensions, Colourb colour);
	// Renders a box with a hole in the middle.
	static void RenderBox(Vector2f origin, Vector2f dimensions, Vector2f hole_origin, Vector2f hole_dimensions, Colourb colour);

private:
	Geometry();
};

} // namespace Debugger
} // namespace Rml
