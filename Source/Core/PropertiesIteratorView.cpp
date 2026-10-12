#include "../../Include/RmlUi/Core/PropertiesIteratorView.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "PropertiesIterator.h"

namespace Rml {

PropertiesIteratorView::PropertiesIteratorView(UniquePtr<AllPropertiesIterator> ptr) : ptr(std::move(ptr)) {}

PropertiesIteratorView::PropertiesIteratorView(PropertiesIteratorView&& other) noexcept : ptr(std::move(other.ptr)) {}

PropertiesIteratorView& PropertiesIteratorView::operator=(PropertiesIteratorView&& other) noexcept
{
	ptr = std::move(other.ptr);
	return *this;
}

PropertiesIteratorView::~PropertiesIteratorView() {}

PropertiesIteratorView& PropertiesIteratorView::operator++()
{
	++(*ptr);
	return *this;
}

bool PropertiesIteratorView::AtEnd() const
{
	return ptr->AtEnd();
}

Pair<String, const Property&> PropertiesIteratorView::operator*() const
{
	return *(*ptr);
}

} // namespace Rml
