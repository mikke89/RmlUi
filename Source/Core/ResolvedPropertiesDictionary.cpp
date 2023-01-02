#include "../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../Include/RmlUi/Core/PropertyDefinition.h"

#include "ResolvedPropertiesDictionary.h"
#include "ElementDefinition.h"
#include "ElementStyle.h"

namespace Rml {

ResolvedPropertiesDictionary::ResolvedPropertiesDictionary(ElementStyle* parent) : parent(parent) {}

ResolvedPropertiesDictionary::ResolvedPropertiesDictionary(ElementStyle* parent, const ElementDefinition* source) : parent(parent)
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
	UpdatePropertyDependencies(id);
	if (value.unit == Property::VARIABLETERM)
		dirty_properties.Insert(id);
	else
		resolved_properties.SetProperty(id, value);
}

void ResolvedPropertiesDictionary::SetVariable(VariableId id, const Property& value)
{
	source_properties.SetVariable(id, value);
	UpdateVariableDependencies(id);
	if (value.unit == Property::VARIABLETERM)
		dirty_variables.insert(id);
	else
		resolved_properties.SetVariable(id, value);
}

void ResolvedPropertiesDictionary::SetDependentShorthand(ShorthandId id, const VariableTerm &value)
{
	source_properties.SetDependent(id, value);
	dirty_shorthands.insert(id);
}

bool ResolvedPropertiesDictionary::RemoveProperty(PropertyId id)
{
	auto size_before = source_properties.GetNumProperties();

	source_properties.RemoveProperty(id);
	resolved_properties.RemoveProperty(id);
	dirty_properties.Insert(id);

	return source_properties.GetNumProperties() != size_before;
}

bool ResolvedPropertiesDictionary::RemoveVariable(VariableId id)
{
	auto size_before = source_properties.GetNumVariables();

	source_properties.RemoveVariable(id);
	resolved_properties.RemoveVariable(id);
	dirty_variables.insert(id);
	
	return source_properties.GetNumVariables() != size_before;
}

bool ResolvedPropertiesDictionary::AnyPropertiesDirty() const
{
	return !dirty_properties.Empty() || !dirty_variables.empty() || !dirty_shorthands.empty();
}

const PropertyDictionary& ResolvedPropertiesDictionary::GetProperties() const
{
	return resolved_properties;
}

void ResolvedPropertiesDictionary::ResolveDirtyValues()
{
	if (!parent)
		return;
	
	// update dirty value variable dependencies	
	for (auto const& it : dirty_variables)
		UpdateVariableDependencies(it);

	for (auto const& it : dirty_shorthands)
		UpdateShorthandDependencies(it);
	
	for (auto const& it : dirty_properties)
		UpdatePropertyDependencies(it);

	// apply externally dirtied variables (from parent style or tree ancestors)
	for(auto const& id : parent->GetDirtyVariables())
	{
		auto dependent_variables = variable_dependencies.find(id);
		if (dependent_variables != variable_dependencies.end())
			for (auto const& it : dependent_variables->second)
				dirty_variables.insert(it);
		
		auto dependent_shorthands = shorthand_dependencies.find(id);
		if (dependent_shorthands != shorthand_dependencies.end())
			for (auto const& it : dependent_shorthands->second)
				dirty_shorthands.insert(it);
		
		auto dependent_properties = property_dependencies.find(id);
		if (dependent_properties != property_dependencies.end())
			for (auto const& it : dependent_properties->second)
				dirty_properties.Insert(it);
	}

	// resolve dirty variables using iterative depth-first-search with graph coloring for dependency graph navigation and cycle detection
	if (!dirty_variables.empty())
	{
		// 3 colors:
		// not in map: unknown
		//      false: known, in progress
		//       true: completed
		UnorderedMap<VariableId, bool> colors;
		
		std::list<VariableId> sorted_variables;
		PropertyDictionary variable_cache;
		
		for(auto const& id : dirty_variables)
		{
			if (colors.contains(id))
				continue;
			
			std::stack<VariableId> stack;
			stack.push(id);
			
			while(!stack.empty())
			{
				auto top = stack.top();
				
				{
					auto color = colors.find(top);
					if (color != colors.end())
					{
						if (!color->second)
						{
							color->second = true;
							sorted_variables.push_front(top);
						}
						
						stack.pop();
						continue;
					}
					
					colors[top] = false;
				}
				
				auto value = source_properties.GetVariable(top);
				if (!value)
					value = parent->GetVariable(top);

				if (!value || value->unit != Property::VARIABLETERM)
					continue;
				
				for (auto const& atom : value->value.GetReference<VariableTerm>())
				{
					if (atom.variable != static_cast<VariableId>(0))
					{
						auto color = colors.find(atom.variable);
						if (color == colors.end())
							stack.push(atom.variable);
						else if (!color->second)
							Log::Message(Log::LT_ERROR, "Cycle detected during RCSS variable resolution for '%s'. Replacing with empty value.", GetVariableName(atom.variable).c_str());
					}
				}
			}
		}
		
		for (auto const& it : sorted_variables)
			if (dirty_variables.contains(it))
				ResolveVariable(it);
		
		dirty_variables.clear();
	}
	
	// resolve dirty properties
	for (auto const& it : dirty_shorthands)
		ResolveShorthand(it);

	for (auto const& it : dirty_properties)
		ResolveProperty(it);
	
	dirty_shorthands.clear();
	dirty_properties.Clear();
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
			{
				parent->DirtyProperty(it);
				dirty_properties.Erase(it);
			}
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
			resolved_properties.SetVariable(id, *var);
	}
	else
		resolved_properties.RemoveVariable(id);

	if (parent)
		parent->DirtyVariable(id);
}

void ResolvedPropertiesDictionary::UpdatePropertyDependencies(PropertyId id)
{
	for (auto iter = property_dependencies.begin(); iter != property_dependencies.end();)
	{
		iter->second.Erase(id);
		if (iter->second.Empty())
			iter = property_dependencies.erase(iter);
		else
			++iter;
	}

	auto property = source_properties.GetProperty(id);

	if (property && property->unit == Property::VARIABLETERM)
	{
		auto term = property->value.GetReference<VariableTerm>();
		for (auto const& atom : term)
			if (atom.variable != static_cast<VariableId>(0))
				property_dependencies[atom.variable].Insert(id);
	}
}

void ResolvedPropertiesDictionary::UpdateShorthandDependencies(ShorthandId id)
{
	for (auto iter = shorthand_dependencies.begin(); iter != shorthand_dependencies.end();)
	{
		iter->second.erase(id);
		if (iter->second.empty())
			iter = shorthand_dependencies.erase(iter);
		else
			++iter;
	}
	
	auto const& shorthands = source_properties.GetDependentShorthands();
	auto shorthand = shorthands.find(id);
	if (shorthand != shorthands.end())
		for (auto const& atom : shorthand->second)
			if (atom.variable != static_cast<VariableId>(0))
				shorthand_dependencies[atom.variable].insert(id);
}

void ResolvedPropertiesDictionary::UpdateVariableDependencies(VariableId id)
{
	// sanitize dependencies on local variable
	for (auto iter = variable_dependencies.begin(); iter != variable_dependencies.end();)
	{
		iter->second.erase(id);
		if (iter->second.empty())
			iter = variable_dependencies.erase(iter);
		else
			++iter;
	}
	
	auto local_variable = source_properties.GetVariable(id);

	if (local_variable && local_variable->unit == Property::VARIABLETERM)
	{
		auto term = local_variable->value.GetReference<VariableTerm>();
		for (auto const& atom : term)
			if (atom.variable != static_cast<VariableId>(0))
				variable_dependencies[atom.variable].insert(id);
	}
}

} // namespace Rml
