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

#ifndef RMLUICOREDATAMODEL_H
#define RMLUICOREDATAMODEL_H

#include "Header.h"
#include "Types.h"
#include "Traits.h"
#include "Variant.h"
#include "DataView.h"
#include "DataController.h"
#include "DataVariable.h"

namespace Rml {
namespace Core {

class Element;


class RMLUICORE_API DataModel : NonCopyMoveable {
public:
	void AddView(UniquePtr<DataView> view) { views.Add(std::move(view)); }
	void AddController(UniquePtr<DataController> controller) { controllers.Add(std::move(controller)); }

	bool BindVariable(const String& name, Variable variable);

	bool InsertAlias(Element* element, const String& alias_name, Address replace_with_address);
	bool EraseAliases(Element* element);

	Address ResolveAddress(const String& address_str, Element* parent) const;

	Variable GetVariable(const Address& address) const;

	template<typename T>
	bool GetValue(const Address& address, T& out_value) const {
		Variant variant;
		Variable variable = GetVariable(address);
		return variable && variable.Get(variant) && variant.GetInto<T>(out_value);
	}

	void DirtyVariable(const String& variable_name);
	bool IsVariableDirty(const String& variable_name) const;

	void OnElementRemove(Element* element);
	void DirtyController(Element* element);

	bool Update();

private:
	DataViews views;
	DataControllers controllers;

	UnorderedMap<String, Variable> variables;
	SmallUnorderedSet<String> dirty_variables;

	using ScopedAliases = UnorderedMap<Element*, SmallUnorderedMap<String, Address>>;
	ScopedAliases aliases;
};


class RMLUICORE_API DataModelHandle {
public:
	DataModelHandle() : model(nullptr), type_register(nullptr) {}
	DataModelHandle(DataModel* model, DataTypeRegister* type_register) : model(model), type_register(type_register) {
		RMLUI_ASSERT(model && type_register);
	}

	void Update() {
		model->Update();
	}

	bool IsVariableDirty(const String& variable_name) {
		return model->IsVariableDirty(variable_name);
	}
	void DirtyVariable(const String& variable_name) {
		model->DirtyVariable(variable_name);
	}

	// Bind a data variable.
	// @note For non-scalar types make sure they first have been registered with the appropriate 'Register...()' functions.
	template<typename T> bool Bind(const String& name, T* ptr) {
		RMLUI_ASSERTMSG(ptr, "Invalid pointer to data variable");
		return model->BindVariable(name, Variable(type_register->GetOrAddScalar<T>(), ptr));
	}
	// Bind a get/set function pair.
	bool BindFunc(const String& name, DataGetFunc get_func, DataSetFunc set_func = {}) {
		VariableDefinition* func_definition = type_register->RegisterFunc(std::move(get_func), std::move(set_func));
		bool result = model->BindVariable(name, Variable(func_definition, nullptr));
		return result;
	}

	// Register a struct type.
	// @note The type applies to every data model associated with the current Context.
	// @return A handle which can be used to register struct members.
	template<typename T>
	StructHandle<T> RegisterStruct() {
		return type_register->RegisterStruct<T>();
	}

	// Register an array type.
	// @note The type applies to every data model associated with the current Context.
	// @note If 'Container::value_type' represents a non-scalar type, that type must already have been registered with the appropriate 'Register...()' functions.
	// @note Container requires the following functions implemented: size() and operator[]. This includes std::vector and std::array.
	template<typename Container>
	bool RegisterArray() {
		return type_register->RegisterArray<Container>();
	}

	explicit operator bool() { return model && type_register; }

private:
	DataModel* model;
	DataTypeRegister* type_register;

};

}
}

#endif
