namespace Rml {

template <typename T>
T Element::GetProperty(const String& name)
{
	const Property* property = GetProperty(name);
	if (!property)
	{
		Log::Message(Log::LT_WARNING, "Invalid property name %s.", name.c_str());
		return T{};
	}
	return property->Get<T>();
}

template <typename T>
void Element::SetAttribute(const String& name, const T& value)
{
	Variant variant(value);
	attributes[name] = variant;
	ElementAttributes changed_attributes;
	changed_attributes.emplace(name, std::move(variant));
	OnAttributeChange(changed_attributes);
}

template <typename T>
T Element::GetAttribute(const String& name, const T& default_value) const
{
	return Get(attributes, name, default_value);
}

} // namespace Rml
