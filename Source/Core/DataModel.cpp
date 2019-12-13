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
#include "../../Include/RmlUi/Core/DataModel.h"

namespace Rml {
namespace Core {


bool DataModel::GetValue(const String& name, Variant& out_value) const
{
	bool success = true;

	auto it = bindings.find(name);
	if (it != bindings.end())
	{
		DataBinding& binding = *it->second;

		success = binding.Get(out_value);
		if (!success)
			Log::Message(Log::LT_WARNING, "Could not get value from '%s' in data model.", name.c_str());
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Could not find value named '%s' in data model.", name.c_str());
		success = false;
	}

	return success;
}


bool DataModel::SetValue(const String& name, const Variant& value) const
{
	bool success = true;

	auto it = bindings.find(name);
	if (it != bindings.end())
	{
		DataBinding& binding = *it->second;

		success = binding.Set(value);
		if (!success)
			Log::Message(Log::LT_WARNING, "Could not set value to '%s' in data model.", name.c_str());
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Could not find value named '%s' in data model.", name.c_str());
		success = false;
	}

	return success;
}

}
}
