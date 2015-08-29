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

#ifndef ROCKETCOREFONTPROVIDER_H
#define ROCKETCOREFONTPROVIDER_H

#include <Rocket/Core/StringUtilities.h>
#include <Rocket/Core/Font.h>
#include <Rocket/Core/Header.h>

namespace Rocket {
namespace Core {

class FontFaceHandle;
class FontFamily;

/**
    The font database contains all font families currently in use by Rocket.
    @author Peter Curry
 */

class ROCKETCORE_API FontProvider
{
public:

    /// Returns a handle to a font face that can be used to position and render text. This will return the closest match
    /// it can find, but in the event a font family is requested that does not exist, NULL will be returned instead of a
    /// valid handle.
    /// @param[in] family The family of the desired font handle.
    /// @param[in] charset The set of characters required in the font face, as a comma-separated list of unicode ranges.
    /// @param[in] style The style of the desired font handle.
    /// @param[in] weight The weight of the desired font handle.
    /// @param[in] size The size of desired handle, in points.
    /// @return A valid handle if a matching (or closely matching) font face was found, NULL otherwise.
    FontFaceHandle* GetFontFaceHandle(const String& family, const String& charset, Font::Style style, Font::Weight weight, int size);

protected:

    typedef std::map< String, FontFamily*, StringUtilities::StringComparei > FontFamilyMap;
    FontFamilyMap font_families;
};

}
}

#endif
