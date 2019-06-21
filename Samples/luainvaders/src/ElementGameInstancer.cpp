#include "ElementGameInstancer.h"
#include "ElementGame.h"

ElementGameInstancer::~ElementGameInstancer()
{
}
	
// Instances an element given the tag name and attributes
Rml::Core::Element* ElementGameInstancer::InstanceElement(Rml::Core::Element* /*parent*/, const Rml::Core::String& tag, const Rml::Core::XMLAttributes& /*attributes*/)
{
	return new ElementGame(tag);
}



// Releases the given element
void ElementGameInstancer::ReleaseElement(Rml::Core::Element* element)
{
	delete element;
}



// Release the instancer
void ElementGameInstancer::Release()
{
	delete this;
}
