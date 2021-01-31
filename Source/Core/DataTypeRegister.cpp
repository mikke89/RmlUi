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

#include "../../Include/RmlUi/Core/DataTypeRegister.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/Variant.h"

namespace Rml {

DataTypeRegister::DataTypeRegister()
{
    // Add default transform functions.

	transform_register.Register("to_lower", [](Variant& variant, const VariantList& /*arguments*/) -> bool {
		String value;
		if (!variant.GetInto(value))
			return false;
		variant = StringUtilities::ToLower(value);
		return true;
	});

	transform_register.Register("to_upper", [](Variant& variant, const VariantList& /*arguments*/) -> bool {
		String value;
		if (!variant.GetInto(value))
			return false;
		variant = StringUtilities::ToUpper(value);
		return true;
	});

	transform_register.Register("format", [](Variant& variant, const VariantList& arguments) -> bool {
        // Arguments in:
        //   0 : int[0,32]  Precision. Number of digits after the decimal point.
        //  [1]: bool       True to remove trailing zeros (default = false).
        if (arguments.size() < 1 || arguments.size() > 2) {
            Log::Message(Log::LT_WARNING, "Transform function 'format' requires at least one argument, at most two arguments.");
            return false;
        }
        int precision = 0;
        if (!arguments[0].GetInto(precision) || precision < 0 || precision > 32) {
            Log::Message(Log::LT_WARNING, "Transform function 'format': First argument must be an integer in [0, 32].");
            return false;
        }
        bool remove_trailing_zeros = false;
        if (arguments.size() >= 2) {
            if (!arguments[1].GetInto(remove_trailing_zeros))
                return false;
        }

		double value = 0;
		if (!variant.GetInto(value))
			return false;

        String format_specifier = String(remove_trailing_zeros ? "%#." : "%.") + ToString(precision) + 'f';
        String result;
        if (FormatString(result, 64, format_specifier.c_str(), value) == 0)
            return false;

        if (remove_trailing_zeros)
            StringUtilities::TrimTrailingDotZeros(result);

        variant = result;
		return true;
	});

	transform_register.Register("round", [](Variant& variant, const VariantList& /*arguments*/) -> bool {
		double value = 0;
		if (!variant.GetInto(value))
			return false;
        variant = Math::RoundFloat(value);
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

} // namespace Rml
