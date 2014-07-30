#ifndef ELEMENTGAMEINSTANCER_H
#define ELEMENTGAMEINSTANCER_H
#include <Rocket/Core/ElementInstancer.h>

class ElementGameInstancer : public Rocket::Core::ElementInstancer
{
public:
	virtual ~ElementGameInstancer();
	
	/// Instances an element given the tag name and attributes
	/// @param tag Name of the element to instance
	/// @param attributes vector of name value pairs
    virtual Rocket::Core::Element* InstanceElement(Rocket::Core::Element* parent, const Rocket::Core::String& tag, const Rocket::Core::XMLAttributes& attributes);

	/// Releases the given element
	/// @param element to release
	virtual void ReleaseElement(Rocket::Core::Element* element);

	/// Release the instancer
	virtual void Release();
};

#endif
