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
#include "Traits.h"
#include "DataVariable.h"
#include <unordered_map>

namespace Rml {
namespace Core {

class Element;
class ElementText;
class DataModel;
class DataExpression;
using DataExpressionPtr = UniquePtr<DataExpression>;

class RMLUICORE_API DataView : NonCopyMoveable {
public:
	virtual ~DataView();
	virtual bool Update(DataModel& model) = 0;
	virtual StringList GetVariableNameList() const = 0;

	bool IsValid() const { return (bool)attached_element; }
	explicit operator bool() const { return IsValid(); }

	Element* GetElement() const;
	int GetElementDepth() const { return element_depth; }
	
protected:
	DataView(Element* element);

	void InvalidateView() { attached_element.reset(); }

private:
	ObserverPtr<Element> attached_element;
	int element_depth;
};

class DataViewText final : public DataView {
public:
	DataViewText(DataModel& model, ElementText* in_element, const String& in_text, size_t index_begin_search = 0);
	~DataViewText();

	bool Update(DataModel& model) override;
	StringList GetVariableNameList() const override;

private:
	String BuildText() const;

	struct DataEntry {
		size_t index = 0; // Index into 'text'
		DataExpressionPtr data_expression;
		String value;
	};

	String text;
	std::vector<DataEntry> data_entries;
};



class DataViewAttribute final : public DataView {
public:
	DataViewAttribute(DataModel& model, Element* element, const String& binding_name, const String& attribute_name);
	~DataViewAttribute();

	bool Update(DataModel& model) override;

	StringList GetVariableNameList() const override;
private:
	String attribute_name;
	DataExpressionPtr data_expression;
};


class DataViewStyle final : public DataView {
public:
	DataViewStyle(DataModel& model, Element* element, const String& binding_name, const String& property_name);
	~DataViewStyle();

	bool Update(DataModel& model) override;

	StringList GetVariableNameList() const override;
private:
	String property_name;
	DataExpressionPtr data_expression;
};


class DataViewIf final : public DataView {
public:
	DataViewIf(DataModel& model, Element* element, const String& binding_name);
	~DataViewIf();

	bool Update(DataModel& model) override;

	StringList GetVariableNameList() const override;
private:
	DataExpressionPtr data_expression;
};


class DataViewFor final : public DataView {
public:
	DataViewFor(DataModel& model, Element* element, const String& binding_name, const String& rml_contents);

	bool Update(DataModel& model) override;

	StringList GetVariableNameList() const override {
		return variable_address.empty() ? StringList() : StringList{ variable_address.front().name };
	}

private:
	DataAddress variable_address;
	String alias_name;
	String rml_contents;
	ElementAttributes attributes;

	ElementList elements;
};


class RMLUICORE_API DataViews : NonCopyMoveable {
public:
	DataViews();
	~DataViews();

	void Add(UniquePtr<DataView> view);

	void OnElementRemove(Element* element);

	bool Update(DataModel& model, const SmallUnorderedSet< String >& dirty_variables);

private:
	using DataViewList = std::vector<UniquePtr<DataView>>;

	DataViewList views;
	
	DataViewList views_to_add;
	DataViewList views_to_remove;

	using NameViewMap = std::unordered_multimap<String, DataView*>;
	NameViewMap name_view_map;
};

}
}

#endif
