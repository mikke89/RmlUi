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
#include "DataVariable.h"

namespace Rml {
namespace Core {

class Element;
class DataModel;

class RMLUICORE_API DataController {
public:
	bool UpdateVariable(DataModel& model);

    String GetVariableName() const {
        return address.empty() ? String() : address.front().name;
    }

    Element* GetElement() const {
        return attached_element.get();
    }

	explicit operator bool() const {
		return !address.empty() && attached_element;
	}

    virtual ~DataController();

protected:
	DataController(Element* element);

	void SetAddress(Address new_address) {
		address = std::move(new_address);
	}

	// Return true if value changed
	virtual bool UpdateValue(Element* element, Variant& value_inout) = 0;

private:
	ObserverPtr<Element> attached_element;
	Address address;
	Variant value;
};


class DataControllerValue final : public DataController {
public:
	DataControllerValue(DataModel& model, Element* element, const String& in_value_name);

private:
	bool UpdateValue(Element* element, Variant& value_inout) override;
};


class RMLUICORE_API DataControllers : NonCopyMoveable {
public:
	void Add(UniquePtr<DataController> controller);

    void DirtyElement(DataModel& model, Element* element);

private:
	UnorderedMap<Element*, UniquePtr<DataController>> controllers;
};


}
}

#endif
