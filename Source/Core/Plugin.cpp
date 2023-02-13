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

#include "../../Include/RmlUi/Core/Plugin.h"

namespace Rml {

Plugin::~Plugin()
{
}

int Plugin::GetEventClasses()
{
	return EVT_ALL;
}

void Plugin::OnInitialise()
{
}

void Plugin::OnShutdown()
{
}

// Called when a new context is created.
void Plugin::OnContextCreate(Context* RMLUI_UNUSED_PARAMETER(context))
{
	RMLUI_UNUSED(context);
}

// Called when a context is destroyed.
void Plugin::OnContextDestroy(Context* RMLUI_UNUSED_PARAMETER(context))
{
	RMLUI_UNUSED(context);
}

// Called when a document load request occurs, before the document's file is opened.
void Plugin::OnDocumentOpen(Context* RMLUI_UNUSED_PARAMETER(context), const String& RMLUI_UNUSED_PARAMETER(document_path))
{
	RMLUI_UNUSED(context);
	RMLUI_UNUSED(document_path);
}

// Called when a document is successfully loaded from file or instanced, initialised and added to
// its context.
void Plugin::OnDocumentLoad(ElementDocument* RMLUI_UNUSED_PARAMETER(document))
{
	RMLUI_UNUSED(document);
}

// Called when a document is unloaded from its context.
void Plugin::OnDocumentUnload(ElementDocument* RMLUI_UNUSED_PARAMETER(document))
{
	RMLUI_UNUSED(document);
}

// Called when a new element is created.
void Plugin::OnElementCreate(Element* RMLUI_UNUSED_PARAMETER(element))
{
	RMLUI_UNUSED(element);
}

// Called when an element is destroyed.
void Plugin::OnElementDestroy(Element* RMLUI_UNUSED_PARAMETER(element))
{
	RMLUI_UNUSED(element);
}

void Plugin::OnPreShutdown()
{
}

} // namespace Rml
