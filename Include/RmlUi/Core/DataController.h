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

#ifndef RMLUICOREDATACONTROLLER_H
#define RMLUICOREDATACONTROLLER_H

#include "Header.h"
#include "Types.h"
#include "Variant.h"
#include "StringUtilities.h"

namespace Rml {
namespace Core {

class Element;
class DataModel;


class DataControllerAttribute {
public:
	DataControllerAttribute(const DataModel& model, const String& in_attribute_name, const String& in_value_name);

	inline operator bool() const {
		return !attribute_name.empty();
	}
	bool Update(Element* element, const DataModel& model);


	bool OnAttributeChange( const ElementAttributes& changed_attributes)
	{
		bool result = false;
		if (changed_attributes.count(attribute_name) > 0)
		{
			dirty = true;
			result = true;
		}
		return result;
	}

private:
	bool dirty = false;
	String attribute_name;
	String value_name;
};


class DataControllers {
public:

	void AddController(Element* element, DataControllerAttribute&& controller) {
		// TODO: Enable multiple controllers per element
		bool inserted = attribute_controllers.emplace(element, std::move(controller)).second;
		RMLUI_ASSERT(inserted);
	}

	bool Update(const DataModel& model)
	{
		bool result = false;
		for (auto& controller : attribute_controllers)
			result |= controller.second.Update(controller.first, model);
		return result;
	}


	void OnAttributeChange(DataModel& model, Element* element, const ElementAttributes& changed_attributes)
	{
		auto it = attribute_controllers.find(element);
		if (it != attribute_controllers.end())
		{
			it->second.OnAttributeChange(changed_attributes);
		}
	}

private:
	UnorderedMap<Element*, DataControllerAttribute> attribute_controllers;
};


}
}

#endif
