#pragma once

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Types.h"
#include <algorithm>

namespace Rml {

template <typename ID>
class IdNameMap {
	Vector<String> name_map; // IDs are indices into the name_map
	UnorderedMap<String, ID> reverse_map;

protected:
	IdNameMap(size_t num_ids_to_reserve)
	{
		static_assert((int)ID::Invalid == 0, "Invalid id must be zero");
		name_map.reserve(num_ids_to_reserve);
		reverse_map.reserve(num_ids_to_reserve);
		AddPair(ID::Invalid, "invalid");
	}

public:
	void AddPair(ID id, const String& name)
	{
		// Should only be used for defined IDs
		if ((size_t)id >= name_map.size())
			name_map.resize(1 + (size_t)id);
		name_map[(size_t)id] = name;
		bool inserted = reverse_map.emplace(name, id).second;
		RMLUI_ASSERT(inserted);
		(void)inserted;
	}

	bool AssertAllInserted(ID number_of_defined_ids) const
	{
		std::ptrdiff_t cnt = std::count_if(name_map.begin(), name_map.end(), [](const String& name) { return !name.empty(); });
		return cnt == (std::ptrdiff_t)number_of_defined_ids && reverse_map.size() == (size_t)number_of_defined_ids;
	}

	ID GetId(const String& name) const
	{
		auto it = reverse_map.find(name);
		if (it != reverse_map.end())
			return it->second;
		return ID::Invalid;
	}
	const String& GetName(ID id) const
	{
		if (static_cast<size_t>(id) < name_map.size())
			return name_map[static_cast<size_t>(id)];
		return name_map[static_cast<size_t>(ID::Invalid)];
	}

	ID GetOrCreateId(const String& name)
	{
		// All predefined properties must be set before possibly adding custom properties here
		RMLUI_ASSERT(name_map.size() == reverse_map.size());

		ID next_id = static_cast<ID>(name_map.size());

		// Only insert if not already in list
		auto pair = reverse_map.emplace(name, next_id);
		const auto& it = pair.first;
		bool inserted = pair.second;

		if (inserted)
			name_map.push_back(name);

		// Return the property id that already existed, or the new one if inserted
		return it->second;
	}
};

class PropertyIdNameMap : public IdNameMap<PropertyId> {
public:
	PropertyIdNameMap(size_t reserve_num_properties) : IdNameMap(reserve_num_properties) {}
};

class ShorthandIdNameMap : public IdNameMap<ShorthandId> {
public:
	ShorthandIdNameMap(size_t reserve_num_shorthands) : IdNameMap(reserve_num_shorthands) {}
};

} // namespace Rml
