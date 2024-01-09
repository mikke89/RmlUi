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

#include "QjsDocument.h"
#include <RmlUi/Core/Stream.h>
#include <RmlUi/Core/FileInterface.h>

#include <RmlUi/QuickJS/Qjs.h>
#include "QjsPlugin.h"
//#include <RmlUi/Lua/Interpreter.h>

namespace Rml {
namespace Qjs {

QjsDocument::QjsDocument(const String& tag) : ElementDocument(tag) {

}

void QjsDocument::LoadInlineScript(const String& context, const String& source_path, int source_line)
{
	//String buffer;
	//buffer += "--";
	//buffer += source_path;
	//buffer += ":";
	//buffer += Rml::ToString(source_line);
	//buffer += "\n";
	//buffer += context;
	//Interpreter::DoString(buffer, buffer);

	JSContext* ctx = QjsPlugin::GetQjsState();
	JS_Eval( ctx, context.c_str(), context.length(), source_path.c_str(), JS_EVAL_TYPE_GLOBAL|JS_EVAL_FLAG_STRICT );
}

void QjsDocument::LoadExternalScript(const String& source_path)
{
	FileInterface* file_interface = GetFileInterface();
	FileHandle handle = file_interface->Open(source_path.c_str() );
	if (handle == 0)
	{
		Log::Message(Log::LT_WARNING, "LoadFile: Unable to open file: %s", source_path.c_str());
		return;
	}

	size_t size = file_interface->Length(handle);
	if (size == 0)
	{
		Log::Message(Log::LT_WARNING, "LoadFile: File is 0 bytes in size: %s", source_path.c_str());
		return;
	}

	UniquePtr<char[]> file_contents(new char[size]);
	file_interface->Read(file_contents.get(), size, handle);
	file_interface->Close(handle);

	JSContext* ctx = QjsPlugin::GetQjsState();
	JS_Eval( ctx, file_contents.get(), size, source_path.c_str(), JS_EVAL_TYPE_GLOBAL|JS_EVAL_FLAG_STRICT );
}

} // namespace Qjs
} // namespace Rml
