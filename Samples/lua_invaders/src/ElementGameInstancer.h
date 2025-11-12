#ifndef RMLUI_LUAINVADERS_ELEMENTGAMEINSTANCER_H
#define RMLUI_LUAINVADERS_ELEMENTGAMEINSTANCER_H

#include <RmlUi/Core/NodeInstancer.h>

class ElementGameInstancer : public Rml::NodeInstancer {
public:
	virtual ~ElementGameInstancer();

	/// Instances an element given the tag name and attributes
	/// @param tag Name of the element to instance
	Rml::NodePtr InstanceNode(const Rml::String& tag) override;

	/// Releases the given element
	/// @param node to release
	void ReleaseNode(Rml::Node* node) override;
};

#endif
