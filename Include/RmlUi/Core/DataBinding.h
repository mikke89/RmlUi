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

#ifndef RMLUICOREDATABINDING_H
#define RMLUICOREDATABINDING_H

#include "Header.h"
#include "Types.h"
#include "Variant.h"
#include "StringUtilities.h"

namespace Rml {
namespace Core {

class Element;
class DataModel;


class DataViewText {
public:
	DataViewText(Element* in_parent_element, const String& in_text, size_t index_begin_search = 0);

	inline operator bool() const {
		return !data_entries.empty() && parent_element;
	}

	inline bool IsDirty() const {
		return is_dirty;
	}

	bool Update(const DataModel& model);

private:
	String CreateText() const;

	struct DataEntry {
		size_t index = 0; // Index into 'text'
		String name;
		String value;
	};

	ObserverPtr<Element> parent_element;
	String text;
	std::vector<DataEntry> data_entries;
	bool is_dirty = false;
};


class DataViews {
public:

	void AddTextView(DataViewText&& text_view) {
		text_views.push_back(std::move(text_view));
	}

	bool Update(const DataModel& model)
	{
		bool result = false;
		for (auto& view : text_views)
			result |= view.Update(model);
		return result;
	}

private:
	std::vector<DataViewText> text_views;
};


class DataModel {
public:
	using Type = Variant::Type;

	struct Binding {
		Type type = Type::NONE;
		const void* ptr = nullptr;
	};

	bool GetValue(const String& name, String& out_value) const;

	using Bindings = Rml::Core::UnorderedMap<Rml::Core::String, Binding>;

	Bindings bindings;

	DataViews views;
};


class DataModelHandle {
public:
	using Type = Variant::Type;

	DataModelHandle() : model(nullptr) {}
	DataModelHandle(DataModel* model) : model(model) {}

	DataModelHandle& BindData(String name, Type type, const void* ptr)
	{
		RMLUI_ASSERT(model);
		model->bindings.emplace(name, DataModel::Binding{ type, ptr });
		return *this;
	}

	void UpdateViews() {
		RMLUI_ASSERT(model);
		model->views.Update(*model);
	}

	operator bool() { return model != nullptr; }

private:
	DataModel* model;
};


}
}

#endif
