#pragma once

#include "../../Include/RmlUi/Core/DataModelHandle.h"
#include "../../Include/RmlUi/Core/DataTypes.h"
#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Traits.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class DataViews;
class DataControllers;
class DataVariable;
class Element;
class FuncDefinition;

class DataModel : NonCopyMoveable {
public:
	DataModel(DataTypeRegister* data_type_register = nullptr);
	~DataModel();

	void AddView(DataViewPtr view);
	void AddController(DataControllerPtr controller);

	bool BindVariable(const String& name, DataVariable variable);
	bool BindFunc(const String& name, DataGetFunc get_func, DataSetFunc set_func);

	bool BindEventCallback(const String& name, DataEventFunc event_func);

	bool InsertAlias(Element* element, const String& alias_name, DataAddress replace_with_address);
	bool EraseAliases(Element* element);
	void CopyAliases(Element* source_element, Element* target_element);

	DataAddress ResolveAddress(const String& address_str, Element* element) const;
	const DataEventFunc* GetEventCallback(const String& name);

	DataVariable GetVariable(const DataAddress& address) const;
	bool GetVariableInto(const DataAddress& address, Variant& out_value) const;

	void DirtyVariable(const String& variable_name);
	bool IsVariableDirty(const String& variable_name) const;
	void DirtyAllVariables();

	bool CallTransform(const String& name, const VariantList& arguments, Variant& out_result) const;

	// Elements declaring 'data-model' need to be attached.
	void AttachModelRootElement(Element* element);
	ElementList GetAttachedModelRootElements() const;

	void OnElementRemove(Element* element);

	bool Update(bool clear_dirty_variables);

	DataTypeRegister* GetDataTypeRegister() const { return data_type_register; }
	const UnorderedMap<String, DataVariable>& GetAllVariables() const { return variables; }

private:
	UniquePtr<DataViews> views;
	UniquePtr<DataControllers> controllers;

	UnorderedMap<String, DataVariable> variables;
	DirtyVariables dirty_variables;

	UnorderedMap<String, UniquePtr<FuncDefinition>> function_variable_definitions;
	UnorderedMap<String, DataEventFunc> event_callbacks;

	using ScopedAliases = UnorderedMap<Element*, SmallUnorderedMap<String, DataAddress>>;
	ScopedAliases aliases;

	DataTypeRegister* data_type_register;

	SmallUnorderedSet<Element*> attached_elements;
};

} // namespace Rml
