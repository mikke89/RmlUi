#pragma once

#include <RmlUi/Core/ElementInstancer.h>

class ElementGameInstancer : public Rml::ElementInstancer {
public:
	virtual ~ElementGameInstancer();

	/// Instances an element given the tag name and attributes
	/// @param tag Name of the element to instance
	/// @param attributes vector of name value pairs
	Rml::ElementPtr InstanceElement(Rml::Element* parent, const Rml::String& tag, const Rml::XMLAttributes& attributes) override;

	/// Releases the given element
	/// @param element to release
	void ReleaseElement(Rml::Element* element) override;
};
