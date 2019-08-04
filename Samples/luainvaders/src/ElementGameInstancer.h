#ifndef ELEMENTGAMEINSTANCER_H
#define ELEMENTGAMEINSTANCER_H
#include <RmlUi/Core/ElementInstancer.h>

class ElementGameInstancer : public Rml::Core::ElementInstancer
{
public:
	virtual ~ElementGameInstancer();
	
	/// Instances an element given the tag name and attributes
	/// @param tag Name of the element to instance
	/// @param attributes vector of name value pairs
    Rml::Core::ElementPtr InstanceElement(Rml::Core::Element* parent, const Rml::Core::String& tag, const Rml::Core::XMLAttributes& attributes) override;

	/// Releases the given element
	/// @param element to release
	void ReleaseElement(Rml::Core::Element* element) override;
};

#endif
