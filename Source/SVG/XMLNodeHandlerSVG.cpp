/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019- The RmlUi Team, and contributors
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

#include "XMLNodeHandlerSVG.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "../../Include/RmlUi/SVG/ElementSVG.h"

namespace Rml {
namespace SVG {
	XMLNodeHandlerSVG::XMLNodeHandlerSVG() = default;
	XMLNodeHandlerSVG::~XMLNodeHandlerSVG() = default;

	bool XMLNodeHandlerSVG::ElementData(XMLParser* parser, const String& data, XMLDataType type)
	{
		const String& tag = parser->GetParseFrame()->tag;

		// Store the title
		if (tag == "svg" && type == XMLDataType::CData)
		{
			// Determine the parent
			Element* parent = parser->GetParseFrame()->element;
			RMLUI_ASSERT(parent);

			// For CDATA tags make the CDATA available through the _cdata attribute
			parent->SetAttribute("_cdata", data);
		}

		return true;
	}
} // namespace SVG
} // namespace Rml
