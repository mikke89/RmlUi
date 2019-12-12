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
class ElementText;


class DataViewText {
public:
	DataViewText(const DataModel& model, ElementText* in_element, const String& in_text, size_t index_begin_search = 0);

	inline operator bool() const {
		return !data_entries.empty() && element;
	}

	bool Update(const DataModel& model);

private:
	String BuildText() const;

	struct DataEntry {
		size_t index = 0; // Index into 'text'
		String name;
		String value;
	};

	ObserverPtr<Element> element;
	String text;
	std::vector<DataEntry> data_entries;
};



class DataViewAttribute {
public:
	DataViewAttribute(const DataModel& model, Element* element, const String& attribute_name, const String& value_name);

	inline operator bool() const {
		return !attribute_name.empty() && element;
	}
	bool Update(const DataModel& model);

private:
	ObserverPtr<Element> element;
	String attribute_name;
	String value_name;
};


class DataViews {
public:

	void AddView(DataViewText&& view) {
		text_views.push_back(std::move(view));
	}

	void AddView(DataViewAttribute&& view) {
		attribute_views.push_back(std::move(view));
	}

	bool Update(const DataModel& model)
	{
		bool result = false;
		for (auto& view : text_views)
			result |= view.Update(model);
		for (auto& view : attribute_views)
			result |= view.Update(model);
		return result;
	}

private:
	std::vector<DataViewText> text_views;
	std::vector<DataViewAttribute> attribute_views;
};


class DataControllerAttribute {
public:
	DataControllerAttribute(const DataModel& model, const String& in_attribute_name, const String& in_value_name);

	inline operator bool() const {
		return !attribute_name.empty();
	}
	bool Update(Element* element, const DataModel& model);


	bool OnAttributeChange( const ElementAttributes& changed_attributes)
	{
		bool result = false;
		if (changed_attributes.count(attribute_name) > 0)
		{
			dirty = true;
			result = true;
		}
		return result;
	}

private:
	bool dirty = false;
	String attribute_name;
	String value_name;
};


class DataControllers {
public:

	void AddController(Element* element, DataControllerAttribute&& controller) {
		// TODO: Enable multiple controllers per element
		bool inserted = attribute_controllers.emplace(element, std::move(controller)).second;
		RMLUI_ASSERT(inserted);
	}

	bool Update(const DataModel& model)
	{
		bool result = false;
		for (auto& controller : attribute_controllers)
			result |= controller.second.Update(controller.first, model);
		return result;
	}


	void OnAttributeChange(DataModel& model, Element* element, const ElementAttributes& changed_attributes)
	{
		auto it = attribute_controllers.find(element);
		if (it != attribute_controllers.end())
		{
			it->second.OnAttributeChange(changed_attributes);
		}
	}

private:
	UnorderedMap<Element*, DataControllerAttribute> attribute_controllers;
};



class DataModel {
public:
	using Type = Variant::Type;

	struct Binding {
		Type type = Type::NONE;
		void* ptr = nullptr;
		bool writable = false;
	};

	bool GetValue(const String& name, String& out_value) const;
	bool SetValue(const String& name, const String& value) const;
	bool IsWritable(const String& name) const;

	using Bindings = Rml::Core::UnorderedMap<Rml::Core::String, Binding>;

	Bindings bindings;

	DataControllers controllers;
	DataViews views;
};


class DataModelHandle {
public:
	using Type = Variant::Type;

	DataModelHandle() : model(nullptr) {}
	DataModelHandle(DataModel* model) : model(model) {}

	DataModelHandle& BindData(String name, Type type, void* ptr, bool writable = false)
	{
		RMLUI_ASSERT(model);
		model->bindings.emplace(name, DataModel::Binding{ type, ptr, writable });
		return *this;
	}

	void UpdateControllers() {
		RMLUI_ASSERT(model);
		model->controllers.Update(*model);
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
