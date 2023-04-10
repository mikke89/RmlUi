#include "ResolvedPropertiesDictionary.h"
#include "../Include/RmlUi/Core/PropertyDefinition.h"
#include "../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "ElementDefinition.h"
#include "ElementStyle.h"

namespace Rml {

ResolvedPropertiesDictionary::ResolvedPropertiesDictionary(ElementStyle* parent) : parent(parent) {}

ResolvedPropertiesDictionary::ResolvedPropertiesDictionary(ElementStyle* parent, const ElementDefinition* source) : parent(parent)
{
	auto const& props = source->GetProperties();

	if (source)
	{
		for (auto const& it : props.GetPropertyVariables())
			SetPropertyVariable(it.first, it.second);

		for (auto const& it : props.GetDependentShorthands())
			SetDependentShorthand(it.first, it.second);

		for (auto const& it : props.GetProperties())
			SetProperty(it.first, it.second);
	}
}

const Property* ResolvedPropertiesDictionary::GetProperty(PropertyId id) const
{
	return resolved_properties.GetProperty(id);
}

const Property* ResolvedPropertiesDictionary::GetPropertyVariable(String const& name) const
{
	return resolved_properties.GetPropertyVariable(name);
}

void ResolvedPropertiesDictionary::SetProperty(PropertyId id, const Property& value)
{
	// ignore pending values
	if (value.unit == Property::UNKNOWN)
		return;

	if (value.unit == Property::VARIABLETERM)
		source_properties.SetProperty(id, value);
	else
		resolved_properties.SetProperty(id, value);

	// if is or was a dependent property, update dependent data store
	if (value.unit == Property::VARIABLETERM || source_properties.GetProperty(id) != nullptr)
	{
		dirty_values.insert(id);
		UpdatePropertyDependencies(id);
	}
}

void ResolvedPropertiesDictionary::SetPropertyVariable(String const& name, const Property& value)
{
	if (value.unit == Property::VARIABLETERM)
		source_properties.SetPropertyVariable(name, value);
	else
		resolved_properties.SetPropertyVariable(name, value);

	// if is or was a dependent variable, update dependent data store
	if (value.unit == Property::VARIABLETERM || source_properties.GetPropertyVariable(name) != nullptr)
	{
		dirty_values.insert(name);
		UpdatePropertyVariableDependencies(name);
	}
}

void ResolvedPropertiesDictionary::SetDependentShorthand(ShorthandId id, const PropertyVariableTerm& value)
{
	source_properties.SetDependent(id, value);
	dirty_values.insert(id);
}

bool ResolvedPropertiesDictionary::RemoveProperty(PropertyId id)
{
	auto size_before = source_properties.GetNumProperties();

	source_properties.RemoveProperty(id);
	resolved_properties.RemoveProperty(id);
	dirty_values.insert(id);

	return source_properties.GetNumProperties() != size_before;
}

bool ResolvedPropertiesDictionary::RemovePropertyVariable(String const& name)
{
	auto size_before = source_properties.GetNumPropertyVariables();

	source_properties.RemovePropertyVariable(name);
	resolved_properties.RemovePropertyVariable(name);
	dirty_values.insert(name);

	return source_properties.GetNumPropertyVariables() != size_before;
}

bool ResolvedPropertiesDictionary::AnyPropertiesDirty() const
{
	return !dirty_values.empty();
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
	for (auto const& it : dirty_values)
	{
		switch (it.type)
		{
		case IdWrapper::Variable: UpdatePropertyVariableDependencies(it.get<String>()); break;
		case IdWrapper::Shorthand: UpdateShorthandDependencies(it.get<ShorthandId>()); break;
		case IdWrapper::Property: UpdatePropertyDependencies(it.get<PropertyId>()); break;
		}
	}

	// apply externally dirtied variables (from parent style or tree ancestors)
	for (auto const& id : parent->GetDirtyVariables())
	{
		auto dependents = dependencies.find(id);
		if (dependents != dependencies.end())
			for (auto const& it : dependents->second)
				dirty_values.insert(it);
	}

	// resolve dirty variables using iterative depth-first-search with graph coloring for dependency graph navigation and cycle detection
	// 3 colors:
	// not in map: unknown
	//      false: known, in progress
	//       true: completed
	UnorderedMap<String, bool> colors;

	std::list<String> sorted_variables;
	PropertyDictionary variable_cache;

	for (auto const& it : dirty_values)
	{
		if (it.type != IdWrapper::Variable)
			continue;

		auto const& id = it.get<String>();

		if (colors.find(id) != colors.end())
			continue;

		std::stack<String> stack;
		stack.push(id);

		while (!stack.empty())
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

			auto value = source_properties.GetPropertyVariable(top);
			if (!value)
				value = parent->GetPropertyVariable(top);

			if (!value || value->unit != Property::VARIABLETERM)
				continue;

			for (auto const& atom : value->value.GetReference<PropertyVariableTerm>())
			{
				if (!atom.variable.empty())
				{
					auto color = colors.find(atom.variable);
					if (color == colors.end())
						stack.push(atom.variable);
					else if (!color->second)
						Log::Message(Log::LT_ERROR, "Cycle detected during RCSS variable resolution for '%s'. Replacing with empty value.",
							atom.variable.c_str());
				}
			}
		}
	}

	for (auto const& it : sorted_variables)
	{
		if (dirty_values.find(it) != dirty_values.end())
		{
			ResolvePropertyVariable(it);
			dirty_values.erase(it);
		}
	}

	// resolve dirty properties
	for (auto const& it : dirty_values)
		if (it.type == IdWrapper::Shorthand)
			ResolveShorthand(it.get<ShorthandId>());

	for (auto const& it : dirty_values)
		if (it.type == IdWrapper::Property)
			ResolveProperty(it.get<PropertyId>());

	dirty_values.clear();
}

void ResolvedPropertiesDictionary::ResolveVariableTerm(String& result, const PropertyVariableTerm& term)
{
	StringList atoms;
	for (auto const& atom : term)
	{
		if (!atom.variable.empty())
		{
			const Property* var = nullptr;

			if (parent)
				var = parent->GetPropertyVariable(atom.variable);
			else
				var = resolved_properties.GetPropertyVariable(atom.variable);

			if (var)
				atoms.push_back(var->ToString());
			else
			{
				if (atom.constant.empty())
					Log::Message(Log::LT_ERROR, "Failed to resolve RCSS variable '%s'. No fallback was provided.",
						atom.variable.c_str());

				atoms.push_back(atom.constant);
			}
		}
		else
			atoms.push_back(atom.constant);
	}
	
	// Join without any actual delimiter, thus \0
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
			ResolveVariableTerm(string_value, value->value.GetReference<PropertyVariableTerm>());

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

		StyleSheetSpecification::ParseShorthandDeclaration(resolved_properties, id, string_value);

		if (parent)
			for (auto const& it : properties)
			{
				parent->DirtyProperty(it);
				dirty_values.erase(it);
			}
	}
}

void ResolvedPropertiesDictionary::ResolvePropertyVariable(String const& name)
{
	auto var = source_properties.GetPropertyVariable(name);
	if (var)
	{
		if (var->unit == Property::VARIABLETERM)
		{
			String string_value;
			ResolveVariableTerm(string_value, var->value.GetReference<PropertyVariableTerm>());
			resolved_properties.SetPropertyVariable(name, Property(string_value, Property::STRING));
		}
		else
			resolved_properties.SetPropertyVariable(name, *var);
	}
	else
		resolved_properties.RemovePropertyVariable(name);

	if (parent)
		parent->DirtyVariable(name);
}

void ResolvedPropertiesDictionary::UpdatePropertyDependencies(PropertyId id)
{
	for (auto iter = dependencies.begin(); iter != dependencies.end();)
	{
		iter->second.erase(IdWrapper(id));
		if (iter->second.empty())
			iter = dependencies.erase(iter);
		else
			++iter;
	}

	auto property = source_properties.GetProperty(id);

	if (property)
	{
		if (property->unit == Property::VARIABLETERM)
		{
			auto term = property->value.GetReference<PropertyVariableTerm>();
			for (auto const& atom : term)
				if (!atom.variable.empty())
					dependencies[atom.variable].insert(IdWrapper(id));
		}
		else
			source_properties.RemoveProperty(id);
	}
}

void ResolvedPropertiesDictionary::UpdateShorthandDependencies(ShorthandId id)
{
	for (auto iter = dependencies.begin(); iter != dependencies.end();)
	{
		iter->second.erase(IdWrapper(id));
		if (iter->second.empty())
			iter = dependencies.erase(iter);
		else
			++iter;
	}

	auto const& shorthands = source_properties.GetDependentShorthands();
	auto shorthand = shorthands.find(id);
	if (shorthand != shorthands.end())
		for (auto const& atom : shorthand->second)
			if (!atom.variable.empty())
				dependencies[atom.variable].insert(IdWrapper(id));
}

void ResolvedPropertiesDictionary::UpdatePropertyVariableDependencies(String const& name)
{
	// sanitize dependencies on local variable
	for (auto iter = dependencies.begin(); iter != dependencies.end();)
	{
		iter->second.erase(IdWrapper(name));
		if (iter->second.empty())
			iter = dependencies.erase(iter);
		else
			++iter;
	}

	auto local_variable = source_properties.GetPropertyVariable(name);

	if (local_variable)
	{
		if (local_variable->unit == Property::VARIABLETERM)
		{
			auto term = local_variable->value.GetReference<PropertyVariableTerm>();
			for (auto const& atom : term)
				if (!atom.variable.empty())
					dependencies[atom.variable].insert(IdWrapper(name));
		}
		else
			source_properties.RemovePropertyVariable(name);
	}
}

} // namespace Rml
