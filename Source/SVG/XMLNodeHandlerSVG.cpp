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

namespace Rml {
namespace SVG {
	XMLNodeHandlerSVG::XMLNodeHandlerSVG()
	{
		// Initialize rng for generating element ids for non file based svg tags to be used as cache keys
		std::random_device rd;
		rand_gen = std::mt19937(rd());
	};
	XMLNodeHandlerSVG::~XMLNodeHandlerSVG() = default;

	bool XMLNodeHandlerSVG::ElementData(XMLParser* parser, const String& data, XMLDataType type)
	{
		const String& tag = parser->GetParseFrame()->tag;

		// Shouldn't be necessary but just to make sure
		if (tag == "svg" && type == XMLDataType::CDATA)
		{
			auto* parent = rmlui_static_cast<ElementSVG*>(parser->GetParseFrame()->element);
			RMLUI_ASSERT(parent);

			// Try to get the existing text node, or create if its missing (it is inserted as a non DOM child so should be at the start)
			Element* data_element = parent->GetChild(0);
			if (!data_element || data_element->GetTagName() != "#text")
				data_element = parent->AppendChild(Factory::InstanceElement(parent, "#text", "#text", XMLAttributes()), false);

			// Set a unique id for the element, used as a cache key for SVGCache
			data_element->SetAttribute("id",
				"svg_" +
					std::to_string(
						std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) +
					"_" + std::to_string(std::generate_canonical<double, 10>(rand_gen) * 1000000));

			// Set the content of the text node to the SVG data
			rmlui_static_cast<ElementText*>(data_element)->SetText(data);

			// Force the SVG element to re-render
			parent->SetDirtyFlag();
		}

		return true;
	}
} // namespace SVG
} // namespace Rml
