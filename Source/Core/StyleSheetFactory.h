#pragma once

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class StyleSheetContainer;
enum class StructuralSelectorType;
struct StructuralSelector;

/**
    Creates stylesheets on the fly as needed. The factory keeps a cache of built sheets for optimisation.
 */

class StyleSheetFactory {
public:
	~StyleSheetFactory();

	/// Initialise the style factory
	static bool Initialise();
	/// Shutdown style manager
	static void Shutdown();

	/// Gets the named sheet, retrieving it from the cache if its already been loaded.
	/// @param sheet name of sheet to load
	/// @lifetime Returned pointer is valid until the next call to ClearStyleSheetCache or Shutdown, it should not be stored around.
	static const StyleSheetContainer* GetStyleSheetContainer(const String& sheet);

	/// Clear the style sheet cache.
	static void ClearStyleSheetCache();

	/// Returns one of the available node selectors.
	/// @param name[in] The name of the desired selector.
	/// @return The selector registered with the given name, or nullptr if none exists.
	static StructuralSelector GetSelector(const String& name);

private:
	StyleSheetFactory();

	// Loads an individual style sheet
	UniquePtr<const StyleSheetContainer> LoadStyleSheetContainer(const String& sheet);

	// Individual loaded stylesheets
	using StyleSheets = UnorderedMap<String, UniquePtr<const StyleSheetContainer>>;
	StyleSheets stylesheets;

	// Custom complex selectors available for style sheets.
	using SelectorMap = UnorderedMap<String, StructuralSelectorType>;
	SelectorMap selectors;
};

} // namespace Rml
