#include "ElementGameInstancer.h"
#include "ElementGame.h"

ElementGameInstancer::~ElementGameInstancer() {}

Rml::NodePtr ElementGameInstancer::InstanceNode(const Rml::String& tag)
{
	return Rml::NodePtr(new ElementGame(tag));
}

void ElementGameInstancer::ReleaseNode(Rml::Node* node)
{
	delete node;
}
