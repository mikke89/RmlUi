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
#include "../../Include/RmlUi/Core/DataController.h"
#include "../../Include/RmlUi/Core/DataModel.h"

namespace Rml {
namespace Core {


DataControllerAttribute::DataControllerAttribute(DataModel& model, Element* parent, const String& in_attribute_name, const String& in_value_name) : attribute_name(in_attribute_name)
{
    variable_address = model.ResolveAddress(in_value_name, parent);
    if (!model.GetVariable(variable_address))
	{
		attribute_name.clear();
        variable_address.clear();
	}
}

bool DataControllerAttribute::Update(Element* element, DataModel& model) 
{
	bool result = false;
	if (dirty)
	{
		if(Variant* value = element->GetAttribute(attribute_name))
        {
            if (Variable variable = model.GetVariable(variable_address))
            {
                result = variable.Set(*value);
                model.DirtyVariable(variable_address.front().name);
            }
        }
		dirty = false;
	}
	return result;
}



}
}
