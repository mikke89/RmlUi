#include "PropertyParserFilter.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Filter.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/StyleSheetTypes.h"

namespace Rml {

PropertyParserFilter::PropertyParserFilter() {}

PropertyParserFilter::~PropertyParserFilter() {}

bool PropertyParserFilter::ParseValue(Property& property, const String& filter_string_value, const ParameterMap& /*parameters*/) const
{
	// Filters are declared as
	//   filter: <filter-value>[ <filter-value> ...];
	// Where <filter-value> is specified with inline properties
	//   filter: brightness( <shorthand properties> ) ...;

	if (filter_string_value.empty() || filter_string_value == "none")
	{
		property.value = Variant(FiltersPtr());
		property.unit = Unit::FILTER;
		return true;
	}

	RMLUI_ZoneScoped;

	// Make sure we don't split inside the parenthesis since they may appear in filter shorthands.
	StringList filter_string_list;
	StringUtilities::ExpandString(filter_string_list, filter_string_value, ' ', '(', ')', true);

	FilterDeclarationList filters;
	filters.value = filter_string_value;
	filters.list.reserve(filter_string_list.size());

	// Get or instance each filter in the comma-separated string list
	for (const String& filter_string : filter_string_list)
	{
		const size_t shorthand_open = filter_string.find('(');
		const size_t shorthand_close = filter_string.rfind(')');
		const bool invalid_parenthesis = (shorthand_open == String::npos || shorthand_close == String::npos || shorthand_open >= shorthand_close);

		if (invalid_parenthesis)
		{
			// We found no parenthesis, filters can only be declared anonymously for now.
			Log::Message(Log::LT_WARNING, "Invalid syntax for font-effect '%s'.", filter_string.c_str());
			return false;
		}
		else
		{
			const String type = StringUtilities::StripWhitespace(filter_string.substr(0, shorthand_open));

			// Check for valid filter type
			FilterInstancer* instancer = Factory::GetFilterInstancer(type);
			if (!instancer)
			{
				Log::Message(Log::LT_WARNING, "Filter type '%s' not found.", type.c_str());
				return false;
			}

			const String shorthand = filter_string.substr(shorthand_open + 1, shorthand_close - shorthand_open - 1);
			const PropertySpecification& specification = instancer->GetPropertySpecification();

			// Parse the shorthand properties given by the 'filter' shorthand property
			PropertyDictionary properties;
			if (!specification.ParsePropertyDeclaration(properties, "filter", shorthand))
			{
				// Empty values are allowed in filters, if the value is not empty we must have encountered a parser error.
				if (!StringUtilities::StripWhitespace(shorthand).empty())
					return false;
			}

			// Set unspecified values to their defaults
			specification.SetPropertyDefaults(properties);

			filters.list.emplace_back(FilterDeclaration{type, instancer, std::move(properties)});
		}
	}

	if (filters.list.empty())
		return false;

	property.value = Variant(MakeShared<FilterDeclarationList>(std::move(filters)));
	property.unit = Unit::FILTER;

	return true;
}

} // namespace Rml
