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

#include "precompiled.h"
#include "../../Include/RmlUi/Core/DataTypeRegister.h"

namespace Rml {
namespace Core {


DataTypeRegister::DataTypeRegister()
{
    // Add default transform functions.

	transform_register.Register("to_lower", [](Variant& variant, const VariantList& arguments) -> bool {
		String value;
		if (!variant.GetInto(value))
			return false;
		variant = StringUtilities::ToLower(value);
		return true;
	});

	transform_register.Register("to_upper", [](Variant& variant, const VariantList& arguments) -> bool {
		String value;
		if (!variant.GetInto(value))
			return false;
		variant = StringUtilities::ToUpper(value);
		return true;
	});
}

DataTypeRegister::~DataTypeRegister()
{}

void TransformFuncRegister::Register(const String& name, DataTransformFunc transform_func)
{
    RMLUI_ASSERT(transform_func);
    bool inserted = transform_functions.emplace(name, std::move(transform_func)).second;
    if (!inserted)
    {
        Log::Message(Log::LT_ERROR, "Transform function '%s' already exists.", name.c_str());
        RMLUI_ERROR;
    }
}

bool TransformFuncRegister::Call(const String& name, Variant& inout_result, const VariantList& arguments) const
{
    auto it = transform_functions.find(name);
    if (it == transform_functions.end())
        return false;

    const DataTransformFunc& transform_func = it->second;
    RMLUI_ASSERT(transform_func);

    return transform_func(inout_result, arguments);
}

}
}
