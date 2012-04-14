#pragma once

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Controls/DataSource.h>

using Rocket::Controls::DataSource;
namespace Rocket {
namespace Core {
namespace Lua {

class LuaDataSource : public Rocket::Controls::DataSource
{
public:
    //default initilialize the lua func references to -1
    LuaDataSource(const String& name = "");

	/// Fetches the contents of one row of a table within the data source.
	/// @param[out] row The list of values in the table.
	/// @param[in] table The name of the table to query.
	/// @param[in] row_index The index of the desired row.
	/// @param[in] columns The list of desired columns within the row.
	virtual void GetRow(Rocket::Core::StringList& row, const Rocket::Core::String& table, int row_index, const Rocket::Core::StringList& columns);
	/// Fetches the number of rows within one of this data source's tables.
	/// @param[in] table The name of the table to query.
	/// @return The number of rows within the specified table. Returns -1 in case of an incorrect Lua function.
	virtual int GetNumRows(const Rocket::Core::String& table);

    //make the protected members of DataSource public
    using DataSource::NotifyRowAdd;
    using DataSource::NotifyRowRemove;
    using DataSource::NotifyRowChange;

    //lua reference to DataSource.GetRow
    int getRowRef;
    //lua reference to DataSource.GetNumRows
    int getNumRowsRef;
};

}
}
}