#include "../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../Include/RmlUi/Core/PropertyDefinition.h"

#include "PropertyShorthandDefinition.h"
#include "ResolvedPropertiesDictionary.h"
#include "ElementDefinition.h"
#include "ElementStyle.h"

namespace Rml {

ResolvedPropertiesDictionary::ResolvedPropertiesDictionary(ElementStyle* parent) : parent(parent) {}

ResolvedPropertiesDictionary::ResolvedPropertiesDictionary(ElementStyle* parent, const ElementDefinition* source) :
	parent(parent)
{
	auto const& props = source->GetProperties();

	if (source) 
	{
		for (auto const& it : props.GetProperties())
			SetProperty(it.first, it.second);

		for (auto const& it : props.GetVariables())
			SetVariable(it.first, it.second);
		
		for (auto const& it : props.GetDependentShorthands())
			SetDependentShorthand(it.first, it.second);
	}
}

const Property* ResolvedPropertiesDictionary::GetProperty(PropertyId id) const
{
	return resolved_properties.GetProperty(id);
}

const Property* ResolvedPropertiesDictionary::GetVariable(VariableId id) const
{
	return resolved_properties.GetVariable(id);
}

void ResolvedPropertiesDictionary::SetProperty(PropertyId id, const Property& value)
{
	source_properties.SetProperty(id, value);
	
	ResolveProperty(id);
	
	UpdatePropertyDependencies(id);
}

void ResolvedPropertiesDictionary::SetVariable(VariableId id, const Property& value)
{
	source_properties.SetVariable(id, value);
	
	ResolveVariable(id);
	
	UpdateVariableDependencies(id, true);
}

void ResolvedPropertiesDictionary::SetDependentShorthand(ShorthandId id, const VariableTerm &value)
{
	source_properties.SetDependent(id, value);
	
	ResolveShorthand(id);
	
	UpdateShorthandDependencies(id);
}

bool ResolvedPropertiesDictionary::RemoveProperty(PropertyId id)
{
	auto size_before = resolved_properties.GetNumProperties();

	resolved_properties.RemoveProperty(id);
	source_properties.RemoveProperty(id);

	UpdatePropertyDependencies(id);

	return resolved_properties.GetNumProperties() != size_before;
}

bool ResolvedPropertiesDictionary::RemoveVariable(VariableId id)
{
	auto size_before = resolved_properties.GetNumVariables();

	resolved_properties.RemoveVariable(id);
	source_properties.RemoveVariable(id);

	UpdateVariableDependencies(id, true);

	return resolved_properties.GetNumVariables() != size_before;
}

const PropertyDictionary& ResolvedPropertiesDictionary::GetProperties() const
{
	return resolved_properties;
}

void ResolvedPropertiesDictionary::ApplyDirtyVariables()
{
	if (!parent)
		return;
	
	for(auto const& id : parent->GetDirtyVariables())
	{
		UpdateVariableDependencies(id, false);
	}
}

void ResolvedPropertiesDictionary::ResolveVariableTerm(String& result, const VariableTerm& term)
{
	StringList atoms;
	for (auto const& atom : term)
	{
		if (atom.variable != static_cast<VariableId>(0))
		{
			const Property* var = nullptr;
			
			if (parent)
				var = parent->GetVariable(atom.variable);
			else
				var = resolved_properties.GetVariable(atom.variable);
			
			if (var)
				atoms.push_back(var->ToString());
			else
			{
				if (atom.constant.empty())
					Log::Message(Log::LT_ERROR, "Failed to resolve RCSS variable '%s'. No fallback was provided.", GetVariableName(atom.variable).c_str());
					
				atoms.push_back(atom.constant);
			}
		}
		else
			atoms.push_back(atom.constant);
	}
	
	StringUtilities::JoinString(result, atoms, '\0');
}

void ResolvedPropertiesDictionary::ResolveProperty(PropertyId id)
{
	auto value = source_properties.GetProperty(id);
	if (value)
	{
		if (value->unit == Property::VARIABLETERM)
		{
			// try to resolve value
			String string_value;
			ResolveVariableTerm(string_value, value->value.GetReference<VariableTerm>());
			
			auto definition = StyleSheetSpecification::GetProperty(id);
			if (definition)
			{
				Property parsed_value;
				if (definition->ParseValue(parsed_value, string_value))
					resolved_properties.SetProperty(id, parsed_value);
				else
					Log::Message(Log::LT_ERROR, "Failed to parse RCSS variable-dependent property '%s' with value '%s'.",
						StyleSheetSpecification::GetPropertyName(id).c_str(), string_value.c_str());
			}
		}
		else if (value->unit != Property::UNKNOWN)
		{
			// Ignore dependent-shorthand pending values
			resolved_properties.SetProperty(id, *value);
		}
	}
	else
	{
		resolved_properties.RemoveProperty(id);
	}
	
	if (parent)
		parent->DirtyProperty(id);
}

void ResolvedPropertiesDictionary::ResolveShorthand(ShorthandId id)
{
	auto const& shorthands = source_properties.GetDependentShorthands();
	auto var = shorthands.find(id);
	if (var != shorthands.end())
	{
		String string_value;
		ResolveVariableTerm(string_value, var->second);
		
		// clear underlying properties
		auto properties = StyleSheetSpecification::GetShorthandUnderlyingProperties(id);
		for (auto const& it : properties)
		{
			resolved_properties.RemoveProperty(it);
			
			// Even remove the source_properties definition, which would otherwise copy over the "pending" value
			source_properties.RemoveProperty(it);
		}
		
		StyleSheetSpecification::ParseShorthandDeclaration(resolved_properties, id, string_value);
		
		if (parent)
			for (auto const& it : properties)
				parent->DirtyProperty(it);
	}
}

void ResolvedPropertiesDictionary::ResolveVariable(VariableId id)
{
	auto var = source_properties.GetVariable(id);
	if (var)
	{
		if (var->unit == Property::VARIABLETERM)
		{
			String string_value;
			ResolveVariableTerm(string_value, var->value.GetReference<VariableTerm>());
			resolved_properties.SetVariable(id, Property(string_value, Property::STRING));
		}
		else
		{
			resolved_properties.SetVariable(id, *var);
		}
	}
	else
	{
		resolved_properties.RemoveVariable(id);
	}
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
		auto term = property->value.GetReference<VariableTerm>();
		for (auto const& atom : term)
		{
			if (atom.variable != static_cast<VariableId>(0))
				property_dependencies.insert(std::make_pair(atom.variable, id));
		}
	}
}

void ResolvedPropertiesDictionary::UpdateShorthandDependencies(ShorthandId id)
{
	for (auto iter = shorthand_dependencies.begin(); iter != shorthand_dependencies.end();)
	{
		if (iter->second == id)
			iter = shorthand_dependencies.erase(iter);
		else
			++iter;
	}
	
	auto const& shorthands = source_properties.GetDependentShorthands();
	auto shorthand = shorthands.find(id);
	if (shorthand != shorthands.end())
	{
		for (auto const& atom : shorthand->second)
		{
			if (atom.variable != static_cast<VariableId>(0))
				shorthand_dependencies.insert(std::make_pair(atom.variable, id));
		}
	}
}

void ResolvedPropertiesDictionary::UpdateVariableDependencies(VariableId id, bool local_change)
{
	for (auto iter = variable_dependencies.begin(); iter != variable_dependencies.end();)
	{
		if (iter->second == id)
			iter = variable_dependencies.erase(iter);
		else
			++iter;
	}
	
	auto variable = parent->GetVariable(id);
	
	if (local_change)
	{
		auto local_variable = source_properties.GetVariable(id);
	
		if (local_variable && local_variable->unit == Property::VARIABLETERM)
		{
			auto term = local_variable->value.GetReference<VariableTerm>();
			for (auto const& atom : term)
			{
				if (atom.variable != static_cast<VariableId>(0))
					variable_dependencies.insert(std::make_pair(atom.variable, id));
			}
		}
	}

	if (!variable)
	{
		// Variable removed, remove resolved dependent values
		
		auto dependent_variables = variable_dependencies.equal_range(id);
		for (auto iter = dependent_variables.first; iter != dependent_variables.second;)
		{
			resolved_properties.RemoveVariable(iter->second);
			iter = variable_dependencies.erase(iter);
		}
		
		auto dependent_shorthands = shorthand_dependencies.equal_range(id);
		for (auto iter = dependent_shorthands.first; iter != dependent_shorthands.second;)
		{
			iter = shorthand_dependencies.erase(iter);
		}
		
		auto dependent_properties = property_dependencies.equal_range(id);
		for (auto iter = dependent_properties.first; iter != dependent_properties.second;)
		{
			resolved_properties.RemoveProperty(iter->second);
			iter = property_dependencies.erase(iter);
		}
	} else {
		auto dependent_variables = variable_dependencies.equal_range(id);

		UnorderedSet<VariableId> dirty_variables;
		for (auto iter = dependent_variables.first; iter != dependent_variables.second; ++iter)
		{
			ResolveVariable(iter->second);
			dirty_variables.insert(iter->second);
		}
		
		for(auto const& it : dirty_variables)
			UpdateVariableDependencies(it, true);

		auto dependent_shorthands = shorthand_dependencies.equal_range(id);
		for (auto iter = dependent_shorthands.first; iter != dependent_shorthands.second; ++iter)
		{
			ResolveShorthand(iter->second);
		}
		
		auto dependent_properties = property_dependencies.equal_range(id);
		for (auto iter = dependent_properties.first; iter != dependent_properties.second; ++iter)
		{
			ResolveProperty(iter->second);
		}
	}
}

} // namespace Rml
