#ifndef INVENTORY_H
#define INVENTORY_H

#include <Rocket/Core/String.h>
#include <Rocket/Core/Context.h>
#include <Rocket/Core/ElementDocument.h>

/**
	@author Peter Curry
 */

class Inventory
{
public:
	/// Constructs a new inventory and opens its window.
	/// @param[in] title The title of the new inventory.
	/// @param[in] position The position of the inventory window.
	/// @param[in] context The context to open the inventory window in.
	Inventory(const Rocket::Core::String& title, const Rocket::Core::Vector2f& position, Rocket::Core::Context* context);
	/// Destroys the inventory and closes its window.
	~Inventory();

	/// Adds a brand-new item into this inventory.
	/// @param[in] name The name of this item.
	void AddItem(const Rocket::Core::String& name);

private:
	Rocket::Core::ElementDocument* document;
};

#endif
