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

#ifndef RMLUI_CORE_DATACONTROLLER_H
#define RMLUI_CORE_DATACONTROLLER_H

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Traits.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;
class DataModel;

class DataControllerInstancer : public NonCopyMoveable {
public:
	DataControllerInstancer() {}
	virtual ~DataControllerInstancer() {}
	virtual DataControllerPtr InstanceController(Element* element) = 0;
};

template <typename T>
class DataControllerInstancerDefault final : public DataControllerInstancer {
public:
	DataControllerPtr InstanceController(Element* element) override { return DataControllerPtr(new T(element)); }
};

/**
    Data controller.

    Data controllers are used to respond to some change in the document,
    usually by setting data variables. Such document changes are usually
    a result of user input.
    A data controller is declared in the document by the element attribute:

        data-[type]-[modifier]="[assignment_expression]"

    This is similar to declaration of data views, except that controllers
    instead take an assignment expression to set a variable. Note that, as
    opposed to views, controllers only respond to certain changes in the
    document, not to changed data variables.

    The modifier may or may not be required depending on the data controller.

 */

class DataController : public Releasable {
public:
	virtual ~DataController();

	// Initialize the data controller.
	// @param[in] model The data model the controller will be attached to.
	// @param[in] element The element which spawned the controller.
	// @param[in] expression The value of the element's 'data-' attribute which spawned the controller (see above).
	// @param[in] modifier The modifier for the given controller type (see above).
	// @return True on success.
	virtual bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) = 0;

	// Returns the attached element if it still exists.
	Element* GetElement() const;

	// Returns true if the element still exists.
	bool IsValid() const;

protected:
	DataController(Element* element);

private:
	ObserverPtr<Element> attached_element;
};

class DataControllers : NonCopyMoveable {
public:
	DataControllers();
	~DataControllers();

	void Add(DataControllerPtr controller);

	void OnElementRemove(Element* element);

private:
	using ElementControllersMap = UnorderedMultimap<Element*, DataControllerPtr>;
	ElementControllersMap controllers;
};

} // namespace Rml
#endif
