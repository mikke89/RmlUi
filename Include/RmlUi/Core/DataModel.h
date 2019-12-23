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
	DataModel(DataTypeRegister* type_register) : type_register(type_register) {}

	DataTypeRegister& GetTypeRegister() { return *type_register; }

	template<typename T> bool BindScalar(String name, T* ptr) {
		return Bind(name, ptr, type_register->GetOrAddScalar<T>(), VariableType::Scalar);
	}
	template<typename T> bool BindStruct(String name, T* ptr) {
		return Bind(name, ptr, type_register->Get<T>(), VariableType::Struct);
	}
	template<typename T> bool BindArray(String name, T* ptr) {
		return Bind(name, ptr, type_register->Get<T>(), VariableType::Array);
	}

	Variant GetValue(const String& address_str) const;
	bool SetValue(const String& address_str, const Variant& variant) const;

	template<typename T>
	bool GetValue(const Address& address, T& out_value) const {
		Variant variant;
		Variable variable = GetVariable(address);
		return variable && variable.Get(variant) && variant.GetInto<T>(out_value);
	}

	Variable GetVariable(const String& address_str) const;
	Variable GetVariable(const Address& address) const;

	Address ResolveAddress(const String& address_str, Element* parent) const;

	void AddView(UniquePtr<DataView> view) { views.Add(std::move(view)); }

	// Todo: remove const / mutable. 
	bool InsertAlias(Element* element, const String& alias_name, Address replace_with_address) const;
	bool EraseAliases(Element* element) const;

	bool UpdateViews() { return views.Update(*this); }

	// Todo: Make private
	DataControllers controllers;
private:
	bool Bind(String name, void* ptr, VariableDefinition* variable, VariableType type);

	DataTypeRegister* type_register;

	UnorderedMap<String, Variable> variables;

	using ScopedAliases = UnorderedMap< Element*, SmallUnorderedMap<String, Address> >;
	mutable ScopedAliases aliases;

	DataViews views;
};


class RMLUICORE_API DataModelHandle {
public:
	DataModelHandle(DataModel* model) : model(model) {}

	void UpdateControllers() {
		RMLUI_ASSERT(model);
		model->controllers.Update(*model);
	}

	void UpdateViews() {
		RMLUI_ASSERT(model);
		model->UpdateViews();
	}

	DataModel* GetModel() { return model; }

	operator bool() { return model != nullptr; }

private:
	DataModel* model;
};

}
}

#endif
