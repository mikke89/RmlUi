#pragma once

#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "DocumentHeader.h"

namespace Rml {

class Element;

/**
    Contains a RML template. The Header is stored in parsed form, body in an unparsed stream.
 */

class Template {
public:
	Template();
	~Template();

	/// Load a template from the given stream
	bool Load(Stream* stream);

	/// Get the ID of the template
	const String& GetName() const;

	/// Parse the template into the given element
	/// @param element Element to parse into
	/// @returns The element to continue the parse from
	Element* ParseTemplate(Element* element);

	/// Get the template header
	const DocumentHeader* GetHeader();

private:
	String name;
	String content;
	DocumentHeader header;
	UniquePtr<StreamMemory> body;
};

} // namespace Rml
