#include "../../Include/RmlUi/Core/PropertiesIteratorView.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "PropertiesIterator.h"

namespace Rml {

PropertiesIteratorView::PropertiesIteratorView(UniquePtr<PropertiesIterator> ptr) : ptr(std::move(ptr)) {}

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

PropertyId PropertiesIteratorView::GetId() const
{
	return (*(*ptr)).first;
}

const String& PropertiesIteratorView::GetName() const
{
	return StyleSheetSpecification::GetPropertyName(GetId());
}

const Property& PropertiesIteratorView::GetProperty() const
{
	return (*(*ptr)).second;
}

bool PropertiesIteratorView::AtEnd() const
{
	return ptr->AtEnd();
}

} // namespace Rml
