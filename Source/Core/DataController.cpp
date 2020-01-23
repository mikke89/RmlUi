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

DataController::DataController(Element* element) : attached_element(element->GetObserverPtr())
{}

bool DataController::UpdateVariable(DataModel& model)
{
    Element* element = attached_element.get();
    if (!element)
        return false;

    if (!UpdateValue(element, value))
        return false;

    bool variable_changed = false;
    if (Variable variable = model.GetVariable(address))
        variable_changed = variable.Set(value);

    return variable_changed;
}

const String& DataController::GetVariableName() const {
    static const String empty_string;
    return address.empty() ? empty_string : address.front().name;
}


DataControllerValue::DataControllerValue(DataModel& model, Element* element, const String& in_value_name) : DataController(element)
{
    Address variable_address = model.ResolveAddress(in_value_name, element);

    if (model.GetVariable(variable_address) && !variable_address.empty())
    {
        SetAddress(std::move(variable_address));
    }
}

bool DataControllerValue::UpdateValue(Element* element, Variant& value_inout)
{
    bool value_changed = false;

    if (Variant* new_value = element->GetAttribute("value"))
    {
        if (*new_value != value_inout)
        {
            value_inout = *new_value;
            value_changed = true;
        }
    }
    
    return value_changed;
}




void DataControllers::Add(UniquePtr<DataController> controller) {
    RMLUI_ASSERT(controller);

    Element* element = controller->GetElement();
    RMLUI_ASSERTMSG(element, "Invalid controller, make sure it is valid before adding");

    bool inserted = controllers.emplace(element, std::move(controller)).second;
    if (!inserted)
    {
        RMLUI_ERRORMSG("Cannot add multiple controllers to the same element.");
    }
}

void DataControllers::DirtyElement(DataModel& model, Element* element)
{
    auto it = controllers.find(element);
    if (it != controllers.end())
    {
        DataController& controller = *it->second;
        if (controller.UpdateVariable(model))
        {
            model.DirtyVariable(controller.GetVariableName());
        }
    }
}

}
}
