/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "DataView.h"
#include "../../Include/RmlUi/Core/Element.h"
#include <algorithm>

namespace Rml {

DataView::~DataView() {}

Element* DataView::GetElement() const
{
	Element* result = attached_element.get();
	if (!result)
		Log::Message(Log::LT_WARNING, "Could not retrieve element in view, was it destroyed?");
	return result;
}

int DataView::GetSortOrder() const
{
	return sort_order;
}

bool DataView::IsValid() const
{
	return static_cast<bool>(attached_element);
}

DataView::DataView(Element* element, int bias) : attached_element(element->GetObserverPtr()), sort_order(bias + 1000)
{
	RMLUI_ASSERT(bias >= -1000 && bias <= 999);

	if (element)
	{
		for (Element* parent = element->GetParentNode(); parent; parent = parent->GetParentNode())
			sort_order += 2000;
	}
}

DataViews::DataViews() {}

DataViews::~DataViews() {}

void DataViews::Add(DataViewPtr view)
{
	views_to_add.push_back(std::move(view));
}

void DataViews::OnElementRemove(Element* element)
{
	for (auto it = views.begin(); it != views.end();)
	{
		auto& view = *it;
		if (view && view->GetElement() == element)
		{
			views_to_remove.push_back(std::move(view));
			it = views.erase(it);
		}
		else
			++it;
	}
}

bool DataViews::Update(DataModel& model, const DirtyVariables& dirty_variables)
{
	bool result = false;
	size_t num_dirty_variables_prev = 0;

	// View updates may result in newly added views, or even new dirty variables. Thus, we do the
	// update recursively but with an upper limit. Without the loop, newly added views won't be
	// updated until the next Update() call.
	for (int i = 0; (i == 0 || !views_to_add.empty() || num_dirty_variables_prev != dirty_variables.size()) && i < 10; i++)
	{
		num_dirty_variables_prev = dirty_variables.size();

		Vector<DataView*> dirty_views;

		if (!views_to_add.empty())
		{
			views.reserve(views.size() + views_to_add.size());
			for (auto&& view : views_to_add)
			{
				dirty_views.push_back(view.get());
				for (const String& variable_name : view->GetVariableNameList())
					name_view_map.emplace(variable_name, view.get());

				views.push_back(std::move(view));
			}
			views_to_add.clear();
		}

		for (const String& variable_name : dirty_variables)
		{
			auto pair = name_view_map.equal_range(variable_name);
			for (auto it = pair.first; it != pair.second; ++it)
				dirty_views.push_back(it->second);
		}

		// Remove duplicate entries
		std::sort(dirty_views.begin(), dirty_views.end());
		auto it_remove = std::unique(dirty_views.begin(), dirty_views.end());
		dirty_views.erase(it_remove, dirty_views.end());

		// Sort by the element's depth in the document tree so that any structural changes due to a changed variable are reflected in the element's
		// children. Eg. the 'data-for' view will remove children if any of its data variable array size is reduced.
		std::sort(dirty_views.begin(), dirty_views.end(), [](auto&& left, auto&& right) { return left->GetSortOrder() < right->GetSortOrder(); });

		for (DataView* view : dirty_views)
		{
			RMLUI_ASSERT(view);
			if (!view)
				continue;

			if (view->IsValid())
				result |= view->Update(model);
		}

		// Destroy views marked for destruction
		// @performance: Horrible...
		if (!views_to_remove.empty())
		{
			for (const auto& view : views_to_remove)
			{
				for (auto it = name_view_map.begin(); it != name_view_map.end();)
				{
					if (it->second == view.get())
						it = name_view_map.erase(it);
					else
						++it;
				}
			}

			views_to_remove.clear();
		}
	}

	return result;
}

} // namespace Rml
