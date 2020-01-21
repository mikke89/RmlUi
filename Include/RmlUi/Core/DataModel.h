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
	bool Bind(String name, void* ptr, VariableDefinition* variable, VariableType type);

	Variable GetVariable(const String& address_str) const;
	Variable GetVariable(const Address& address) const;

	template<typename T>
	bool GetValue(const Address& address, T& out_value) const {
		Variant variant;
		Variable variable = GetVariable(address);
		return variable && variable.Get(variant) && variant.GetInto<T>(out_value);
	}

	void DirtyVariable(const String& variable_name);
	bool UpdateVariable(const String& variable_name);

	Address ResolveAddress(const String& address_str, Element* parent) const;

	void AddView(UniquePtr<DataView> view) { views.Add(std::move(view)); }

	// Todo: remove const / mutable. 
	bool InsertAlias(Element* element, const String& alias_name, Address replace_with_address) const;
	bool EraseAliases(Element* element) const;

	bool UpdateViews();

	void OnElementRemove(Element* element);

	// Todo: Make private
	DataControllers controllers;

private:
	UnorderedMap<String, Variable> variables;

	using ScopedAliases = UnorderedMap< Element*, SmallUnorderedMap<String, Address> >;
	mutable ScopedAliases aliases;

	DataViews views;

	SmallUnorderedSet< String > dirty_variables;
};


class RMLUICORE_API DataModelHandle {
public:
	DataModelHandle() : model(nullptr), type_register(nullptr) {}
	DataModelHandle(DataModel* model, DataTypeRegister* type_register) : model(model), type_register(type_register) {
		RMLUI_ASSERT(model && type_register);
	}

	void UpdateControllers() {
		model->controllers.Update(*model);
	}
	void UpdateViews() {
		model->UpdateViews();
	}

	bool UpdateVariable(const String& variable_name) {
		return model->UpdateVariable(variable_name);
	}
	void DirtyVariable(const String& variable_name) {
		model->DirtyVariable(variable_name);
	}

	template<typename T> bool BindScalar(String name, T* ptr) {
		static_assert(is_valid_scalar<T>::value, "Provided type is not a valid scalar type.");
		return model->Bind(name, ptr, type_register->GetOrAddScalar<T>(), VariableType::Scalar);
	}
	template<typename T> bool BindScalar(String name, T* ptr, MemberGetFunc<T> get_func, MemberSetFunc<T> set_func) {
		RMLUI_ASSERT(get_func || set_func);
		return model->Bind(name, ptr, type_register->RegisterMemberFunc<T>(get_func, set_func), VariableType::Scalar);
	}
	template<typename T> bool BindStruct(String name, T* ptr) {
		return model->Bind(name, ptr, type_register->Get<T>(), VariableType::Struct);
	}
	template<typename T> bool BindArray(String name, T* ptr) {
		return model->Bind(name, ptr, type_register->Get<T>(), VariableType::Array);
	}
	bool BindFunction(String name, DataGetFunc get_func, DataSetFunc set_func = {}) {
		FuncHandle func_handle = type_register->RegisterFunc(std::move(get_func), std::move(set_func));
		bool result = model->Bind(name, nullptr, func_handle.GetDefinition(), VariableType::Function);
		return result;
	}


	explicit operator bool() { return model && type_register; }

private:
	DataModel* model;
	DataTypeRegister* type_register;

};

}
}

#endif
