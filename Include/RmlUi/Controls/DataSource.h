/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUICONTROLSDATASOURCE_H
#define RMLUICONTROLSDATASOURCE_H

#include "Header.h"
#include "../Core/Types.h"
#include <list>

namespace Rml {
namespace Controls {

class DataSourceListener;

/**
	Generic object that provides a database-like interface for requesting rows from a table.
	@author Robert Curry
 */

class RMLUICONTROLS_API DataSource
{
	public:
		DataSource(const Rml::Core::String& name = "");
		virtual ~DataSource();

		const Rml::Core::String& GetDataSourceName();
		static DataSource* GetDataSource(const Rml::Core::String& data_source_name);

		/// Fetches the contents of one row of a table within the data source.
		/// @param[out] row The list of values in the table.
		/// @param[in] table The name of the table to query.
		/// @param[in] row_index The index of the desired row.
		/// @param[in] columns The list of desired columns within the row.
		virtual void GetRow(Rml::Core::StringList& row, const Rml::Core::String& table, int row_index, const Rml::Core::StringList& columns) = 0;
		/// Fetches the number of rows within one of this data source's tables.
		/// @param[in] table The name of the table to query.
		/// @return The number of rows within the specified table.
		virtual int GetNumRows(const Rml::Core::String& table) = 0;

		void AttachListener(DataSourceListener* listener);
		void DetachListener(DataSourceListener* listener);

		virtual void* GetScriptObject() const;

		static const Rml::Core::String CHILD_SOURCE;
		static const Rml::Core::String DEPTH;
		static const Rml::Core::String NUM_CHILDREN;

	protected:
		/// Tells all attached listeners that one or more rows have been added to the data source.
		/// @param[in] table The name of the table to have rows added to it.
		/// @param[in] first_row_added The index of the first row added.
		/// @param[in] num_rows_added The number of rows added (including the first row).
		void NotifyRowAdd(const Rml::Core::String& table, int first_row_added, int num_rows_added);

		/// Tells all attached listeners that one or more rows have been removed from the data source.
		/// @param[in] table The name of the table to have rows removed from it.
		/// @param[in] first_row_removed The index of the first row removed.
		/// @param[in] num_rows_removed The number of rows removed (including the first row).
		void NotifyRowRemove(const Rml::Core::String& table, int first_row_removed, int num_rows_removed);

		/// Tells all attached listeners that one or more rows have been changed in the data source.
		/// @param[in] table The name of the table to have rows changed in it.
		/// @param[in] first_row_changed The index of the first row changed.
		/// @param[in] num_rows_changed The number of rows changed (including the first row).
		void NotifyRowChange(const Rml::Core::String& table, int first_row_changed, int num_rows_changed);

		/// Tells all attached listeners that the row structure has completely changed in the data source.
		/// @param[in] table The name of the table to have rows changed in it.
		void NotifyRowChange(const Rml::Core::String& table);

		/// Helper function for building a result set.
		typedef Core::UnorderedMap< Rml::Core::String, Rml::Core::String > RowMap;
		void BuildRowEntries(Rml::Core::StringList& row, const RowMap& row_map, const Rml::Core::StringList& columns);

	private:
		Core::String name;

		using ListenerList = std::list< DataSourceListener* >;
		ListenerList listeners;
};

}
}

#endif // RMLUICONTROLSDATASOURCE_H

