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

	auto pos = name.find('[');
	if (pos != String::npos)
	{
		auto pos_end = name.find(']', pos + 2);
		if(pos_end != String::npos)
		{
			const String container_name = name.substr(0, pos);
			const int index = FromString(name.substr(pos + 1, pos_end - pos + 1), -1);
			if (index >= 0)
			{
				auto it_container = containers.find(container_name);
				if (it_container != containers.end())
				{
					DataBindingContext context({ DataBindingContext::Item{ "", index} });

					if (it_container->second->Get(out_value, context))
						return true;
				}
			}
		}
		return false;
	}

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

String DataModel::ResolveVariableName(const String& raw_name, Element* parent) const
{
	auto it = bindings.find(raw_name);
	if (it != bindings.end())
		return raw_name;

	Element* ancestor = parent;
	while (ancestor && ancestor->GetDataModel() == this)
	{
		auto it_element = aliases.find(ancestor);
		if (it_element != aliases.end())
		{
			auto& alias_names = it_element->second;
			auto it_alias_name = alias_names.find(raw_name);
			if (it_alias_name != alias_names.end())
			{
				return it_alias_name->second;
			}
		}

		ancestor = ancestor->GetParentNode();
	}

	Log::Message(Log::LT_WARNING, "Could not find variable name '%s' in data model.", raw_name.c_str());

	return String();
}

}
}
