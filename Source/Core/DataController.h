#pragma once

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
