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

#include "../precompiled.h"
#include "FontFaceLayer.h"
#include "FontFaceHandle.h"

namespace Rocket {
namespace Core {
namespace BitmapFont {

FontFaceLayer::FontFaceLayer() : Rocket::Core::FontFaceLayer()
{
    handle = NULL;
    effect = NULL;
}

FontFaceLayer::~FontFaceLayer()
{
    if (effect != NULL)
        effect->RemoveReference();
}

// Generates the character and texture data for the layer.
bool FontFaceLayer::Initialise(const Rocket::Core::FontFaceHandle* _handle, FontEffect* _effect, const Rocket::Core::FontFaceLayer* clone, bool deep_clone)
{
    Rocket::Core::BitmapFont::FontFaceHandle
        * bm_font_face_handle;

    handle = _handle;
    //effect = _effect;

    bm_font_face_handle = ( Rocket::Core::BitmapFont::FontFaceHandle * ) handle;

    if (effect != NULL)
    {
        //effect->AddReference();
        //Log::Message( Log::LT_WARNING, "Effects are not supported" );
    }

    const FontGlyphList& glyphs = handle->GetGlyphs();

    // Clone the geometry and textures from the clone layer.
    if (clone != NULL)
    {
        // Copy the cloned layer's characters.
        characters = clone->characters;

        // Copy (and reference) the cloned layer's textures.
        for (size_t i = 0; i < clone->textures.size(); ++i)
            textures.push_back(clone->textures[i]);
    }
    else
    {
        // Initialise the texture layout for the glyphs.
        for (FontGlyphList::const_iterator i = glyphs.begin(); i != glyphs.end(); ++i)
        {
            const FontGlyph& glyph = *i;

            Vector2i glyph_origin( glyph.bitmap_dimensions.x, glyph.bitmap_dimensions.y ); // position in texture
            Vector2i glyph_dimension = glyph.dimensions; // size of char

            Character character;
            character.origin = Vector2f((float) (glyph.bearing.x), (float) (glyph.bearing.y));
            character.dimensions = Vector2f((float) glyph.dimensions.x, (float) glyph.dimensions.y);

            // Set the character's texture index.
            character.texture_index = 0;

            // Generate the character's texture coordinates.
            character.texcoords[0].x = float(glyph_origin.x) / float(bm_font_face_handle->GetTextureSize());
            character.texcoords[0].y = float(glyph_origin.y) / float(bm_font_face_handle->GetTextureSize());
            character.texcoords[1].x = float(glyph_origin.x + character.dimensions.x) / float(bm_font_face_handle->GetTextureSize());
            character.texcoords[1].y = float(glyph_origin.y + character.dimensions.y) / float(bm_font_face_handle->GetTextureSize());

            characters[glyph.character] = character;

        }

        Texture texture;
        if (!texture.Load( bm_font_face_handle->GetTextureBaseName() + "_0.tga", bm_font_face_handle->GetTextureDirectory() ) )
            return false;
        textures.push_back(texture);
    }


    return true;
}

// Generates the texture data for a layer (for the texture database).
bool FontFaceLayer::GenerateTexture(const byte*& texture_data, Vector2i& texture_dimensions, int texture_id)
{
    return true;
}

}
}
}
