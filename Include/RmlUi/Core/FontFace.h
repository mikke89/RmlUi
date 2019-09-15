/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUICOREFONTFACE_H
#define RMLUICOREFONTFACE_H

#include "ComputedValues.h"

namespace Rml {
namespace Core {

class FontFaceHandle;

/**
	@author Peter Curry
 */

class FontFace
{
public:
    FontFace(Style::FontStyle style, Style::FontWeight weight, bool release_stream);
    virtual ~FontFace();

	/// Returns the style of the font face.
	/// @return The font face's style.
	Style::FontStyle GetStyle() const;
	/// Returns the weight of the font face.
	/// @return The font face's weight.
	Style::FontWeight GetWeight() const;

	/// Returns a handle for positioning and rendering this face at the given size.
	/// @param[in] size The size of the desired handle, in points.
	/// @return The shared font handle.
    virtual SharedPtr<FontFaceHandle> GetHandle(int size) = 0;

	/// Releases the face's FreeType face structure. This will mean handles for new sizes cannot be constructed,
	/// but existing ones can still be fetched.
    virtual void ReleaseFace() = 0;

protected:
	Style::FontStyle style;
	Style::FontWeight weight;

	bool release_stream;

	// Key is font size
	using HandleMap = UnorderedMap< int, SharedPtr<FontFaceHandle> >;
	HandleMap handles;
};

}
}

#endif
