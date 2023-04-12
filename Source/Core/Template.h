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

#ifndef RMLUI_CORE_TEMPLATE_H
#define RMLUI_CORE_TEMPLATE_H

#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "DocumentHeader.h"

namespace Rml {

class Element;

/**
    Contains a RML template. The Header is stored in parsed form, body in an unparsed stream.

    @author Lloyd Weehuizen
 */

class Template {
public:
	Template();
	~Template();

	/// Load a template from the given stream
	bool Load(Stream* stream);

	/// Get the ID of the template
	const String& GetName() const;

	/// Parse the template into the given element
	/// @param element Element to parse into
	/// @returns The element to continue the parse from
	Element* ParseTemplate(Element* element);

	/// Get the template header
	const DocumentHeader* GetHeader();

private:
	String name;
	String content;
	DocumentHeader header;
	UniquePtr<StreamMemory> body;
};

} // namespace Rml
#endif
