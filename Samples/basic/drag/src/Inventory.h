#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>

class Inventory {
public:
	/// Constructs a new inventory and opens its window.
	/// @param[in] title The title of the new inventory.
	/// @param[in] position The position of the inventory window.
	/// @param[in] context The context to open the inventory window in.
	Inventory(const Rml::String& title, const Rml::Vector2f& position, Rml::Context* context);
	/// Destroys the inventory and closes its window.
	~Inventory();

	/// Adds a brand-new item into this inventory.
	/// @param[in] name The name of this item.
	void AddItem(const Rml::String& name);

private:
	Rml::ElementDocument* document;
};
