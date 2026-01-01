#pragma once

#include "Header.h"
#include "Property.h"
#include "PropertyParser.h"

namespace Rml {

enum class RelativeTarget { None, ContainingBlockWidth, ContainingBlockHeight, FontSize, ParentFontSize, LineHeight };

class RMLUICORE_API PropertyDefinition final : public NonCopyMoveable {
public:
	PropertyDefinition(PropertyId id, const String& default_value, bool inherited, bool forces_layout);
	~PropertyDefinition();

	/// Registers a parser to parse values for this definition.
	/// @param[in] parser_name The name of the parser (default parsers are 'string', 'keyword', 'number' and 'colour').
	/// @param[in] parser_parameters A comma-separated list of validation parameters for the parser.
	/// @return This property definition.
	PropertyDefinition& AddParser(const String& parser_name, const String& parser_parameters = "");

	/// Set target for relative units when resolving sizes such as percentages.
	PropertyDefinition& SetRelativeTarget(RelativeTarget relative_target);

	/// Called when parsing a RCSS declaration.
	/// @param property[out] The property to set the parsed value onto.
	/// @param value[in] The raw value defined for this property.
	/// @return True if all values were parsed successfully, false otherwise.
	bool ParseValue(Property& property, const String& value) const;
	/// Called to convert a parsed property back into a value.
	/// @param value[out] The string to return the value in.
	/// @param property[in] The processed property to parse.
	/// @return True if the property was reverse-engineered successfully, false otherwise.
	bool GetValue(String& value, const Property& property) const;

	/// Returns true if this property is inherited from parent to child elements.
	bool IsInherited() const;

	/// Returns true if this property forces a re-layout when changed.
	bool IsLayoutForced() const;

	/// Returns the default defined for this property.
	const Property* GetDefaultValue() const;

	/// Returns the target for resolving values with percent and possibly number units.
	RelativeTarget GetRelativeTarget() const;

	/// Return the property id
	PropertyId GetId() const;

private:
	PropertyId id;

	Property default_value;
	bool inherited;
	bool forces_layout;

	struct ParserState {
		PropertyParser* parser;
		ParameterMap parameters;
	};

	Vector<ParserState> parsers;

	RelativeTarget relative_target;
};

} // namespace Rml
