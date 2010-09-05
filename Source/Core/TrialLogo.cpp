/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "precompiled.h"
#include "TrialLogo.h"
#include "FontFaceHandle.h"
#include "LayoutEngine.h"
#include "TrialLogoTexture.h"
#include <Rocket/Core.h>

namespace Rocket {
namespace Core {

const float LOGO_BEGIN = 5;
const float LOGO_END = 25;
const float LOGO_SLIDE = 1.5f;

TrialLogo::TrialLogo(Context* _context) : logo_geometry(_context)
{
	context = _context;
	start_time = GetSystemInterface()->GetElapsedTime();

	logo_texture.Load("?trial::");

	logo_geometry.SetTexture(&logo_texture);
	std::vector< Vertex >& vertices = logo_geometry.GetVertices();
	std::vector< int >& indices = logo_geometry.GetIndices();

	vertices.resize(4);
	indices.resize(6);

	GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0, 0), Vector2f(128, 128), Colourb(255, 255, 255), Vector2f(0, 0), Vector2f(1, 1));
}

TrialLogo::~TrialLogo()
{
}

void TrialLogo::Render()
{
	float time_delta = GetSystemInterface()->GetElapsedTime() - start_time;
	if (time_delta < LOGO_BEGIN ||
		time_delta > LOGO_END)
		return;

	const Vector2i& dimensions = context->GetDimensions();
	if (dimensions.x < 256 ||
		dimensions.y < 256)
		return;

	Vector2f logo_position((float) dimensions.x - 128, (float) dimensions.y - 96);
	if (time_delta < LOGO_BEGIN + LOGO_SLIDE)
		logo_position.x += (1.0f - (time_delta - LOGO_BEGIN) / LOGO_SLIDE) * 128;
	if (time_delta > LOGO_END - LOGO_SLIDE)
		logo_position.y += ((time_delta - (LOGO_END - LOGO_SLIDE)) / LOGO_SLIDE) * 96;

	logo_geometry.Render(logo_position);
}

void TrialLogo::GenerateTexture(const byte*& texture_data, Vector2i& texture_dimensions)
{
	texture_data = (const byte*) logo_source;
	texture_dimensions.x = 128;
	texture_dimensions.y = 128;
}

}
}
