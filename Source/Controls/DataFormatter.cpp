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

#include "../../Include/RmlUi/Controls/DataFormatter.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"

namespace Rml {
namespace Controls {

typedef Core::UnorderedMap< Rml::Core::String, DataFormatter* > DataFormatterMap;
static DataFormatterMap data_formatters;

DataFormatter::DataFormatter(const Rml::Core::String& _name)
{
	if (!_name.empty())
	{
		name = _name;
	}
	else
	{
		name = Core::CreateString(64, "%x", this);
	}
	data_formatters[name] = this;
}

DataFormatter::~DataFormatter()
{
}

const Rml::Core::String& DataFormatter::GetDataFormatterName()
{
	return name;
}

DataFormatter* DataFormatter::GetDataFormatter(const Rml::Core::String& data_formatter_name)
{
	DataFormatterMap::iterator i = data_formatters.find(data_formatter_name);
	if (i == data_formatters.end())
	{
		return nullptr;
	}

	return (*i).second;
}

void* DataFormatter::GetScriptObject() const
{
	return nullptr;
}

}
}
