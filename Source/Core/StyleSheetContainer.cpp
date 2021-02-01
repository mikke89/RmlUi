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

#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/StyleSheetContainer.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "StyleSheetParser.h"
#include "Utilities.h"

namespace Rml {

StyleSheetContainer::StyleSheetContainer()
{
}

StyleSheetContainer::~StyleSheetContainer()
{
}

bool StyleSheetContainer::LoadStyleSheetContainer(Stream* stream, int begin_line_number)
{
	StyleSheetParser parser;
	int rule_count = parser.Parse(media_blocks, stream, begin_line_number);
	return rule_count >= 0;
}

StyleSheet* StyleSheetContainer::GetCompiledStyleSheet(Vector2i dimensions, float density_ratio)
{
    if(compiled_style_sheet && dimensions == current_dimensions && density_ratio == current_density_ratio)
        return compiled_style_sheet.get();

    UniquePtr<StyleSheet> new_sheet = MakeUnique<StyleSheet>();

    for(auto const& media_block : media_blocks)
    {
        bool all_match = true;
        for(auto const& property : media_block.properties.GetProperties())
        {
            switch(static_cast<MediaQueryId>(property.first)) 
            {
            case MediaQueryId::Width:
                if(dimensions.x != property.second.Get<int>())
                    all_match = false;
                break;
            case MediaQueryId::MinWidth:
                if(dimensions.x < property.second.Get<int>())
                    all_match = false;
                break;
            case MediaQueryId::MaxWidth:
                if(dimensions.x > property.second.Get<int>())
                    all_match = false;
                break;
            case MediaQueryId::Height:
                if(dimensions.y != property.second.Get<int>())
                    all_match = false;
                break;
            case MediaQueryId::MinHeight:
                if(dimensions.y < property.second.Get<int>())
                    all_match = false;
                break;
            case MediaQueryId::MaxHeight:
                if(dimensions.y > property.second.Get<int>())
                    all_match = false;
                break;
            case MediaQueryId::AspectRatio:
                if(((float)dimensions.x / (float)dimensions.y) != property.second.Get<float>())
                    all_match = false;
                break;
            case MediaQueryId::MinAspectRatio:
                if(((float)dimensions.x / (float)dimensions.y) < property.second.Get<float>())
                    all_match = false;
                break;
            case MediaQueryId::MaxAspectRatio:
                if(((float)dimensions.x / (float)dimensions.y) > property.second.Get<float>())
                    all_match = false;
                break;
            case MediaQueryId::Resolution:
                if(density_ratio != property.second.Get<float>())
                    all_match = false;
                break;
            case MediaQueryId::MinResolution:
                if(density_ratio < property.second.Get<float>())
                    all_match = false;
                break;
            case MediaQueryId::MaxResolution:
                if(density_ratio > property.second.Get<float>())
                    all_match = false;
                break;
            case MediaQueryId::Orientation:
                // Landscape (x > y) = 0 
                // Portrait (x <= y) = 1
                if((dimensions.x <= dimensions.y) != property.second.Get<bool>())
                    all_match = false;
                break;  
            // Invalid properties
            case MediaQueryId::Invalid:
            case MediaQueryId::NumDefinedIds:
            case MediaQueryId::MaxNumIds:
                break;
            }
            
            if(!all_match)
                break;
        }

        if(all_match)
        {
            new_sheet = new_sheet->CombineStyleSheet(*media_block.stylesheet);
        }
    }
    
    new_sheet->BuildNodeIndex();
    new_sheet->OptimizeNodeProperties();

    compiled_style_sheet = std::move(new_sheet);
    current_dimensions = dimensions;
    current_density_ratio = density_ratio;
    return compiled_style_sheet.get();
}

/// Combines this style sheet container with another one, producing a new sheet container.
SharedPtr<StyleSheetContainer> StyleSheetContainer::CombineStyleSheetContainer(const StyleSheetContainer& container) const
{
    SharedPtr<StyleSheetContainer> new_sheet = MakeShared<StyleSheetContainer>();

    for(auto const& pair : media_blocks)
    {      
        PropertyDictionary dict;
        dict.Import(pair.properties);
        new_sheet->media_blocks.push_back(MediaBlock(dict, pair.stylesheet->CombineStyleSheet(StyleSheet{})));
    }

    for(auto const& pair : container.media_blocks)
    {      
        bool block_found = false;
        for(auto& media_block : new_sheet->media_blocks)
        {
            if(pair.properties.GetProperties() == media_block.properties.GetProperties())
            {
                media_block.stylesheet = media_block.stylesheet->CombineStyleSheet(*pair.stylesheet);
                block_found = true;
                break;
            }
        }

        if (!block_found)
        {
            PropertyDictionary dict;
            dict.Import(pair.properties);
            new_sheet->media_blocks.push_back(MediaBlock(dict, pair.stylesheet->CombineStyleSheet(StyleSheet{})));
        }
    }

    return new_sheet;
}

} // namespace Rml
