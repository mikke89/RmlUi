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

#ifndef RMLUICOREDATAVIEWDEFAULT_H
#define RMLUICOREDATAVIEWDEFAULT_H

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/DataView.h"

namespace Rml {
namespace Core {

class Element;
class DataExpression;
using DataExpressionPtr = UniquePtr<DataExpression>;


class DataViewCommon : public DataView {
public:
	DataViewCommon(Element* element);
	~DataViewCommon();

	bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) override;

	StringList GetVariableNameList() const override;

protected:
	const String& GetModifier() const;
	DataExpression& GetExpression();

private:
	String modifier;
	DataExpressionPtr expression;
};


class DataViewAttribute final : public DataViewCommon {
public:
	DataViewAttribute(Element* element);
	~DataViewAttribute();

	bool Update(DataModel& model) override;
};


class DataViewStyle final : public DataViewCommon {
public:
	DataViewStyle(Element* element);
	~DataViewStyle();

	bool Update(DataModel& model) override;
};


class DataViewClass final : public DataViewCommon {
public:
	DataViewClass(Element* element);
	~DataViewClass();

	bool Update(DataModel& model) override;
};


class DataViewRml final : public DataViewCommon {
public:
	DataViewRml(Element* element);
	~DataViewRml();

	bool Update(DataModel& model) override;

private:
	String previous_rml;
};


class DataViewIf final : public DataViewCommon {
public:
	DataViewIf(Element* element);
	~DataViewIf();

	bool Update(DataModel& model) override;
};


class DataViewText final : public DataView {
public:
	DataViewText(Element* in_element);
	~DataViewText();

	bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) override;

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


class DataViewFor final : public DataView {
public:
	DataViewFor(Element* element);
	~DataViewFor();

	bool Initialize(DataModel& model, Element* element, const String& expression, const String& inner_rml) override;

	bool Update(DataModel& model) override;

	StringList GetVariableNameList() const override;

private:
	DataAddress container_address;
	String iterator_name;
	String rml_contents;
	ElementAttributes attributes;

	ElementList elements;
};

}
}

#endif
