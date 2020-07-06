#include "ElementGameInstancer.h"
#include "ElementGame.h"

ElementGameInstancer::~ElementGameInstancer()
{
}
	
// Instances an element given the tag name and attributes
Rml::ElementPtr ElementGameInstancer::InstanceElement(Rml::Element* /*parent*/, const Rml::String& tag, const Rml::XMLAttributes& /*attributes*/)
{
	return Rml::ElementPtr(new ElementGame(tag));
}



// Releases the given element
void ElementGameInstancer::ReleaseElement(Rml::Element* element)
{
	delete element;
}


