#include "ResolvedPropertiesDictionary.h"
#include "ElementDefinition.h"

namespace Rml {

ResolvedPropertiesDictionary::ResolvedPropertiesDictionary() : mutable_source(true) {}

ResolvedPropertiesDictionary::ResolvedPropertiesDictionary(const ElementDefinition* source) : mutable_source(false)
{
	auto const& props = source->GetProperties();
	for (auto const& it : props.GetProperties())
		SetProperty(it.first, it.second);

	for (auto const& it : props.GetVariables())
		SetVariable(it.first, it.second);
}

const Property* ResolvedPropertiesDictionary::GetProperty(PropertyId id)
{
	return resolved_properties.GetProperty(id);
}

const Property* ResolvedPropertiesDictionary::GetVariable(VariableId id)
{
	return resolved_properties.GetVariable(id);
}

void ResolvedPropertiesDictionary::SetProperty(PropertyId id, const Property& value)
{
	if (mutable_source)
		source_properties.SetProperty(id, value);

	UpdatePropertyDependencies(id);
	if (value.unit == Property::VARIABLETERM)
	{
		// try to resolve value
		String string_value;
		auto term = value.Get<VariableTerm>();
		ResolveVariableTerm(string_value, term);

		auto definition = StyleSheetSpecification::GetProperty(id);
		if (definition)
		{
			Property parsed_value;
			if (definition->ParseValue(parsed_value, string_value))
				resolved_properties.SetProperty(id, value);
		}
	}
	else
	{
		resolved_properties.SetProperty(id, value);
	}
}

void ResolvedPropertiesDictionary::SetVariable(VariableId id, const Property& value)
{
	if (mutable_source)
		source_properties.SetVariable(id, value);

	// Resolve value
	UpdateVariableDependencies(id);

	if (value.unit == Property::VARIABLETERM)
	{
		String string_value;
		auto term = value.Get<VariableTerm>();
		ResolveVariableTerm(string_value, term);
		resolved_properties.SetVariable(id, Property(string_value, Property::STRING));
	}
	else
	{
		resolved_properties.SetVariable(id, value);
	}
}

void ResolvedPropertiesDictionary::RemoveProperty(PropertyId id)
{
	resolved_properties.RemoveProperty(id);
	if (mutable_source)
		source_properties.RemoveProperty(id);

	UpdatePropertyDependencies(id);
}

void ResolvedPropertiesDictionary::RemoveVariable(VariableId id)
{
	resolved_properties.RemoveVariable(id);
	if (mutable_source)
		source_properties.RemoveVariable(id);

	UpdateVariableDependencies(id);
}

const PropertyDictionary& ResolvedPropertiesDictionary::GetProperties() const
{
	return resolved_properties;
}

void ResolvedPropertiesDictionary::ResolveVariableTerm(String& result, const VariableTerm& term)
{
	StringList atoms;
	for (auto const& atom : term)
	{
		if (atom.variable != static_cast<VariableId>(0))
		{
			auto var = resolved_properties.GetVariable(atom.variable);
			if (var)
				atoms.push_back(var->ToString());
			else
				atoms.push_back(atom.constant);
		}
		else
			atoms.push_back(atom.constant);
	}

	StringUtilities::JoinString(result, atoms, ' ');
}

void ResolvedPropertiesDictionary::UpdatePropertyDependencies(PropertyId id)
{
	for (auto iter = property_dependencies.begin(); iter != property_dependencies.end();)
	{
		if (iter->second == id)
			iter = property_dependencies.erase(iter);
		else
			++iter;
	}

	auto property = source_properties.GetProperty(id);

	if (property && property->unit == Property::VARIABLETERM)
	{
		auto term = property->Get<VariableTerm>();
		for (auto const& atom : term)
		{
			if (atom.variable != static_cast<VariableId>(0))
				property_dependencies.insert(std::make_pair(atom.variable, id));
		}
	}
}

void ResolvedPropertiesDictionary::UpdateVariableDependencies(VariableId id)
{
	for (auto iter = variable_dependencies.begin(); iter != variable_dependencies.end();)
	{
		if (iter->second == id)
			iter = variable_dependencies.erase(iter);
		else
			++iter;
	}

	auto variable = source_properties.GetVariable(id);

	if (variable && variable->unit == Property::VARIABLETERM)
	{
		auto term = variable->Get<VariableTerm>();
		for (auto const& atom : term)
		{
			if (atom.variable != static_cast<VariableId>(0))
				variable_dependencies.insert(std::make_pair(atom.variable, id));
		}
	}

	if (!variable)
	{
		// Variable removed, remove resolved dependent values

		auto dependent_properties = property_dependencies.equal_range(id);
		for (auto iter = dependent_properties.first; iter != dependent_properties.second;)
		{
			resolved_properties.RemoveProperty(iter->second);
			iter = property_dependencies.erase(iter);
		}

		auto dependent_variables = variable_dependencies.equal_range(id);
		for (auto iter = dependent_variables.first; iter != dependent_variables.second;)
		{
			resolved_properties.RemoveVariable(iter->second);
			iter = variable_dependencies.erase(iter);
		}
	}
}

} // namespace Rml
