#include "ElementGameInstancer.h"
#include "ElementGame.h"

ElementGameInstancer::~ElementGameInstancer()
{
}
	
// Instances an element given the tag name and attributes
Rocket::Core::Element* ElementGameInstancer::InstanceElement(Rocket::Core::Element* /*parent*/, const Rocket::Core::String& tag, const Rocket::Core::XMLAttributes& /*attributes*/)
{
	return new ElementGame(tag);
}



// Releases the given element
void ElementGameInstancer::ReleaseElement(Rocket::Core::Element* element)
{
	delete element;
}



// Release the instancer
void ElementGameInstancer::Release()
{
	delete this;
}
