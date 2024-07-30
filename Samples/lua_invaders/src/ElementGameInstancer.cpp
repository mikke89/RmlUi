#include "ElementGameInstancer.h"
#include "ElementGame.h"

ElementGameInstancer::~ElementGameInstancer() {}

Rml::ElementPtr ElementGameInstancer::InstanceElement(Rml::Element* /*parent*/, const Rml::String& tag, const Rml::XMLAttributes& /*attributes*/)
{
	return Rml::ElementPtr(new ElementGame(tag));
}

void ElementGameInstancer::ReleaseElement(Rml::Element* element)
{
	delete element;
}
