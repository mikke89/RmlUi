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

#include "DataController.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "EventSpecification.h"

namespace Rml {

DataController::DataController(Element* element) : attached_element(element->GetObserverPtr()) {}

DataController::~DataController() {}
Element* DataController::GetElement() const
{
	return attached_element.get();
}

bool DataController::IsValid() const
{
	return static_cast<bool>(attached_element);
}

DataControllers::DataControllers() {}

DataControllers::~DataControllers() {}

void DataControllers::Add(DataControllerPtr controller)
{
	RMLUI_ASSERT(controller);

	Element* element = controller->GetElement();
	RMLUI_ASSERTMSG(element, "Invalid controller, make sure it is valid before adding");
	if (!element)
		return;

	controllers.emplace(element, std::move(controller));
}

void DataControllers::OnElementRemove(Element* element)
{
	controllers.erase(element);
}

} // namespace Rml
