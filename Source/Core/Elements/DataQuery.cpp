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

#include "../../../Include/RmlUi/Core/Elements/DataQuery.h"
#include "../../../Include/RmlUi/Core/Elements/DataSource.h"
#include <algorithm>

namespace Rml {

class DataQuerySort
{
	public:
		DataQuerySort(const StringList& _order_parameters)
		{
			order_parameters = _order_parameters;
		}

		bool operator()(const StringList& RMLUI_UNUSED_PARAMETER(left), const StringList& RMLUI_UNUSED_PARAMETER(right))
		{
			RMLUI_UNUSED(left);
			RMLUI_UNUSED(right);

			return false;
		}

	private:
		StringList order_parameters;
};



DataQuery::DataQuery(DataSource* data_source, const String& table, const String& _fields, int offset, int limit, const String& order)
{
	ExecuteQuery(data_source, table, _fields, offset, limit, order);
}



DataQuery::DataQuery()
{
	data_source = nullptr;
	table = "";
	offset = -1;
	limit = -1;
}



DataQuery::~DataQuery()
{
}



void DataQuery::ExecuteQuery(DataSource* _data_source, const String& _table, const String& _fields, int _offset, int _limit, const String& order)
{
	data_source = _data_source;
	table = _table;
	offset = _offset;
	limit = _limit;

	// Set up the field list and field index cache.
	StringUtilities::ExpandString(fields, _fields);
	for (size_t i = 0; i < fields.size(); i++)
	{
		field_indices[fields[i]] = i;
	}

	// Initialise the row pointer.
	current_row = -1;

	// If limit is -1, then we fetch to the end of the data source.
	if (limit == -1)
	{
		limit = data_source->GetNumRows(table) - offset;
	}

	if (!order.empty())
	{
		// Fetch the rows from offset to limit.
		rows.resize(limit);
		for (int i = 0; i < limit; i++)
		{
			data_source->GetRow(rows[i], table, offset + i, fields);
		}

		// Now sort the rows, based on the ordering requirements.
		StringList order_parameters;
		StringUtilities::ExpandString(order_parameters, order);
		std::sort(rows.begin(), rows.end(), DataQuerySort(order_parameters));
	}
}



bool DataQuery::NextRow()
{
	current_row++;

	if (current_row >= limit)
	{
		return false;
	}

	LoadRow();
	return true;
}



bool DataQuery::IsFieldSet(const String& field) const
{
	FieldIndices::const_iterator itr = field_indices.find(field);
	if (itr == field_indices.end() || (*itr).second >= rows[current_row].size())
	{
		return false;
	}

	return true;
}



void DataQuery::LoadRow()
{
	RMLUI_ASSERT(current_row <= (int)rows.size());
	if (current_row >= (int)rows.size())
	{
		rows.push_back(StringList());
		data_source->GetRow(rows[current_row], table, offset + current_row, fields);
	}
}

} // namespace Rml
