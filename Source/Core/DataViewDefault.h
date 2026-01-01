#pragma once

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/Variant.h"
#include "DataView.h"

namespace Rml {

class Element;
class DataExpression;
using DataExpressionPtr = UniquePtr<DataExpression>;

class DataViewCommon : public DataView {
public:
	DataViewCommon(Element* element, String override_modifier = String(), int sort_offset = 0);

	bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) override;

	StringList GetVariableNameList() const override;

protected:
	const String& GetModifier() const;
	DataExpression& GetExpression();

	// Delete this
	void Release() override;

private:
	String modifier;
	DataExpressionPtr expression;
};

class DataViewAttribute : public DataViewCommon {
public:
	DataViewAttribute(Element* element);
	DataViewAttribute(Element* element, String override_attribute, int sort_offset);

	bool Update(DataModel& model) override;
};

class DataViewAttributeIf final : public DataViewCommon {
public:
	DataViewAttributeIf(Element* element);

	bool Update(DataModel& model) override;
};

class DataViewValue final : public DataViewAttribute {
public:
	DataViewValue(Element* element);
};

class DataViewChecked final : public DataViewCommon {
public:
	DataViewChecked(Element* element);

	bool Update(DataModel& model) override;
};

class DataViewStyle final : public DataViewCommon {
public:
	DataViewStyle(Element* element);

	bool Update(DataModel& model) override;
};

class DataViewClass final : public DataViewCommon {
public:
	DataViewClass(Element* element);

	bool Update(DataModel& model) override;
};

class DataViewRml final : public DataViewCommon {
public:
	DataViewRml(Element* element);

	bool Update(DataModel& model) override;

private:
	String previous_rml;
};

class DataViewIf final : public DataViewCommon {
public:
	DataViewIf(Element* element);

	bool Update(DataModel& model) override;
};

class DataViewVisible final : public DataViewCommon {
public:
	DataViewVisible(Element* element);

	bool Update(DataModel& model) override;
};

class DataViewText final : public DataView {
public:
	DataViewText(Element* in_element);

	bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) override;

	bool Update(DataModel& model) override;
	StringList GetVariableNameList() const override;

protected:
	void Release() override;

private:
	String BuildText() const;

	struct DataEntry {
		size_t index = 0; // Index into 'text'
		DataExpressionPtr data_expression;
		String value;
	};

	String text;
	Vector<DataEntry> data_entries;
};

class DataViewFor final : public DataView {
public:
	DataViewFor(Element* element);

	bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) override;

	bool Update(DataModel& model) override;

	StringList GetVariableNameList() const override;

protected:
	void Release() override;

private:
	const String* RMLContents() const;

	DataAddress container_address;
	String iterator_name;
	String iterator_index_name;
	ElementAttributes attributes;

	ElementList elements;
};

class DataViewAlias final : public DataView {
public:
	DataViewAlias(Element* element);
	virtual StringList GetVariableNameList() const override;
	bool Update(DataModel& model) override;
	bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) override;

protected:
	void Release() override;

private:
	StringList variables;
};

} // namespace Rml
