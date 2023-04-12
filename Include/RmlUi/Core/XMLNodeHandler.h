/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#ifndef RMLUI_CORE_XMLNODEHANDLER_H
#define RMLUI_CORE_XMLNODEHANDLER_H

#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class Element;
class XMLParser;
enum class XMLDataType;

/**
    A handler gets ElementStart, ElementEnd and ElementData called by the XMLParser.

    @author Lloyd Weehuizen
 */

class RMLUICORE_API XMLNodeHandler : public NonCopyMoveable {
public:
	virtual ~XMLNodeHandler();

	/// Called when a new element tag is opened.
	/// @param parser The parser executing the parse.
	/// @param name The XML tag name.
	/// @param attributes The tag attributes.
	/// @return The new element, may be nullptr if no element was created.
	virtual Element* ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes) = 0;

	/// Called when an element is closed.
	/// @param parser The parser executing the parse.
	/// @param name The XML tag name.
	virtual bool ElementEnd(XMLParser* parser, const String& name) = 0;

	/// Called for element data.
	/// @param parser The parser executing the parse.
	/// @param data The element data.
	virtual bool ElementData(XMLParser* parser, const String& data, XMLDataType type) = 0;
};

} // namespace Rml
#endif
