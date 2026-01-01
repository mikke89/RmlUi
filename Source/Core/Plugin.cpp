#include "../../Include/RmlUi/Core/Plugin.h"

namespace Rml {

Plugin::~Plugin() {}

int Plugin::GetEventClasses()
{
	return EVT_ALL;
}

void Plugin::OnInitialise() {}

void Plugin::OnShutdown() {}

void Plugin::OnContextCreate(Context* /*context*/) {}

void Plugin::OnContextDestroy(Context* /*context*/) {}

void Plugin::OnDocumentOpen(Context* /*context*/, const String& /*document_path*/) {}

void Plugin::OnDocumentLoad(ElementDocument* /*document*/) {}

void Plugin::OnDocumentUnload(ElementDocument* /*document*/) {}

void Plugin::OnElementCreate(Element* /*element*/) {}

void Plugin::OnElementDestroy(Element* /*element*/) {}

} // namespace Rml
