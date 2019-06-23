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
    virtual Rml::Core::Element* InstanceElement(Rml::Core::Element* parent, const Rml::Core::String& tag, const Rml::Core::XMLAttributes& attributes);

	/// Releases the given element
	/// @param element to release
	virtual void ReleaseElement(Rml::Core::Element* element);

	/// Release the instancer
	virtual void Release();
};

#endif
