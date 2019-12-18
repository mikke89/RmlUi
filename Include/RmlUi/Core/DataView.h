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

#ifndef RMLUICOREDATAVIEW_H
#define RMLUICOREDATAVIEW_H

#include "Header.h"
#include "Types.h"
#include "Variant.h"
#include "StringUtilities.h"

namespace Rml {
namespace Core {

class Element;
class ElementText;
class DataModel;

class RMLUICORE_API DataView {

};

class DataViewText : public DataView {
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
		String binding_name;
		String value;
	};

	ObserverPtr<Element> element;
	String text;
	std::vector<DataEntry> data_entries;
};



class DataViewAttribute : public DataView {
public:
	DataViewAttribute(const DataModel& model, Element* element, const String& binding_name, const String& attribute_name);

	inline operator bool() const {
		return !attribute_name.empty() && element;
	}
	bool Update(const DataModel& model);

private:
	ObserverPtr<Element> element;
	String binding_name;
	String attribute_name;
};


class DataViewStyle : public DataView {
public:
	DataViewStyle(const DataModel& model, Element* element, const String& binding_name, const String& property_name);

	inline operator bool() const {
		return !binding_name.empty() && !property_name.empty() && element;
	}
	bool Update(const DataModel& model);

private:
	ObserverPtr<Element> element;
	String binding_name;
	String property_name;
};


class DataViewIf : public DataView {
public:
	DataViewIf(const DataModel& model, Element* element, const String& binding_name);

	inline operator bool() const {
		return !binding_name.empty() && element;
	}
	bool Update(const DataModel& model);

private:
	ObserverPtr<Element> element;
	String binding_name;
};


class DataViewFor : public DataView {
public:
	DataViewFor(const DataModel& model, Element* element, const String& binding_name, const String& rml_contents);

	inline operator bool() const {
		return !binding_name.empty() && element;
	}
	bool Update(const DataModel& model);

private:
	ObserverPtr<Element> element;
	String binding_name;
	String rml_contents;
	ElementAttributes attributes;

	std::vector<Element*> elements;
};


class RMLUICORE_API DataViews {
public:

	void AddView(DataViewText&& view) {
		text_views.push_back(std::move(view));
	}
	void AddView(DataViewAttribute&& view) {
		attribute_views.push_back(std::move(view));
	}
	void AddView(DataViewStyle&& view) {
		style_views.push_back(std::move(view));
	}
	void AddView(DataViewIf&& view) {
		if_views.push_back(std::move(view));
	}
	void AddView(DataViewFor&& view) {
		for_views.push_back(std::move(view));
	}

	bool Update(const DataModel& model)
	{
		bool result = false;
		for (auto& view : text_views)
			result |= view.Update(model);
		for (auto& view : attribute_views)
			result |= view.Update(model);
		for (auto& view : style_views)
			result |= view.Update(model);
		for (auto& view : if_views)
			result |= view.Update(model);
		for (auto& view : for_views)
			result |= view.Update(model);
		return result;
	}

private:
	std::vector<DataViewText> text_views;
	std::vector<DataViewAttribute> attribute_views;
	std::vector<DataViewStyle> style_views;
	std::vector<DataViewIf> if_views;
	std::vector<DataViewFor> for_views;
};

}
}

#endif
