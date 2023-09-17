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

#ifndef RMLUI_CORE_DATAMODEL_H
#define RMLUI_CORE_DATAMODEL_H

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

	inline DataTypeRegister* GetDataTypeRegister() const { return data_type_register; }

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
#endif
